#include "SrtRbfNode.h"
#include "PoseVariable.h"
#include <vector>
#include <Eigen/Dense>
#include <Eigen/LU>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTransform.h>
#include <maya/MMatrix.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>
#include <maya/MQuaternion.h>
#include <maya/MEulerRotation.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MDagModifier.h>
#include <maya/MArgList.h>

const MString SrtRbfNode::className = "SrtRbfNode";
const MTypeId SrtRbfNode::SrtRbfNodeID = 0x00010; // TO BE CHANGED
const MString SrtRbfNode::versionAttrName[3]   = { "version",   "v",        "Version" };
const MString SrtRbfNode::inputAttrName[3]     = { "input",     "im",       "Input" };
const MString SrtRbfNode::outputAttrName[3]    = { "output",    "out",      "Output" };
const MString SrtRbfNode::numExsAttrName[3]    = { "examples",  "exs",      "Examples" };
const MString SrtRbfNode::targetAttrName[3]    = { "target",    "trgt",     "Target" };
const MString SrtRbfNode::primRefAttrName[3]   = { "primref",   "primref",  "Primary Reference" };
const MString SrtRbfNode::primaryAttrName[3]   = { "primary",   "prim",     "Primary Relative" };
const MString SrtRbfNode::secondaryAttrName[3] = { "secondary", "sec",      "Secondary" };
const MString SrtRbfNode::invKerMatAttrName[3] = { "invker",    "invker",   "Inverse Kernel" };
const MString SrtRbfNode::affinityAttrName[3]  = { "affinity",  "affinity", "Affinity Constraint" };
const MString SrtRbfNode::rbfAttrName[3]       = { "rbf",       "rbf",      "RBF Type" };
const MString SrtRbfNode::distAttrName[3]      = { "dist",      "dist",     "Distance Type" };
MObject SrtRbfNode::outputAttr   = MObject::kNullObj;
MObject SrtRbfNode::versionAttr  = MObject::kNullObj;
MObject SrtRbfNode::numExsAttr   = MObject::kNullObj;
MObject SrtRbfNode::affinityAttr = MObject::kNullObj;
MObject SrtRbfNode::rbfAttr      = MObject::kNullObj;
MObject SrtRbfNode::targetAttr   = MObject::kNullObj;


/// utility ///

MMatrix
GetWorldMatrix(
    MObject node)
{
    MFnTransform fnt(node);
    MObject matrixAttr = fnt.attribute("worldMatrix");
    MPlug matrixPlug(node, matrixAttr);
    matrixPlug = matrixPlug.elementByLogicalIndex(0);
    MFnMatrixData matrixData(matrixPlug.asMObject());
    return matrixData.matrix();
}

void
SetMatrix(
    MPlug plug,
    const MMatrix& matrix)
{
    MObject bmo;
    plug.getValue(bmo);
    MFnMatrixData bm(bmo);
    bm.set(matrix);
    plug.setValue(bmo);
}

MMatrix
GetMatrix(
    MPlug plug)
{
    MObject bmo;
    plug.getValue(bmo);
    MFnMatrixData bm(bmo);
    return bm.matrix();
}

MObject
FindNode(
    const MString& name)
{
    MSelectionList sl;
    MGlobal::getSelectionListByName(name, sl);
    MObject dnode = MObject::kNullObj;
    sl.getDependNode(0, dnode);
    return dnode;
}

/// 

MStatus
SrtRbfNode::initSrtRbfNode()
{
    // version info
    MFnNumericAttribute nAttr;
    versionAttr = nAttr.create(
        versionAttrName[0],
        versionAttrName[1],
        MFnNumericData::kInt,
        20220401);
    nAttr.setNiceNameOverride(versionAttrName[2]);
    nAttr.setReadable(false);
    nAttr.setWritable(false);
    addAttribute(versionAttr);

    // input matrices
    MFnMatrixAttribute mAttr;
    MObject inputAttr = mAttr.create(
        inputAttrName[0],
        inputAttrName[1],
        MFnMatrixAttribute::kDouble);
    mAttr.setNiceNameOverride(inputAttrName[2]);
    mAttr.setReadable(false);
    mAttr.setArray(true);
    mAttr.setKeyable(true);
    mAttr.setDisconnectBehavior(MFnAttribute::kReset);
    addAttribute(inputAttr);

    // output matrix
    outputAttr = mAttr.create(
        outputAttrName[0],
        outputAttrName[1],
        MFnMatrixAttribute::kDouble);
    mAttr.setNiceNameOverride(outputAttrName[2]);
    mAttr.setWritable(false);
    addAttribute(outputAttr);

    // # of examples
    numExsAttr = nAttr.create(
        numExsAttrName[0],
        numExsAttrName[1],
        MFnNumericData::kInt,
        0);
    nAttr.setNiceNameOverride(numExsAttrName[2]);
    addAttribute(numExsAttr);

    // affinity constraint (default: true)
    affinityAttr = nAttr.create(
        affinityAttrName[0],
        affinityAttrName[1],
        MFnNumericData::kBoolean,
        true);
    nAttr.setNiceNameOverride(affinityAttrName[2]);
    addAttribute(affinityAttr);

    // RBF type
    //  0: linearÅiDefaultÅj
    //  1: thinplate
    //  2: gaussian
    rbfAttr = nAttr.create(
        rbfAttrName[0],
        rbfAttrName[1],
        MFnNumericData::kInt,
        0);
    nAttr.setNiceNameOverride(rbfAttrName[2]);
    addAttribute(rbfAttr);

    // dissimilarity measure
    //  0: Geodesic distance on 3-hemisphere
    //  1: Euclidean distance in the Lie algebra (default)
    //  2: Shortest angle on 3-sphere
    //  3: Frobenious norm of diff matrix
    MObject distAttr = nAttr.create(
        distAttrName[0],
        distAttrName[1],
        MFnNumericData::kInt,
        1);
    nAttr.setNiceNameOverride(distAttrName[2]);
    addAttribute(distAttr);

    // target message
    MFnMessageAttribute msgAttr;
    targetAttr = msgAttr.create(
        targetAttrName[0],
        targetAttrName[1]);
    msgAttr.setNiceNameOverride(targetAttrName[2]);
    addAttribute(targetAttr);

    // reference primary transformation
    MObject primRefAttr = nAttr.create(
        primRefAttrName[0],
        primRefAttrName[1],
        MFnNumericData::kDouble,
        0.0);
    nAttr.setNiceNameOverride(primRefAttrName[2]);
    nAttr.setArray(true);
    nAttr.setKeyable(false);
    nAttr.setConnectable(false);
    addAttribute(primRefAttr);

    // primary transformations
    MObject primaryAttr = nAttr.create(
        primaryAttrName[0],
        primaryAttrName[1],
        MFnNumericData::kDouble,
        0.0);
    nAttr.setNiceNameOverride(primaryAttrName[2]);
    nAttr.setArray(true);
    nAttr.setKeyable(false);
    nAttr.setConnectable(false);
    addAttribute(primaryAttr);

    // secondary transformations
    MObject secondaryAttr = nAttr.create(
        secondaryAttrName[0],
        secondaryAttrName[1],
        MFnNumericData::kDouble,
        0.0);
    nAttr.setNiceNameOverride(secondaryAttrName[2]);
    nAttr.setArray(true);
    nAttr.setKeyable(false);
    nAttr.setConnectable(false);
    addAttribute(secondaryAttr);

    // inverse kernel matrix
    MObject invKerMatAttr = nAttr.create(
        invKerMatAttrName[0],
        invKerMatAttrName[1],
        MFnNumericData::kDouble,
        0.0);
    nAttr.setNiceNameOverride(invKerMatAttrName[2]);
    nAttr.setArray(true);
    nAttr.setKeyable(false);
    nAttr.setConnectable(false);
    addAttribute(invKerMatAttr);

    return MS::kSuccess;
}

MStatus
SrtRbfNode::setAffinityConstraint(
    bool flag)
{
    MFnDependencyNode fnThisNode(thisMObject());
    MPlug affPlug = fnThisNode.findPlug(affinityAttrName[0], true);
    affPlug.setBool(flag);
    return MStatus::kSuccess;
}

MStatus
SrtRbfNode::setRbfType(
    int type,
    double param = 1.0)
{
    MFnDependencyNode fnThisNode(thisMObject());
    MPlug rbfPlug = fnThisNode.findPlug(rbfAttrName[0], true);
    rbfPlug.setInt(type);
    return MStatus::kSuccess;
}

MStatus
SrtRbfNode::setGeodesicType(
    int type)
{
    MFnDependencyNode fnThisNode(thisMObject());
    MPlug distPlug = fnThisNode.findPlug(distAttrName[0], true);
    distPlug.setInt(type);
    return MStatus::kSuccess;
}

MStatus
SrtRbfNode::addExampleSupport(
    const std::vector<PoseVariable>& primPoses,
    const PoseVariable& opose)
{
    MFnDependencyNode fnThisNode(thisMObject());
    MPlug iplug      = fnThisNode.findPlug(inputAttrName[0], true);
    MPlug rbfPlug    = fnThisNode.findPlug(rbfAttrName[0], true);
    MPlug distPlug   = fnThisNode.findPlug(distAttrName[0], true);
    MPlug secPlug    = fnThisNode.findPlug(secondaryAttrName[0], true);
    MPlug priPlug    = fnThisNode.findPlug(primaryAttrName[0], true);
    MPlug numExsPlug = fnThisNode.findPlug(numExsAttr, true);
    const int numInputs = iplug.numElements();
    const int rbfType   = rbfPlug.asInt();
    const int distType  = distPlug.asInt();
    const int numExs    = numExsPlug.asInt();

    // check duplication
    for (int eid = 0; eid < numExs; ++eid)
    {
        auto rst = PoseVariable::getPosesFrom(priPlug, eid, numInputs);
        if (PoseVariable::dissimilarity(rst, primPoses, distType) < 1.0e-3)
        {
            MGlobal::displayInfo("Duplicated example");
            return MS::kInvalidParameter;
        }
    }

    // kernel matrix        
    MPlug affPlug = fnThisNode.findPlug(affinityAttrName[0], true);
    const bool affinityConstraint = affPlug.asBool();
    Eigen::MatrixXd kerMat;
    if (affinityConstraint)
    {
        kerMat.resize(numExs + 2, numExs + 2);
        kerMat.setOnes();
        kerMat(kerMat.rows() - 1, kerMat.cols() - 1) = 0.0;
    }
    else
    {
        kerMat.resize(numExs + 1, numExs + 1);
        kerMat.setOnes();
    }
    for (int r = 0; r < numExs; ++r)
    {
        std::vector<PoseVariable> rpose = PoseVariable::getPosesFrom(priPlug, r, numInputs);
        for (int c = r; c < numExs; ++c)
        {
            std::vector<PoseVariable> cpose = PoseVariable::getPosesFrom(priPlug, c, numInputs);
            kerMat(r, c) = PoseVariable::kernel(rpose, cpose, rbfType, distType);
            kerMat(c, r) = kerMat(r, c);
        }
        kerMat(r, numExs) = PoseVariable::kernel(rpose, primPoses, rbfType, distType);
        kerMat(numExs, r) = kerMat(r, numExs);
    }
    kerMat(numExs, numExs) = PoseVariable::kernel(primPoses, primPoses, rbfType, distType);

    // inverse kernel matrix
    Eigen::FullPivLU<Eigen::MatrixXd> kerMatLU(kerMat);
    if (kerMatLU.rank() < kerMat.rows())
    {
        MGlobal::displayError("Cannot add this example");
        return MStatus::kFailure;
    }
    Eigen::MatrixXd invKerMat = kerMatLU.inverse();
    MPlug icmPlug = fnThisNode.findPlug(invKerMatAttrName[0], true);
    for (int r = 0; r < invKerMat.rows(); ++r)
    {
        for (int c = 0; c < invKerMat.cols(); ++c)
        {
            icmPlug.elementByLogicalIndex(r * invKerMat.cols() + c).setValue(invKerMat(r, c));
        }
    }
    PoseVariable::setPosesTo(priPlug, numExs, numInputs, primPoses);
    PoseVariable::setPoseTo(secPlug, numExs, opose);
    numExsPlug.setValue(numExs + 1);
    return MStatus::kSuccess;
}

MStatus
SrtRbfNode::setDependentsDirty(
    const MPlug& plugBeingDirtied,
    MPlugArray& affectedPlugs)
{
    MFnDependencyNode fnThisNode(thisMObject());
    MString partialName = plugBeingDirtied.partialName();
    if (inputAttrName[1] == partialName
        || inputAttrName[1] != partialName.substring(0, inputAttrName[1].length() - 1))
    {
        return MS::kUnknownParameter;
    }
    affectedPlugs.append(fnThisNode.findPlug(outputAttrName[0], true));
    return MS::kSuccess;
}

void
SrtRbfNode::updateWeight(
    const MPlug& plug,
    MDataBlock& dataBlock)
{
    MFnDependencyNode fnThisNode(thisMObject());
    MPlug nePlug   = fnThisNode.findPlug(numExsAttrName[0], true);
    MPlug affPlug  = fnThisNode.findPlug(affinityAttrName[0], true);
    MPlug rbfPlug  = fnThisNode.findPlug(rbfAttrName[0], true);
    MPlug distPlug = fnThisNode.findPlug(distAttrName[0], true);
    MPlug iplug    = fnThisNode.findPlug(inputAttrName[0], true);
    MPlug refPlug  = fnThisNode.findPlug(primRefAttrName[0], true);
    const int numExs    = nePlug.asInt();
    const int rbfType   = rbfPlug.asInt();
    const int distType  = distPlug.asBool();
    const int numInputs = iplug.numElements();
    const bool affinityConstraint = affPlug.asBool();
    std::vector<PoseVariable> primPoses(numInputs);
    for (int iid = 0; iid < numInputs; ++iid)
    {
        MDataHandle iHandle = dataBlock.inputValue(iplug.elementByLogicalIndex(iid));
        MTransformationMatrix tm(iHandle.asMatrix());
        MQuaternion bq = PoseVariable::getRotateFrom(refPlug, iid);
        primPoses[iid] = PoseVariable::fromMatrix(tm).ontoHemisphere(bq);
        primPoses[iid].rotate = bq.conjugate() * primPoses[iid].rotate;
    }
    // inverse kernel matrix
    Eigen::MatrixXd invKerMat;
    if (affinityConstraint)
    {
        invKerMat.resize(numExs + 1, numExs + 1);
    }
    else
    {
        invKerMat.resize(numExs, numExs);
    }
    MPlug icmPlug = fnThisNode.findPlug(invKerMatAttrName[0], true);
    for (int r = 0; r < invKerMat.rows(); ++r)
    {
        for (int c = 0; c < invKerMat.cols(); ++c)
        {
            invKerMat(r, c) = icmPlug.elementByLogicalIndex(r * invKerMat.cols() + c).asDouble();
        }
    }
    Eigen::VectorXd distVec;
    if (affinityConstraint)
    {
        distVec.resize(numExs + 1);
        distVec[numExs] = 1.0;
    }
    else
    {
        distVec.resize(numExs);
    }
    MPlug priPlug = fnThisNode.findPlug(primaryAttrName[0], true);
    for (int eid = 0; eid < numExs; ++eid)
    {
        auto ppose = PoseVariable::getPosesFrom(priPlug, eid, numInputs);
        distVec[eid] = PoseVariable::kernel(ppose, primPoses, rbfType, distType);
    }
    weight = invKerMat * distVec;
}

MStatus
SrtRbfNode::compute(
    const MPlug& plug,
    MDataBlock& dataBlock)
{
    MFnDependencyNode fnThisNode(thisMObject());
    if (plug.attribute() != outputAttr)
    {
        return MS::kUnknownParameter;
    }
    updateWeight(plug, dataBlock);

    MPlug nePlug  = fnThisNode.findPlug(numExsAttrName[0], true);
    MPlug secPlug = fnThisNode.findPlug(secondaryAttrName[0], true);
    MPlug affPlug = fnThisNode.findPlug(affinityAttrName[0], true);
    const int numExs = nePlug.asInt();
    const bool affinityConstraint = affPlug.asBool();
    MVector ss(0, 0, 0);
    MVector st(0, 0, 0);
    for (int eid = 0; eid < numExs; ++eid)
    {
        ss += weight[eid] * PoseVariable::getScaleFrom(secPlug, eid);
        st += weight[eid] * PoseVariable::getTranslateFrom(secPlug, eid);
    }
    MQuaternion slr(0, 0, 0, 0);
    if (affinityConstraint)
    {
        for (int eid = 1; eid < numExs; ++eid)
        {
            slr = slr + weight[eid] * PoseVariable::getRotateFrom(secPlug, eid);
        }
        slr = PoseVariable::getRotateFrom(secPlug, 0) * slr.exp();
    }
    else
    {
        for (int eid = 0; eid < numExs; ++eid)
        {
            slr = slr + weight[eid] * PoseVariable::getRotateFrom(secPlug, eid);
        }
        slr = slr.exp();
    }
    MTransformationMatrix tm;
    double sv[] = { ss.x, ss.y, ss.z };
    tm.setScale(sv, MSpace::kTransform);
    tm.setRotationQuaternion(slr.x, slr.y, slr.z, slr.w);
    tm.setTranslation(st, MSpace::kTransform);
    MDataHandle outputHandle = dataBlock.outputValue(plug);
    outputHandle.setMMatrix(tm.asMatrix());
    outputHandle.setClean();
    return MS::kSuccess;
}

MStatus
SrtRbfNode::addExample(
    double add, // additional rotation
    int sign)    // sign of duplicated example
{
    MStatus status = MS::kSuccess;

    MFnDependencyNode fnThisNode(thisMObject());
    MPlug iplug      = fnThisNode.findPlug(inputAttrName[0], true);
    MPlug rbfPlug    = fnThisNode.findPlug(rbfAttrName[0], true);
    MPlug distPlug   = fnThisNode.findPlug(distAttrName[0], true);
    MPlug numExsPlug = fnThisNode.findPlug(numExsAttr, true);
    const int numInputs = iplug.numElements();
    const int rbfType   = rbfPlug.asInt();
    const int distType  = distPlug.asInt();

    std::vector<PoseVariable> primPoses;
    for (int i = 0; i < numInputs; ++i)
    {
        MPlug mPlug = iplug.elementByLogicalIndex(i);
        MFnMatrixData matrixData(mPlug.asMObject());
        MTransformationMatrix tm(matrixData.matrix());
        primPoses.push_back(PoseVariable::fromMatrix(tm));
    }

    MPlug tplug = fnThisNode.findPlug(targetAttrName[0], true);
    MPlugArray dparray;
    tplug.connectedTo(dparray, false, true);
    if (dparray.length() == 0)
    {
        return MS::kFailure;
    }
    MObject secNode = dparray[0].node();
    MFnTransform target(secNode);
    MTransformationMatrix targetTransform = target.transformation();
    MPlug secPlug = fnThisNode.findPlug(secondaryAttrName[0], true);
    MPlug refPlug = fnThisNode.findPlug(primRefAttrName[0], true);
    MPlug priPlug = fnThisNode.findPlug(primaryAttrName[0], true);

    if (numExsPlug.asInt() == 0)
    {
        PoseVariable secPose = PoseVariable::fromMatrix(targetTransform.asMatrix());
        PoseVariable::setPoseTo(secPlug, 0, secPose);
        for (int i = 0; i < numInputs; ++i)
        {
            PoseVariable::setPoseTo(refPlug, 0, i, numInputs, primPoses[i]);
            primPoses[i].rotate = MQuaternion::identity;
            PoseVariable::setPoseTo(priPlug, 0, i, numInputs, primPoses[i]);
        }
        numExsPlug.setValue(1);
    }
    else
    {
        PoseVariable secPose = PoseVariable::fromMatrix(targetTransform.asMatrix());
        // relativize
        MQuaternion sref = PoseVariable::getRotateFrom(secPlug, 0);
        secPose.ontoHemisphere(sref);
        secPose.rotate = PoseVariable::qlndiff(sref, secPose.rotate);
        double ra = std::sqrt(PoseVariable::qdot(secPose.rotate, secPose.rotate));
        if (add != 0 && std::fabs(ra) > 0)
        {
            secPose.rotate = (ra + 0.5 * add) / ra * secPose.rotate;
        }
        for (int i = 0; i < numInputs; ++i)
        {
            MQuaternion primrefr = PoseVariable::getRotateFrom(refPlug, i);
            primPoses[i].ontoHemisphere(primrefr);
            primPoses[i].rotate = primrefr.conjugate() * primPoses[i].rotate;
        }
        addExampleSupport(primPoses, secPose);

        // duplicated example
        if (sign != 0)
        {
            PoseVariable dupSecPose = secPose;
            dupSecPose.rotate = sign * secPose.rotate;
            std::vector<PoseVariable> dupPrimPose = primPoses;
            bool isSingular = false;
            for (int i = 0; i < numInputs; ++i)
            {
                if (std::abs(primPoses[i].rotate.w) < 1.0e-6)
                {
                    dupPrimPose[i].rotate = -dupPrimPose[i].rotate;
                    isSingular = true;
                }
            }
            if (isSingular)
            {
                bool isOriginal = true;
                for (int eid = 0; eid < numExsPlug.asInt(); ++eid)
                {
                    auto ppose = PoseVariable::getPosesFrom(priPlug, eid, numInputs);
                    if (PoseVariable::dissimilarity(ppose, dupPrimPose, distType) < 1.0e-6)
                    {
                        isOriginal = false;
                        break;
                    }
                }
                if (isOriginal)
                {
                    MGlobal::displayInfo("Duplicating example");
                    addExampleSupport(dupPrimPose, dupSecPose);
                }
            }
        }
    }
    return status;
}

MStatus
SrtRbfNode::gotoExample(
    int eid)
{
    MFnDependencyNode fnThisNode(thisMObject());
    MPlug nePlug = fnThisNode.findPlug(numExsAttrName[0], true);
    const int numExs = nePlug.asInt();
    if (eid >= numExs)
    {
        return MS::kInvalidParameter;
    }

    MPlug iplug = fnThisNode.findPlug(inputAttrName[0], true);
    MPlug priPlug = fnThisNode.findPlug(primaryAttrName[0], true);
    const int numInputs = iplug.numElements();
    const std::vector<PoseVariable> primPoses =
        PoseVariable::getPosesFrom(priPlug, eid, numInputs);
    for (int i = 0; i < numInputs; ++i)
    {
        MPlug ip = iplug.elementByLogicalIndex(i).source();
        if (ip.isNull())
        {
            continue;
        }
        MFnTransform source(ip.node());
        source.setTranslation(primPoses[i].translate, MSpace::kTransform);
        double sv[3] = { primPoses[i].scale.x, primPoses[i].scale.y, primPoses[i].scale.z };
        source.setScale(sv);
        source.setRotation(primPoses[i].rotate);
    }
    MVector ss(1.0, 1.0, 1.0);
    MVector st(0, 0, 0);
    MQuaternion sq(0, 0, 0, 1.0);
    MPlug secPlug = fnThisNode.findPlug(secondaryAttrName[0], true);
    if (!secPlug.isNull())
    {
        st = PoseVariable::getTranslateFrom(secPlug, eid);
        sq = PoseVariable::getRotateFrom(secPlug, eid);
        ss = PoseVariable::getScaleFrom(secPlug, eid);
        if (eid > 0)
        {
            sq = PoseVariable::getRotateFrom(secPlug, 0) * sq.exp();
        }
    }
    return MStatus::kSuccess;
}

///

std::vector<SrtRbfNode*>
NodesFromActiveSelection()
{
    MSelectionList asl;
    MGlobal::getActiveSelectionList(asl);
    std::vector<SrtRbfNode*> SrtRbfNodes;
    for (MItSelectionList slit(asl, MFn::kPluginDependNode); !slit.isDone(); slit.next())
    {
        MObject node;
        slit.getDependNode(node);
        MFnDependencyNode nodeFn(node);
        SrtRbfNode* mNode = dynamic_cast<SrtRbfNode*>(nodeFn.userNode());
        if (mNode != nullptr)
        {
            SrtRbfNodes.push_back(mNode);
        }
    }
    return SrtRbfNodes;
}

MStatus
CreateSrtRbfNode::doIt(
    const MArgList& args)
{
    MDGModifier dgModifier;
    MSelectionList asl;
    MGlobal::getActiveSelectionList(asl);
    for (MItSelectionList slit(asl, MFn::kTransform); !slit.isDone(); slit.next())
    {
        MObject node = MObject::kNullObj;
        if (slit.getDependNode(node) != MS::kSuccess)
        {
            continue;
        }
        if (node.apiType() != MFn::kTransform && node.apiType() != MFn::kJoint)
        {
            continue;
        }
        MFnDependencyNode nodeFn(node);
        if (nodeFn.hasAttribute(SrtRbfNode::className))
        {
            continue;
        }
        MFnDependencyNode srtRbfNode(dgModifier.createNode(SrtRbfNode::className));
        MFnMessageAttribute srtRbfAttrFn;
        MObject srtRbfAttr = srtRbfAttrFn.create(SrtRbfNode::className, SrtRbfNode::className);
        dgModifier.addAttribute(node, srtRbfAttr);
        MPlug srcPlug = srtRbfNode.findPlug(SrtRbfNode::targetAttrName[0], true);
        MPlug dstPlug = nodeFn.findPlug(srtRbfAttr, true);
        MStatus x = dgModifier.connect(srcPlug, dstPlug);
        if (args.length() > 0)
        {
            dgModifier.renameNode(srtRbfNode.object(), args.asString(0));
        }
        dgModifier.doIt();
        appendToResult(srtRbfNode.name());
    }
    return MS::kSuccess;
}

MStatus
AddSrtRbfExample::doIt(
    const MArgList& args)
{
    std::vector<SrtRbfNode*> controllers = NodesFromActiveSelection();
    for (auto it = controllers.begin(); it != controllers.end(); ++it)
    {
        (*it)->addExample(args.length() == 0 ? 0 : args.asDouble(0), args.length() < 2 ? -1 : args.asInt(1));
    }
    return MS::kSuccess;
}

#ifndef SRTRBF_NODE_H
#define SRTRBF_NODE_H
#pragma once

#include <maya/MPxNode.h>
#include <maya/MPxCommand.h>
#include <maya/MString.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MObjectArray.h>
#include <Eigen/Dense>
#include <vector>
#include "PoseVariable.h"

class SrtRbfNode : public MPxNode
{
//
// attribute names
public:
    static const MString className;
    static const MString inputAttrName[3];
    static const MString outputAttrName[3];
    static const MString versionAttrName[3];
    static const MString numExsAttrName[3];
    static const MString primRefAttrName[3];
    static const MString primaryAttrName[3];
    static const MString secondaryAttrName[3];
    static const MString invKerMatAttrName[3];
    static const MString affinityAttrName[3];
    static const MString rbfAttrName[3];
    static const MString distAttrName[3];
    static const MString targetAttrName[3];
//
// attributes
protected:
    static MObject versionAttr;
    static MObject outputAttr;
    static MObject numExsAttr;
    static MObject affinityAttr;
    static MObject rbfAttr;
    static MObject targetAttr;
//
// interpolation weight
private:
    Eigen::VectorXd weight;
    void
    updateWeight(
        const MPlug& plug,
        MDataBlock& dataBlock);
//
// constructor & destructor
public:
    SrtRbfNode() { };
    virtual ~SrtRbfNode() { };
//
// overrides
public:
    MStatus
    setDependentsDirty(
        const MPlug& plugBeingDirtied,
        MPlugArray& affectedPlugs) override;
    MStatus
    compute(
        const MPlug& plug,
        MDataBlock& dataBlock) override;
    MStatus
    addExample(
        double add,
        int sign);
    MStatus
    setAffinityConstraint(
        bool flag);
    MStatus
    setRbfType(
        int type,
        double param);
    MStatus
    setGeodesicType(
        int type);
    MStatus
    gotoExample(
        int eid);
protected:
    MStatus
    addExampleSupport(
        const std::vector<PoseVariable>& primPose,
        const PoseVariable& secPpose);
//
// node generation
public:
    static MStatus
    initSrtRbfNode();
    static const MTypeId SrtRbfNodeID;
};

///

class CreateSrtRbfNode : public MPxCommand
{
public:
    virtual MStatus
    doIt(
        const MArgList& args);
};

///

class AddSrtRbfExample : public MPxCommand
{
public:
    virtual MStatus
    doIt(
        const MArgList& args);
};

#endif //SRTRBF_NODE_H

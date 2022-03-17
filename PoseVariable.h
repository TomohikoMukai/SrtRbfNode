#ifndef POSE_VARIABLE_H
#define POSE_VARIABLE_H
#pragma once

#include <maya/MVector.h>
#include <maya/MQuaternion.h>
#include <maya/MTransformationMatrix.h>
#include <vector>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>

struct PoseVariable
{
    MVector     scale;
    MQuaternion rotate;
    MVector     translate;

    PoseVariable()
        : scale(1, 1, 1),
        rotate(0, 0, 0, 1),
        translate(0, 0, 0)
    {
    }
    PoseVariable(
        const PoseVariable& src)
        : scale(src.scale),
        rotate(src.rotate),
        translate(src.translate)
    {
    }
    PoseVariable&
    operator =(
        const PoseVariable& src)
    {
        scale     = src.scale;
        rotate    = src.rotate;
        translate = src.translate;
        return *this;
    }
    static PoseVariable
    fromMatrix(
        const MTransformationMatrix& tm)
    {
        PoseVariable c;
        double sv[3];
        tm.getScale(sv, MSpace::kTransform);
        c.scale = MVector(sv[0], sv[1], sv[2]);
        tm.getRotationQuaternion(c.rotate.x, c.rotate.y, c.rotate.z, c.rotate.w);
        c.translate = tm.getTranslation(MSpace::kTransform);
        c.truncateEpsilon();
        return c;
    }
    static MMatrix
    toMatrix(
        const PoseVariable& pv)
    {
        MTransformationMatrix tm = MTransformationMatrix::identity;
        double sv[3] = { pv.scale.x, pv.scale.y, pv.scale.z };
        tm.setScale(sv, MSpace::kTransform);
        tm.setRotationQuaternion(pv.rotate.x, pv.rotate.y, pv.rotate.z, pv.rotate.w);
        tm.setTranslation(pv.translate, MSpace::kTransform);
        return tm.asMatrix();
    }
public:
    PoseVariable&
    ontoHemisphere(
        MQuaternion pole = MQuaternion::identity)
    {
        if (qdot(pole, rotate) < 0)
        {
            rotate = -rotate;
        }
        else if (rotate.x == -1 || rotate.y == -1 || rotate.z == -1)
        {
            rotate = -rotate;
        }
        return *this;
    }
    PoseVariable&
    truncateEpsilon(
        double epsilon = 1.0e-9)
    {
        for (int i = 0; i < 4; ++i)
        {
            rotate[i] = std::abs(rotate[i]) < epsilon ? 0 : rotate[i];
        }
        return *this;
    }

public:
    static double
    qdot(
        MQuaternion a,
        MQuaternion b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    static double
    vdot(
        MVector a,
        MVector b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    static MQuaternion
    qlndiff(
        MQuaternion a,
        MQuaternion b)
    {
        MQuaternion qd = a.conjugate() * b;
        return qd.log();
    }

    static double
    lqdistsq(
        MQuaternion a,
        MQuaternion b)
    {
        MQuaternion qd = a.log() - b.log();
        return qdot(qd, qd);
    }

    static double
    qangle(
        MQuaternion a,
        MQuaternion b)
    {
        return 2.0 * std::acos(qdot(a, b));
    }

    static double
    qangleShortest(
        MQuaternion a,
        MQuaternion b)
    {
        return 2.0 * std::acos(std::min(std::abs(qdot(a, b)), 1.0));
    }

    static double
    linear(
        double d,
        double p)
    {
        return d;
    }

    static double
    thinplate(
        double d,
        double p)
    {
        if (std::abs(d) < 1.0e-6)
        {
            return 0;
        }
        return std::pow(d, 2) * std::log(d);
    }

    static double
    gaussian(
        double d,
        double p)
    {
        return std::exp(-d * d / p);
    }

    static double
    dissimilarity(
        const std::vector<PoseVariable>& a,
        const std::vector<PoseVariable>& b,
        int distType = 0,
        double ws = 1.0,
        double wr = 1.0,
        double wt = 1.0)
    {
        if (distType == 3) //Frobenious norm of diff matrix
        {
            double fnrm = 0.0;
            for (int i = 0; i < a.size(); ++i)
            {
                MMatrix dm = toMatrix(a[i]) - toMatrix(b[i]);
                for (int j = 0; j < 16; ++j)
                {
                    fnrm += std::pow(dm(j / 4, j % 4), 2.0);
                }
            }
            return fnrm <= 0 ? 0.0 : std::sqrt(fnrm);
        }
        double sqe = 0.0; // weighted Euclidean norm
        for (int i = 0; i < a.size(); ++i)
        {
            MVector sv = a[i].scale - b[i].scale;
            MVector tv = a[i].translate - b[i].translate;
            double dssq = vdot(sv, sv);
            double dtsq = vdot(tv, tv);
            sqe += ws * dssq + wt * dtsq;
        }
        switch (distType)
        {
        case 1: // Euclidean distance in tangent vector space
            for (int i = 0; i < a.size(); ++i)
            {
                double drsq = PoseVariable::lqdistsq(a[i].rotate, b[i].rotate);
                sqe += wr * drsq;
            }
            break;
        case 2: // Shortest angle
            for (int i = 0; i < a.size(); ++i)
            {
                double dr = PoseVariable::qangleShortest(a[i].rotate, b[i].rotate);
                sqe += wr * dr * dr;
            }
            break;
        case 0: // Angle on 3-hemisphere
        default:
            for (int i = 0; i < a.size(); ++i)
            {
                double dr = PoseVariable::qangle(a[i].rotate, b[i].rotate);
                sqe += wr * dr * dr;
            }
            break;
        }
        return std::sqrt(sqe);
    }

    static double
    kernel(
        const std::vector<PoseVariable>& a,
        const std::vector<PoseVariable>& b,
        int rbfType = 1,
        int distType = 0,
        double ws = 1.0,
        double wr = 10.0,
        double wt = 1.0)
    {
        static std::function<double(double, double)> ftable[3] = {
            linear, thinplate, gaussian };
        double d = dissimilarity(a, b, distType, ws, wr, wt);
        return ftable[rbfType](d, 10.0);
    }

public:
    static void
    setPoseTo(
        MPlug& plug,
        int offset,
        const PoseVariable& pose)
    {
        offset *= 10;
        plug.elementByLogicalIndex(offset + 0).setValue(pose.scale.x);
        plug.elementByLogicalIndex(offset + 1).setValue(pose.scale.y);
        plug.elementByLogicalIndex(offset + 2).setValue(pose.scale.z);
        plug.elementByLogicalIndex(offset + 3).setValue(pose.rotate.x);
        plug.elementByLogicalIndex(offset + 4).setValue(pose.rotate.y);
        plug.elementByLogicalIndex(offset + 5).setValue(pose.rotate.z);
        plug.elementByLogicalIndex(offset + 6).setValue(pose.rotate.w);
        plug.elementByLogicalIndex(offset + 7).setValue(pose.translate.x);
        plug.elementByLogicalIndex(offset + 8).setValue(pose.translate.y);
        plug.elementByLogicalIndex(offset + 9).setValue(pose.translate.z);
    }
    static void
    setPoseTo(
        MPlug& plug,
        int eid,
        int iid,
        int numInputs,
        const PoseVariable& pose)
    {
        setPoseTo(plug, eid * numInputs + iid, pose);
    }
    static void
    setPosesTo(
        MPlug& plug,
        int eid,
        int numInputs,
        const std::vector<PoseVariable>& poses)
    {
        for (int iid = 0; iid < numInputs; ++iid)
        {
            setPoseTo(plug, eid * numInputs + iid, poses[iid]);
        }
    }
    static PoseVariable
    getPoseFrom(
        MPlug& plug,
        int offset)
    {
        offset *= 10;
        PoseVariable pose;
        pose.scale.x     = plug.elementByLogicalIndex(offset + 0).asDouble();
        pose.scale.y     = plug.elementByLogicalIndex(offset + 1).asDouble();
        pose.scale.z     = plug.elementByLogicalIndex(offset + 2).asDouble();
        pose.rotate.x    = plug.elementByLogicalIndex(offset + 3).asDouble();
        pose.rotate.y    = plug.elementByLogicalIndex(offset + 4).asDouble();
        pose.rotate.z    = plug.elementByLogicalIndex(offset + 5).asDouble();
        pose.rotate.w    = plug.elementByLogicalIndex(offset + 6).asDouble();
        pose.translate.x = plug.elementByLogicalIndex(offset + 7).asDouble();
        pose.translate.y = plug.elementByLogicalIndex(offset + 8).asDouble();
        pose.translate.z = plug.elementByLogicalIndex(offset + 9).asDouble();
        return pose;
    }
    static PoseVariable
    getPoseFrom(
        MPlug& plug,
        int eid,
        int iid,
        int numInputs)
    {
        return getPoseFrom(plug, eid * numInputs + iid);
    }
    static std::vector<PoseVariable>
    getPosesFrom(
        MPlug& plug,
        int eid,
        int numInputs)
    {
        std::vector<PoseVariable> retval;
        for (int iid = 0; iid < numInputs; ++iid)
        {
            retval.push_back(getPoseFrom(plug, eid, iid, numInputs));
        }
        return retval;
    }

    static MVector
    getScaleFrom(
        MPlug& plug,
        int offset)
    {
        offset *= 10;
        MVector scale;
        scale.x = plug.elementByLogicalIndex(offset + 0).asDouble();
        scale.y = plug.elementByLogicalIndex(offset + 1).asDouble();
        scale.z = plug.elementByLogicalIndex(offset + 2).asDouble();
        return scale;
    }
    static MVector
    getScaleFrom(
        MPlug& plug,
        int eid,
        int iid,
        int numInputs)
    {
        return getScaleFrom(plug, eid * numInputs + iid);
    }
    static MQuaternion
    getRotateFrom(
        MPlug& plug,
        int offset)
    {
        offset *= 10;
        MQuaternion rotate;
        rotate.x = plug.elementByLogicalIndex(offset + 3).asDouble();
        rotate.y = plug.elementByLogicalIndex(offset + 4).asDouble();
        rotate.z = plug.elementByLogicalIndex(offset + 5).asDouble();
        rotate.w = plug.elementByLogicalIndex(offset + 6).asDouble();
        return rotate;
    }
    static MQuaternion
    getRotateFrom(
        MPlug& plug,
        int eid,
        int iid,
        int numInputs)
    {
        return getRotateFrom(plug, eid * numInputs + iid);
    }
    static MVector
    getTranslateFrom(
        MPlug& plug,
        int offset)
    {
        offset *= 10;
        MVector translate;
        translate.x = plug.elementByLogicalIndex(offset + 7).asDouble();
        translate.y = plug.elementByLogicalIndex(offset + 8).asDouble();
        translate.z = plug.elementByLogicalIndex(offset + 9).asDouble();
        return translate;
    }
    static MVector
    getTranslateFrom(
        MPlug& plug,
        int eid,
        int iid,
        int numInputs)
    {
        return getTranslateFrom(plug, eid * numInputs + iid);
    }
};

#endif //POSE_VARIABLE_H

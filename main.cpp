#include "SrtRbfNode.h"
#include <maya/MFnPlugin.h>

MStatus initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj, "Mukai Lab", "v.2022.4.1", "2018-2022");
    status = plugin.registerNode(SrtRbfNode::className, SrtRbfNode::SrtRbfNodeID,
        []()->void* {return new SrtRbfNode(); },
        SrtRbfNode::initSrtRbfNode);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = plugin.registerCommand("CreateSrtRbfNode",
        []()->void* { return new CreateSrtRbfNode; });
    CHECK_MSTATUS(status);
    status = plugin.registerCommand("AddSrtRbfExample",
        []()->void* { return new AddSrtRbfExample; });
    CHECK_MSTATUS(status);
    return status;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj);
    status = plugin.deregisterCommand("CreateSrtRbfNode");
    CHECK_MSTATUS(status);
    status = plugin.deregisterCommand("AddSrtRbfExample");
    CHECK_MSTATUS(status);
    status = plugin.deregisterNode(SrtRbfNode::SrtRbfNodeID);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    return status;
}

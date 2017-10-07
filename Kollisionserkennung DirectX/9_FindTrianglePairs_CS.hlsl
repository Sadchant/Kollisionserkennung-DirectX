#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<CellTrianglePair> cellTrianglePairs : register(u0);
RWStructuredBuffer<BoundingBox> boundingBoxes : register(u1);
RWStructuredBuffer<TrianglePair> trianglePairs : register(u2);

RWStructuredBuffer<WorkPosition> cellTrianglePairsWorkPositions : register(u3);


cbuffer Bool_UseWorkPositions : register(b0)
{
    // wird direkt aus cellTrianglePairs gelesen oder die ID, wo der ALgorithmus startet aus CellTrianglePairsWorkPositions benutzt?
    uint bool_UseWorkPositions;
};

bool checkBoundingBoxIntersection(uint boundingBoxID1, uint boundingBoxID2)
{
    BoundingBox boundingBox1 = boundingBoxes[boundingBoxID1];
    BoundingBox boundingBox2 = boundingBoxes[boundingBoxID2];
    return !(((boundingBox1.maxPoint.x < boundingBox2.minPoint.x) || (boundingBox1.minPoint.x > boundingBox2.maxPoint.x)) ||
             ((boundingBox1.maxPoint.y < boundingBox2.minPoint.y) || (boundingBox1.minPoint.y > boundingBox2.maxPoint.y)) ||
             ((boundingBox1.maxPoint.z < boundingBox2.minPoint.z) || (boundingBox1.minPoint.z > boundingBox2.maxPoint.z)));
}


[numthreads(LINEAR_XTHREADS, LINEAR_YTHREADS, LINEAR_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint id = DTid.x;
    uint cellTrianglePairsLength, stride, sortIndicesLength;
    cellTrianglePairs.GetDimensions(cellTrianglePairsLength, stride);

    if (id >= cellTrianglePairsLength)
        return;
    CellTrianglePair firstCellTrianglePair;
    uint curID;
    if (bool_UseWorkPositions)
    {
        WorkPosition curWorkPosition = cellTrianglePairsWorkPositions[id];
        if (curWorkPosition.workPosition == 0)
            return;
        WorkPosition emptyWorkPosition = { { 0, 0, 0 }, 0 };
        cellTrianglePairsWorkPositions[id] = emptyWorkPosition;

        firstCellTrianglePair = curWorkPosition.blockFirstCellTrianglePair;
        curID = curWorkPosition.workPosition;
    }
    else
    {
        firstCellTrianglePair = cellTrianglePairs[id];
        curID = id + 1;
    }
    if (firstCellTrianglePair.cellID == 0)
        return;
        
    CellTrianglePair nextCellTrianglePair = cellTrianglePairs[curID];
    int counter = 0;
    while ((firstCellTrianglePair.cellID == nextCellTrianglePair.cellID) && (nextCellTrianglePair.cellID != 0) && (counter < _9_WORKSIZE))
    {
        if (firstCellTrianglePair.objectID != nextCellTrianglePair.objectID)
        {
            if (checkBoundingBoxIntersection(firstCellTrianglePair.triangleID, nextCellTrianglePair.triangleID))
            {
                TrianglePair newTrianglePair = { firstCellTrianglePair.triangleID, nextCellTrianglePair.triangleID, firstCellTrianglePair.objectID, nextCellTrianglePair.objectID };
                uint curCount = trianglePairs.IncrementCounter();
                trianglePairs[curCount] = newTrianglePair;
            }
        }
        curID++;
        nextCellTrianglePair = cellTrianglePairs[curID];
        counter++;
        if ((counter == _9_WORKSIZE) && (firstCellTrianglePair.cellID == nextCellTrianglePair.cellID) && (nextCellTrianglePair.cellID != 0))
        {
            WorkPosition newWorkPosition = { firstCellTrianglePair, curID };
            uint curCount = cellTrianglePairsWorkPositions.IncrementCounter();
            cellTrianglePairsWorkPositions[curCount] = newWorkPosition;
        }
    }
}
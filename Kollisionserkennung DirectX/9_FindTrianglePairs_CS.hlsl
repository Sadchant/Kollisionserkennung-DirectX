#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<CellTrianglePair> cellTrianglePairs : register(u0);
RWStructuredBuffer<BoundingBox> boundingBoxes : register(u1);
RWStructuredBuffer<TrianglePair> trianglePairs : register(u2);

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
    CellTrianglePair curCellTrianglePair = cellTrianglePairs[id];
    
    if (curCellTrianglePair.cellID == 0)
        return;
    uint curID = id + 1;
    CellTrianglePair nextCellTrianglePair = cellTrianglePairs[curID];
    int counter = 0;
    while ((curCellTrianglePair.cellID == nextCellTrianglePair.cellID) && (nextCellTrianglePair.cellID != 0))
    {
        if (curCellTrianglePair.objectID != nextCellTrianglePair.objectID)
        {
            if (checkBoundingBoxIntersection(curCellTrianglePair.triangleID, nextCellTrianglePair.triangleID))
            {
                TrianglePair newTrianglePair = { curCellTrianglePair.triangleID, nextCellTrianglePair.triangleID, curCellTrianglePair.objectID, nextCellTrianglePair.objectID };
                uint curCount = trianglePairs.IncrementCounter();
                trianglePairs[curCount] = newTrianglePair;
                //trianglePairs.Append(newTrianglePair);
            }
        }
        curID++;
        nextCellTrianglePair = cellTrianglePairs[curID];
        counter++;
    }
    //TrianglePair newTrianglePair = { curCellTrianglePair.cellID, 12 };
    //trianglePairs.Append(newTrianglePair);
}
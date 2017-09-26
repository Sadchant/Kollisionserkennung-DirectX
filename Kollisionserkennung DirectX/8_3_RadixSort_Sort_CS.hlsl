#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<SortIndices> sortIndices : register(u0);
RWStructuredBuffer<CellTrianglePair> cellTrianglePairsInput : register(u1);
RWStructuredBuffer<CellTrianglePair> cellTrianglePairsOutput : register(u2);


[numthreads(LINEAR_XTHREADS, LINEAR_YTHREADS, LINEAR_ZTHREADS)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint id = DTid.x;

    uint cellTrianglePairsLength, stride, sortIndicesLength;
    cellTrianglePairsInput.GetDimensions(cellTrianglePairsLength, stride);

    if(id >= cellTrianglePairsLength)
        return;

    sortIndices.GetDimensions(sortIndicesLength, stride);

    SortIndices curSortIndex = sortIndices[id];
    SortIndices nextSortIndex = sortIndices[id + 1];
    uint curOffset = 0;
    for (int i = 0; i < 4; i++)
    {
        if (i > 0)
            curOffset += sortIndices[sortIndicesLength - 1].array[i - 1];
        if (curSortIndex.array[i] != nextSortIndex.array[i])
        {
            cellTrianglePairsOutput[curSortIndex.array[i] + curOffset] = cellTrianglePairsInput[id];
        }
    }
    //cellTrianglePairsOutput[id] = cellTrianglePairs[id];
}



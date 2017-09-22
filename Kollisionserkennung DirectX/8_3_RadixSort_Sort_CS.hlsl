#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<SortIndices> sortIndices : register(u0);
RWStructuredBuffer<CellTrianglePair> cellTrianglePairs : register(u1);
RWStructuredBuffer<CellTrianglePair> cellTrianglePairsBackBuffer : register(u2);

cbuffer BackBufferIsInput : register(b0)
{
    uint bool_BackBufferIsInput;
}; 

[numthreads(_8_3_RADIXSORT_SORT_XTHREADS, _8_3_RADIXSORT_SORT_YTHREADS, _8_3_RADIXSORT_SORT_ZTHREADS)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint id = DTid.x;

    uint cellTrianglePairsLength, stride, sortIndicesLength;
    cellTrianglePairs.GetDimensions(cellTrianglePairsLength, stride);

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
            if (bool_BackBufferIsInput == 1)            
                cellTrianglePairs[curSortIndex.array[i] + curOffset] = cellTrianglePairsBackBuffer[id];
            else
                cellTrianglePairsBackBuffer[curSortIndex.array[i] + curOffset] = cellTrianglePairs[id];
        }
    }
    //cellTrianglePairsBackBuffer[id] = cellTrianglePairs[id];
}



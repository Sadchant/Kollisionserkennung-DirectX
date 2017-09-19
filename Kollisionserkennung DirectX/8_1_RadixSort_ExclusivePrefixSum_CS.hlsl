#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<CellTrianglePair> cellTrianglePairs : register(u0);
RWStructuredBuffer<SortIndices> sortIndices : register(u1);

groupshared SortIndices groupSortIndices[2048];

cbuffer radixSort_ExclusivePrefixSum_Data : register(b0)
{
    uint loops;
    int read2BitsFromHere; // -1 falls die Bits schon ausgelesen wurden
    uint startCombineDistance;
}

SortIndices getSortIndicesFromInput(uint id, uint cellTrianglePairsSize)
{

    SortIndices resultSortIndices;
    uint read2BitsFromHere_U = (uint) read2BitsFromHere;
    resultSortIndices.array[0] = 0;
    resultSortIndices.array[1] = 0;
    resultSortIndices.array[2] = 0;
    resultSortIndices.array[3] = 0;

    if (id < cellTrianglePairsSize)
    {
        CellTrianglePair cellTrianglePair = cellTrianglePairs[id];
        uint bit0 = (cellTrianglePair.cellID >> read2BitsFromHere_U) & 1;
        uint bit1 = (cellTrianglePair.cellID >> read2BitsFromHere_U + 1) & 1;
        if (bit1 == 0 && bit0 == 0)
            resultSortIndices.array[0] = 1;
        else if(bit1 == 0 && bit0 == 1)
            resultSortIndices.array[1] = 1;
        else if (bit1 == 1 && bit0 == 0)
            resultSortIndices.array[2] = 1;
        else if (bit1 == 1 && bit0 == 1)
            resultSortIndices.array[3] = 1;
    }
    return resultSortIndices;
}


[numthreads(_8_1_RADIXSORT_EXCLUSIVEPREFIXSUM_XTHREADS, _8_1_RADIXSORT_EXCLUSIVEPREFIXSUM_YTHREADS, _8_1_RADIXSORT_EXCLUSIVEPREFIXSUM_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
    uint id = DTid.x;
    uint scaledGroupLocalID = GTid.x * 2;
    // Größe des Buffers ermitteln
    uint cellTrianglePairsLength, stride;
    cellTrianglePairs.GetDimensions(cellTrianglePairsLength, stride);
    uint sortIndicesLength;
    sortIndices.GetDimensions(sortIndicesLength, stride);

    uint threadDistance = startCombineDistance * 2;
    uint dataID = id * threadDistance + threadDistance - 1;
    if (read2BitsFromHere != -1)
    {
        groupSortIndices[scaledGroupLocalID] = getSortIndicesFromInput(id * 2, cellTrianglePairsLength);
        groupSortIndices[scaledGroupLocalID + 1] = getSortIndicesFromInput(id * 2 + 1, cellTrianglePairsLength);
    }
    else
    {
        SortIndices emptySortIndices;
        emptySortIndices.array[0] = 0;
        emptySortIndices.array[1] = 0;
        emptySortIndices.array[2] = 0;
        emptySortIndices.array[3] = 0;
        if (dataID > cellTrianglePairsLength)
            groupSortIndices[scaledGroupLocalID + 1] = emptySortIndices;
        else
            groupSortIndices[scaledGroupLocalID + 1] = sortIndices[dataID];
        if (dataID > cellTrianglePairsLength - startCombineDistance)
            groupSortIndices[scaledGroupLocalID] = emptySortIndices;
        else
            groupSortIndices[scaledGroupLocalID] = sortIndices[dataID - startCombineDistance];
    }
    GroupMemoryBarrierWithGroupSync();    
    uint curCombineDistance = 1;
    for (uint j = 0; j < loops; j++)
    {
        if (dataID < sortIndicesLength)
        {
            if (scaledGroupLocalID % (curCombineDistance * 2) == 0) // curCombineDistance * 2 weil die groupID auch gescaled wurde
            {
                uint curID = scaledGroupLocalID + curCombineDistance * 2 - 1;
                for (uint k = 0; k < 4; k++)
                {
                    // auseinanderfriemeln und debuggen!
                    groupSortIndices[curID].array[k] += groupSortIndices[curID - curCombineDistance].array[k];
                }
                curCombineDistance *= 2;
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }
    if (dataID < sortIndicesLength)
    {
        sortIndices[dataID] = groupSortIndices[scaledGroupLocalID + 1];
        sortIndices[dataID - startCombineDistance] = groupSortIndices[scaledGroupLocalID];
    }
}
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

[numthreads(_8_1_RADIXSORT_EXCLUSIVEPREFIXSUM_XTHREADS, _8_1_RADIXSORT_EXCLUSIVEPREFIXSUM_YTHREADS, _8_1_RADIXSORT_EXCLUSIVEPREFIXSUM_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
    uint id = DTid.x;
    uint scaledGroupLocalID = GTid.x * 2;
    // Größe des Buffers ermitteln
    uint cellTrianglePairsLength, stride, sortIndicesLength;
    cellTrianglePairs.GetDimensions(cellTrianglePairsLength, stride);
    sortIndices.GetDimensions(sortIndicesLength, stride);
    sortIndicesLength -= 1; // die letzte Stelle wird seperat befüllt

    uint threadDistance = startCombineDistance * 2;
    uint dataID = id * threadDistance + threadDistance - 1; // falsch gescaled?
    if (read2BitsFromHere != -1)
    {
        groupSortIndices[scaledGroupLocalID] = getSortIndicesFromInput(id * 2, cellTrianglePairsLength, (uint)read2BitsFromHere, cellTrianglePairs);
        groupSortIndices[scaledGroupLocalID + 1] = getSortIndicesFromInput(id * 2 + 1, cellTrianglePairsLength, (uint) read2BitsFromHere, cellTrianglePairs);
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
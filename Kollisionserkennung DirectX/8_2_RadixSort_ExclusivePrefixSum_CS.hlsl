#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<SortIndices> sortIndices : register(u0);

groupshared SortIndices groupSortIndices[2048];

cbuffer radixSort_ExclusivePrefixSum_2_Data : register(b0)
{
    uint bool_firstStep;
    uint threadDistance;
    uint loops;
}

[numthreads(_8_1_RADIXSORT_EXCLUSIVEPREFIXSUM_XTHREADS, _8_1_RADIXSORT_EXCLUSIVEPREFIXSUM_YTHREADS, _8_1_RADIXSORT_EXCLUSIVEPREFIXSUM_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
    uint id = DTid.x;
    uint scaledGroupLocalID = GTid.x * 2;

    uint sortIndicesLength, stride;
    sortIndices.GetDimensions(sortIndicesLength, stride);

    // kopiere zunächst Daten in den groupShared-Memory, Achtung, hat nichts mit den Daten zu tun die später von diesem Thread bearbeitet werden! 
    uint dataID = id * threadDistance + threadDistance - 1;
    if (dataID < sortIndicesLength)
    {
        if (bool_firstStep == 1 && dataID == sortIndicesLength - 1) // bei Phase 2 muss das Element ganz rechts 0 auf 0 gesetzt werden
        {
            groupSortIndices[scaledGroupLocalID + 1].array[0] = 0;
            groupSortIndices[scaledGroupLocalID + 1].array[1] = 0;
            groupSortIndices[scaledGroupLocalID + 1].array[2] = 0;
            groupSortIndices[scaledGroupLocalID + 1].array[3] = 0;
        }
        else
            groupSortIndices[scaledGroupLocalID + 1] = sortIndices[dataID];
        }
    if ((dataID - threadDistance / 2) < sortIndicesLength)
    {
        groupSortIndices[scaledGroupLocalID] = sortIndices[dataID - threadDistance / 2];

    }
    GroupMemoryBarrierWithGroupSync();

    uint curCombineDistance = pow(2, loops) / 2; // + 1, weil uns die Größe der am Ende bearbeiten Elemente interessiert und der letzte Thread schreibt in 2 Elemente
    uint threadAliveNumber = curCombineDistance * 2;
    for (uint i = 11; i > 11 - loops; i--)
    {
        if (dataID < sortIndicesLength)
        {
            if (scaledGroupLocalID % threadAliveNumber == 0)
            {
                uint curID = scaledGroupLocalID + curCombineDistance * 2 - 1;
                for (uint k = 0; k < 4; k++)
                {
                    uint copyValue = groupSortIndices[curID].array[k];

                    uint value1 = groupSortIndices[curID].array[k];
                    uint value2 = groupSortIndices[curID - curCombineDistance].array[k];
                    groupSortIndices[curID].array[k] = value1 + value2;
                    //groupSortIndices[curID].array[k] += groupSortIndices[curID - curCombineDistance];
                    groupSortIndices[curID - curCombineDistance].array[k] = copyValue;
                }
            }
        }
        GroupMemoryBarrierWithGroupSync();
        curCombineDistance /= 2;
        threadAliveNumber /= 2;
    }

    if (dataID < sortIndicesLength)
    {
        sortIndices[ dataID] = groupSortIndices[scaledGroupLocalID + 1];
        sortIndices[dataID - threadDistance / 2] = groupSortIndices[scaledGroupLocalID];

    }

}
#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<SortIndices> sortIndices : register(u0);
RWStructuredBuffer<CellTrianglePair> cellTrianglePairs : register(u1);


groupshared SortIndices groupSortIndices[2048];

cbuffer radixSort_ExclusivePrefixSum_2_Data : register(b0)
{
    uint bool_firstStep;
    uint threadDistance;
    uint loops;
    uint read2BitsFromHere;
}

[numthreads(LINEAR_XTHREADS, LINEAR_YTHREADS, LINEAR_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
    uint id = DTid.x;
    uint scaledGroupLocalID = GTid.x * 2;

    uint sortIndicesLength, stride;
    sortIndices.GetDimensions(sortIndicesLength, stride);
    sortIndicesLength -= 1; // die letzte Stelle wird seperat befüllt, der Buffer ist für den ALgorithmus eins zu groß

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
        sortIndices[dataID] = groupSortIndices[scaledGroupLocalID + 1];
        sortIndices[dataID - threadDistance / 2] = groupSortIndices[scaledGroupLocalID];
    }

    if (dataID == sortIndicesLength - 1) // sollte dieser Thread für das Kopieren des regulär letzten Elements des Buffers zuständig sein
    {
        uint cellTrianglePairsLength;
        cellTrianglePairs.GetDimensions(cellTrianglePairsLength, stride);
        // hole die Bitwerte aus der letzten regulären Stelle des cellTrianglePairBuffers, um zu ermitteln, welchen Wert der GesamtWert an der letzten Stelle hat
        SortIndices lastSortIndices = getSortIndicesFromInput(sortIndicesLength - 1, cellTrianglePairsLength, read2BitsFromHere, cellTrianglePairs);
        for (uint l = 0; l < 4; l++)
        {
            if (lastSortIndices.array[l] == 1) // solte die letzte reguläre Stelle eine 1 beinhalten, muss der Gesamtwert der exklusive Prefix Sum,
                                               // der an der letzten Stelle steht eins höher sein als der Wert in der letzten regulären Stelle
                sortIndices[sortIndicesLength].array[l] = sortIndices[sortIndicesLength - 1].array[l] + 1;
            else // ansonsten ist es der selbe Wert
                sortIndices[sortIndicesLength].array[l] = sortIndices[sortIndicesLength - 1].array[l];
        }

    }
}
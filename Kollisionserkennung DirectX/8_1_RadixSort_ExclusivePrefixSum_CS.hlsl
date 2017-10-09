#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<CellTrianglePair> cellTrianglePairs : register(u0);
RWStructuredBuffer<SortIndices> sortIndices : register(u1);

groupshared SortIndices groupSortIndices[2048];

cbuffer radixSort_ExclusivePrefixSum_Data : register(b0)
{
    uint loops; // wie viele for-Durchl�ufe hat dieser Shader
    int read2BitsFromHere; // -1 falls die Bits schon ausgelesen wurden, ansonsten: an dieser Position werden 2 Bits genutzt um die SortIndices initial zu bef�llen
    uint startCombineDistance;
}

// sollten die SortIndices noch nicht bef�llt sein, lese die Bits aus cellTrianglePairs.cellID aus und schreibe die Werte in sortIndices
// ansonsten f�hre den einen Teilbereich des ersten Teils der exclusive Prefix Sum aus (oder den gesamten, wenn der Input klein genug ist und nicht
// �ber Dispatches gesynct werden muss
[numthreads(LINEAR_XTHREADS, LINEAR_YTHREADS, LINEAR_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
    uint id = DTid.x;
    uint scaledGroupLocalID = GTid.x * 2;
    // Gr��e des Buffers ermitteln
    uint cellTrianglePairsLength, stride, sortIndicesLength;
    cellTrianglePairs.GetDimensions(cellTrianglePairsLength, stride);
    sortIndices.GetDimensions(sortIndicesLength, stride);
    sortIndicesLength -= 1; // die letzte Stelle wird seperat bef�llt

    uint threadDistance = startCombineDistance * 2; // wie weit sind 2 Threads bezogen auf den Input, den sie auslesen auseinander
    uint dataID = id * threadDistance + threadDistance - 1; // berechne aus id und threadDistance die ID, mit der in diesem Thread auf die Daten zugeriffen wird
    if (read2BitsFromHere != -1) // wenn in read2BitsFromHere ein anderer Wert als -1 steht, muss SortIndices inital bef�llt werden
    {
        // das Ergebnis vom initialen Hochz�hlen kommt direkt in den groupshared Memory, um danach den ersten Teil der exclusive Prefix Sum im groupsharedMemory durchzuf�hren
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
        // sollten keine validen Daten an der Stelle sein, wird ein leeres Array in groupSortIndices geschrieben
        if (dataID > cellTrianglePairsLength)
            groupSortIndices[scaledGroupLocalID + 1] = emptySortIndices;
        else // ansonsten lese mit der schon berechneten dataID das zu diesem Thread geh�rende SortIndices-Array aus und kopiere es in den groupshared Memory
            groupSortIndices[scaledGroupLocalID + 1] = sortIndices[dataID];
        // jeder Thread liest zwei Werte ein, der zweite ist an der Stelle dataID - startCombineDistance (gehe die Kombinier-Distanz zur�ck, das ist das andere Element)
        if (dataID > cellTrianglePairsLength - startCombineDistance)
            groupSortIndices[scaledGroupLocalID] = emptySortIndices;
        else
            groupSortIndices[scaledGroupLocalID] = sortIndices[dataID - startCombineDistance];
    }
    GroupMemoryBarrierWithGroupSync(); // nach jeder Manipulation des groupshared Memory muss gesynct werden
    uint curCombineDistance = 1;
    // die erste H�lfte der exclusive Prefix Sum ist ein naiver Reduce, der loops Durchl�ufe hat (danach muss �ber einen Dispatch gesynct werden)
    for (uint j = 0; j < loops; j++)
    {
        // es k�nnen mehr Threads gespawnt werden (durch die 1024 Threads/Gruppe) als die exclusive Prefix Sum lang ist, also bearbeite nur valide Elemente
        if (dataID < sortIndicesLength) 
        {
            // sortiere die Threads aus, die in diesem Durchlauf nicht arbeiten
            if (scaledGroupLocalID % (curCombineDistance * 2) == 0) // curCombineDistance * 2 weil die groupID auch gescaled wurde
            {
                uint curID = scaledGroupLocalID + curCombineDistance * 2 - 1; // berechne die ID, mit der auf den groupshared memory zugeriffen wird
                for (uint k = 0; k < 4; k++) // bearbeite alle 4 Array-Eintr�ge (es ist ein 2Bit-Pass, also 4 Eintr�ge)
                {
                    // es werden immer zwei Elemente addiert und an der hinteren Stelle abgespeichert
                    groupSortIndices[curID].array[k] += groupSortIndices[curID - curCombineDistance].array[k];
                }
                curCombineDistance *= 2; // die Kombinierdistanz ist beim n�chsten Durchlauf doppelt so gro�
            }
        }
        GroupMemoryBarrierWithGroupSync(); // synce am Ende von jedem for-Durchlauf
    }
    // am Ende wird der groupsharedMemory wieder zur�ckopiert in den Buffer an die Stellen, aus denen an Anfang ausgelesen wurde
    if (dataID < sortIndicesLength)
    {
        sortIndices[dataID] = groupSortIndices[scaledGroupLocalID + 1];
        sortIndices[dataID - startCombineDistance] = groupSortIndices[scaledGroupLocalID];
    }
}
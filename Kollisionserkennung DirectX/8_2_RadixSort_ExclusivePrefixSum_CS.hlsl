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

// f�hre den zweiten Teil der exkclusive Prefix Sum durch
[numthreads(LINEAR_XTHREADS, LINEAR_YTHREADS, LINEAR_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
    uint id = DTid.x;
    uint scaledGroupLocalID = GTid.x * 2;

    uint sortIndicesLength, stride;
    sortIndices.GetDimensions(sortIndicesLength, stride);
    sortIndicesLength -= 1; // die letzte Stelle wird seperat bef�llt, der Buffer ist f�r den ALgorithmus eins zu gro�

    // kopiere zun�chst Daten in den groupShared-Memory, Achtung, hat nichts mit den Daten zu tun die sp�ter von diesem Thread bearbeitet werden! 
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
        else // ansonsten kopiere den Input aus sortIndices in den groupshared Memory
            groupSortIndices[scaledGroupLocalID + 1] = sortIndices[dataID];
    }
    // wie in erstem Teil der excklusive Prefix Sum werden auch hier im zweiten Teil von jedem Thread 2 Werte in den groupshared Memory kopiert
    if ((dataID - threadDistance / 2) < sortIndicesLength)
    {
        groupSortIndices[scaledGroupLocalID] = sortIndices[dataID - threadDistance / 2];
    }
    GroupMemoryBarrierWithGroupSync(); // synce, nachdem der groupshared Memory inital bef�llt wurde

    // der zweite Teil der exclusive Prefix Sum f�ngt bei einer hohen Kombinier-Distanz an und halbiert diese in jedem for-Durchlauf
    uint curCombineDistance = pow(2, loops) / 2; 
    uint threadAliveNumber = curCombineDistance * 2; // am Anfang arbeiten nur wenige Threads, mit jedem Durchlauf dopplet so viele
    for (uint i = 11; i > 11 - loops; i--) // das herunterz�hlende i symbolisiert das r�ckw�rts abarbeiten gegen�ber zu Teil 1
    {
        if (dataID < sortIndicesLength)
        {
            // sortiere die Threads aus, die in diesem Durchlauf nicht arbeiten
            if (scaledGroupLocalID % threadAliveNumber == 0) 
            {
                uint curID = scaledGroupLocalID + curCombineDistance * 2 - 1; // berechne die ID, mit der auf den groupshared memory zugeriffen wird
                for (uint k = 0; k < 4; k++) // bearbeite alle 4 Array-Eintr�ge (es ist ein 2Bit-Pass, also 4 Eintr�ge)
                {
                    uint copyValue = groupSortIndices[curID].array[k]; // merke den rechten Wert in copyValue

                    uint value2 = groupSortIndices[curID - curCombineDistance].array[k]; // value2 ist der linke Wert
                    groupSortIndices[curID].array[k] = copyValue + value2; // �berschreibe den rechten Wert mit der Addition
                    groupSortIndices[curID - curCombineDistance].array[k] = copyValue; // �berschreibe den linken Wert mit dem vorher gemerkten rechten Wert
                }
            }
        }
        GroupMemoryBarrierWithGroupSync(); // synce am Ende von jedem for-Durchlauf
        curCombineDistance /= 2; // die Kombinier-Distanz wird halb so gro�
        threadAliveNumber /= 2; // doppelt so viele Threads werden durch die halbierte threadAliveNumber im n�chsten Durchlauf laufen
    }

    // am Ende wird der groupsharedMemory wieder zur�ckopiert in den Buffer an die Stellen, aus denen an Anfang ausgelesen wurde
    if (dataID < sortIndicesLength)
    {
        sortIndices[dataID] = groupSortIndices[scaledGroupLocalID + 1];
        sortIndices[dataID - threadDistance / 2] = groupSortIndices[scaledGroupLocalID];
    }

    // sortIndices hat einen zus�tzlichen Wert am Ende, der relevant wird falls das letzte Element in sortIndices initial eine 1 war, die w�rde auf die Gesamtsumme nicht aufaddiert
    // daf�r ist die zus�tzliche Stelle am Ende da, um den richtigen Wert der Gesamtsumme zu beinhalten
    if (dataID == sortIndicesLength - 1) // sollte dieser Thread f�r das Kopieren des regul�r letzten Elements des Buffers zust�ndig sein
    {
        uint cellTrianglePairsLength;
        cellTrianglePairs.GetDimensions(cellTrianglePairsLength, stride);
        // hole die Bitwerte aus der letzten regul�ren Stelle des cellTrianglePairBuffers, um zu ermitteln, welchen Wert der GesamtWert an der letzten Stelle hat
        SortIndices lastSortIndices = getSortIndicesFromInput(sortIndicesLength - 1, cellTrianglePairsLength, read2BitsFromHere, cellTrianglePairs);
        for (uint l = 0; l < 4; l++)
        {
            if (lastSortIndices.array[l] == 1) // solte die letzte regul�re Stelle eine 1 beinhalten, muss der Gesamtwert der exklusive Prefix Sum,
                                               // der an der letzten Stelle steht eins h�her sein als der Wert in der letzten regul�ren Stelle
                sortIndices[sortIndicesLength].array[l] = sortIndices[sortIndicesLength - 1].array[l] + 1;
            else // ansonsten ist es der selbe Wert
                sortIndices[sortIndicesLength].array[l] = sortIndices[sortIndicesLength - 1].array[l];
        }
    }
}
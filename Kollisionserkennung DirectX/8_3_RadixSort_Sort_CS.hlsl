#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<SortIndices> sortIndices : register(u0);
RWStructuredBuffer<CellTrianglePair> cellTrianglePairsInput : register(u1);
RWStructuredBuffer<CellTrianglePair> cellTrianglePairsOutput : register(u2);

// nach dem die exclusive Prefix Sum f�r den RadixSort auf 2 Bits ausgef�hrt wurde, stehen in SortIndices die 
// 4 Ids, an deren Stelle die zu sortierenden Elemente (cellTrianglePairs) sortiert werden
// in der letzten Stelle von SortIndices stehen die Gesamt-Summen, die auf die IDs des n�chsten Bits aufaddiert werden m�ssen
[numthreads(LINEAR_XTHREADS, LINEAR_YTHREADS, LINEAR_ZTHREADS)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint id = DTid.x;

    uint cellTrianglePairsLength, stride, sortIndicesLength;
    cellTrianglePairsInput.GetDimensions(cellTrianglePairsLength, stride);

    if(id >= cellTrianglePairsLength)
        return;

    sortIndices.GetDimensions(sortIndicesLength, stride);

    SortIndices curSortIndex = sortIndices[id]; // hole den aktuellen Index aus sortIndices
    SortIndices nextSortIndex = sortIndices[id + 1]; // der n�chste Index wird gebraucht, um zu ermitteln ob das aktuelle Element bewegt werden soll
    uint curOffset = 0;
    for (int i = 0; i < 4; i++) // gehe �ber alle Eintr�ge f�r die 2 Bits
    {
        if (i > 0) // f�e alle bits au�er dem 0ten muss der Offset der vorherigen Bits beim Sortieren aufaddiert werden
            curOffset += sortIndices[sortIndicesLength - 1].array[i - 1]; // eh�he den Offset um die Gesamtsumme des vorherigen EIntrags im SortIndices-Array
        // sortiere den aktuellen Wert nur, wenn der Index im aktuell bearbeiteten Array-Eintrag sich vom gleichen Array-Eintrag im n�chsten SOrtIndices-Eintrag unterscheidet
        if (curSortIndex.array[i] != nextSortIndex.array[i]) 
        {
            // nimm den Wert der exclusive Prefix Sum f�r den aktuell bearbeiteten bin�ren Wert und addiere, falls es nicht der 0te ist, den Offset vom letzten bin�ren Wert
            // schreibe den aktuell bearbeiteten zu sortierenden Wert an diese ermittelte Stelle im Outputbuffer
            cellTrianglePairsOutput[curSortIndex.array[i] + curOffset] = cellTrianglePairsInput[id];
            // der Outputbuffer enth�lt nun die sortierten Werte des Inputbuffers
        }
    }
}



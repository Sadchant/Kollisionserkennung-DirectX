#include "HlslSharedDefines.h"

#define MAXCOLLIDINGOBJECTS 10

RWStructuredBuffer<uint> counterTrees : register(u0); // Input, allerdings wird dieser Shader benutzt, 
RWStructuredBuffer<uint> globalCounterTree : register(u1); // nur von Stelle 1 lesen, da steht der MaxPoint der Szene!

cbuffer fillCounterTreesData : register(b0)
{
    uint4 objectCount;
    uint4 treeSizeInLevel[SUBDIVS + 1]; // uint4, da in Constant Buffers ein Array-Eintrag immer 16 Byte hat, lese also nur von x! 
    // (k�nnte man auch geschickter l�sen, aber an der Stelle lieber dass bisschen Speicher verschwenden als zus�tzliche Instruktionen
    // zum uin4 auseinanderbauen auszuf�hren)
};

[numthreads(D_GLOBALCOUNTERTREE_XTHREADS, D_GLOBALCOUNTERTREE_YTHREADS, D_GLOBALCOUNTERTREE_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint id = DTid.x;
    // obligatorische �berpr�fung f�r den Block, der "zu wenig" zu tun hat
    if (id >= treeSizeInLevel[SUBDIVS].x)
        return;
    uint objectsInThisCellCount = 0; // wie viele Objekte besitzen Dreiecke, die in der von diesem Thread bearbeiteten Zelle liegen?
    uint boundingBoxCounts[MAXCOLLIDINGOBJECTS]; // pro Objekt die Anzahl der Dreiecke, ohne 0er, hieraus wird der letztendliche intersectionTestCount berechnet

    // gehe �ber alle Countertrees (1 pro Objekt)
    for (uint i = 0; i < objectCount.x; i++)
    {
        // berechne die Id der Zelle in Countertrees, die in diesem Durchlauf verrechnet wird
        uint curID = id + i * treeSizeInLevel[SUBDIVS].x;
        // hole die Anzahl der Dreiecke in dieser Zelle
        uint curBoundingBoxCount = counterTrees[curID];
        // sollte es Dreiecke in dieser Zelle geben, merke dir die Anzahl in boundingBoxCounts
        // erh�he au�erdem objectsInThisCellCount um 1
        if (curBoundingBoxCount > 0)
            boundingBoxCounts[objectsInThisCellCount++] = curBoundingBoxCount;
        // setze die bearbeitete Zelle f�r den n�chsten Durchgang von FillCounterTrees wieder auf 0
        counterTrees[curID] = 0;
    }
    // berechne nun aus der Dreiecks-Anzahl von jedem Objekt die insgesamt ben�tigten �berschneidungstests
    uint intersectionTestCount = 0;
    // laufe �ber alle Dreiecks-Anzahl-Werte
    for (uint j = 0; j < objectsInThisCellCount; j++)
    {
        // erh�he intersectionTestCount mit den Zahlen f�r alle Dreiecks-Anzahl-Werte, die nach dem jten Wert in objectsInThisCellCount stehen
        for (uint k = j + 1; k < objectsInThisCellCount; k++)
        {
            intersectionTestCount += boundingBoxCounts[j] * boundingBoxCounts[k];
        }
    }
    // speichere das Ergebnis im Output-Array
    globalCounterTree[id] = intersectionTestCount;
}
#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<uint> globalCounterTree : register(u0);
RWStructuredBuffer<uint> typeTree : register(u1);
cbuffer treeSizeInLevel : register(b0)
{ // nur die erste Stelle von den uint4 lesen!
    uint4 treeSizeInLevel[SUBDIVS + 1];
};
cbuffer StartLevel : register(b1)
{
    uint startLevel; // bei welchem Level startet die for-Schleife?
};


// dieses Mal werden 3D-Ids genutzt, da das Problem mit 3D-IDs wesentlich leichter l�sbar ist
[numthreads(_5_FILLTYPETREE_XTHREADS, _5_FILLTYPETREE_YTHREADS, _5_FILLTYPETREE_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID)
{
     // es gibt 512 Threads pro Gruppe, pro for-Durchlauf wird das Problem um 8 kleiner, es gibt also maximal 4 for-Durchl�ufe
    int forStartNumber; // die Zahl, ab welcher in der for-Schleife bis einschlie�lich 0 heruntergez�hlt wird
    if (startLevel >= 3) // forStartNumber ist also 3, bei einem startLevel gr��er als 3, da die for-Scheife auf jeden Fall viermal l�uft
        forStartNumber = 3;
    else // ansonsten f�ngt die for-Schleife bei einer kleineren Zahl an und hat dementsprechend weniger Durchl�ufe
        forStartNumber = startLevel;
    // forStartNumber ist wichtig, da sie bestimmt welche Zahlen i annimmt und durch i die threadAliveNumber bestimmt wird, die entscheidet, welche Threads
    // laufen und �ber die die 3D-IDs im Tree berechnet werden
    uint curLevel = startLevel; // curLevel wird pro for-Durchlauf um 1 erniedrigt und dient dazu, den Offset und die Resolution f�r die 1D-ID im Tree zu berechnen
    for (int i = forStartNumber; i >= 0; i--) // wichtig: bei 0 gibt es einen weiter Durchlauf, deswegen ist i auch kein uint
    {
        uint threadAliveNumber = pow(2, 3 - i); // gibt an, welche Threads im aktuellen for-Durchlauf noch aktiv sind und wird zur Umrechnung ThreadID->TreeID benutzt
        // bei i=3: threadAliveNumber = 1 im ersten Durchlauf, 2 im zweiten, 4 im Dritten, 8 im Vierten, bei kleinerem Start-i f�ngt die Reihe weiter hinten an, zB bei 2
        // durch das % threadAlive-Number wird sichergestellt, dass wenn nicht alle Threads etwas zu tun haben, nur die unteren linken der letzten Aufl�sungsstufe weiterarbeiten
        if ((DTid.x % threadAliveNumber == 0) &&
            (DTid.y % threadAliveNumber == 0) &&
            (DTid.z % threadAliveNumber == 0)) // es m�ssen alle drei Dimensionen gepr�ft werden
        {
            uint3 curParent3DID = DTid / threadAliveNumber; // die 3D-ID der Elternzelle, die aktuell bearbeitet wird
            uint curParentRes = pow(2, curLevel); // die Aufl�sung des Levels, in der der die aktuell bearbeitete Elternzelle liegt
            uint curOffset;
            if (curLevel == 0) // damit nicht auf treeSizeInLevel[-1] zugegriffen wird
                curOffset = 0;
            else // der Offset wird mit dem aktuellen Level aus treeSizeInLevel geholt (treeSizeInLevel[curLevel-1] beinhaltet die Gr��e des Baums bis zum aktuellen Level)
                curOffset = treeSizeInLevel[curLevel - 1].x;
            uint curParent1DID = get1DID(curParent3DID, curParentRes, curOffset); // mit der 1D-ID kann auf die Buffer zugegriffen werden, die linear aufgebaut sind
            uint curParentCount = globalCounterTree[curParent1DID]; // die Anzahl an �berschneidungsTests f�r die aktelle Zelle
            uint curChildsCount = 0; // die Anzahl an �berschneidungstests f�r die 8 Kind-Zellen
            uint3 bottomLeftChildID = curParent3DID * 2; // die ID der KindZelle- die r�umlich gesehen unten links vorne in der Elternzelle liegt
            uint curChildRes = curParentRes * 2; // die Aufl�sung des Levels, auf dem die Kind-Zellen liegen
            uint curChildOffset = treeSizeInLevel[curLevel].x; // der Offset f�r die Berechnung der 1D-IDs der Kind-Zellen
            uint curChilds1DIDs[8]; // merke dir die 1D-IDs der Kind-Zellen in einem Array, da die 1D-IDs sp�ter noch einmal ben�tigt werden und nicht doppelt berechnet werden sollen
            for (uint x = 0; x < 2; x++) // es gibt 2x2x2 Kindzellen
            {
                for (uint y = 0; y < 2; y++)
                {
                    for (uint z = 0; z < 2; z++) // laufe �ber alle Dimensionen und berechne die Vektoren, die ausgehend von der Zelle unten links alle Kindzellen abdecken
                    {
                        uint3 curChild3DID = bottomLeftChildID + uint3(x, y, z); // berechne die aktuelle Kind-3D-ID aus der ID unten links und dem aktuellen Richtungs-Vektor
                        uint curChild1DID = get1DID(curChild3DID, curChildRes, curChildOffset); // berechne die aktuelle Kind-1D-ID
                        curChilds1DIDs[x * 4 + y * 2 + z] = curChild1DID; // merke dir die aktuelle ID in curChilds1DIDs
                        curChildsCount += globalCounterTree[curChild1DID]; // z�hle alle Kind-Werte zusammen, das Ergebnis steht am Ende in curChildsCount
                    }
                }
            }
            if (curChildsCount < curParentCount) // wenn die aufaddierten Kind-Werte kleiner sind als der Eltern-Wert:
            {     
                globalCounterTree[curParent1DID] = curChildsCount; // im Countertree die Elternzelle mit dem geringeren Wert belegen
                
                typeTree[curParent1DID] = INTERNAL; // die Kindzellen bleiben bestehen, also ist die ELternzelle intern
                for (uint c = 0; c < 8; c++) // trage den vorher gesetzten Typ in die 8 Kindzellen ein
                {
                    if (typeTree[curChilds1DIDs[c]] != INTERNAL) // die Kind-Zellen werden als Blatt markiert, sofern sie nicht eigene Kinder haben
                        typeTree[curChilds1DIDs[c]] = LEAF;
                }
            }
            else // ansonsten, wenn die Elternzelle einen geringeren Wert an �berschneidungstests hat als die 8 Kindzellen zusammen:
            {
                typeTree[curParent1DID] = LEAF; // markiere die Eltern-Zelle als Blatt
                for (uint c = 0; c < 8; c++) // die Kindzellen sind leer, da der Baum bei ihrer Elternzelle endet
                {
                    typeTree[curChilds1DIDs[c]] = EMPTY;                    
                }
            }
        }
        // Wichtig: AllMemory, nicht Groupmemory, da im globalen Speicher operiert wird
        AllMemoryBarrierWithGroupSync(); // warte darauf, dass alle Gruppen in diesem Level fertig sind, da der n�chste Durchlauf Ergebnisse aus anderen Threads verarbeitet
        curLevel--; // im n�chsten Durchlauf wird das n�chsth�here Level bearbeitet (was eine niedrigere ID hat)
    } 
}
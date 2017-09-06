#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<uint> typeTree : register(u0);
RWStructuredBuffer<uint> leafIndexTree : register(u1);

cbuffer TreeSizeInLevels : register(b0)
{ // nur die erste Stelle von den uint4 lesen!
    uint4 treeSizeInLevels[SUBDIVS + 1];
};
cbuffer StartLevel : register(b1)
{
    uint startLevel; // bei welchem Level startet die for-Schleife?
};
cbuffer Loops : register(b2)
{
    uint loops; // Wie oft l�uft die for-Schleife?
};

// schreibt die 1D-ID der Zellen, die Bl�tter sind solange in deren Kindknoten in LeafIndexTree, bis das unterste Level erreicht ist
// am Ende stehen im untersten Level die IDs der Zellen, die an der Position im Raum die Blattzellen sind
// jeder Zweig hat also an seiner untersten Stelle die ID der in diesem Zweig als LEAF markierten Zelle <stehen
[numthreads(_6_FILLLEAFINDEXTREE_XTHREADS, _6_FILLLEAFINDEXTREE_YTHREADS, _6_FILLLEAFINDEXTREE_ZTHREADS)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint curLevel = startLevel; // setzte das aktuelle Level
    for (int i = 0; i <= loops; i++) // funktioniert �hnlich wie in 5_FillTypeTree_CS, aber l�uft diesmal von 0 - X und nicht von X - 0
    {
        uint threadAliveNumber = pow(2, 3 - i); // aus 5_FillTypeTree_CS bekannter Wert, der entscheidet welche Threads laufen und welche ID bearbeitet wird
        if ((DTid.x % threadAliveNumber == 0) &&
            (DTid.y % threadAliveNumber == 0) &&
            (DTid.z % threadAliveNumber == 0)) // es m�ssen alle drei Dimensionen gepr�ft werden, ob die aktuelle ID in diesem for-Durchlauf etwas zu tun bekommt
        {
            uint3 curParent3DID = DTid / threadAliveNumber; // die 3D-ID im Grid der aktuell bearbeiteten Zelle
            uint curParentRes = pow(2, curLevel); // die Aufl�sung des gerade bearbeiteten Levels
            uint curParentOffset = treeSizeInLevels[curLevel - 1]; // das Offset im Aktuell bearbeiteten Level um die 1D-ID zu berechnen
            uint curParent1DID = get1DID(curParent3DID.x, curParent3DID.y, curParent3DID.z, curParentRes, curParentOffset); //berechne die ID-ID
            uint curParentType = typeTree[curParent1DID]; // hole den Typ der Elternzelle
            uint curParentLeafIndex = leafIndexTree[curParent1DID];

            uint3 bottomLeftChildID = curParent3DID * 2; // die ID der KindZelle- die r�umlich gesehen unten links vorne in der Elternzelle liegt
            uint curChildsRes = curParentRes * 2; // die Aufl�sung ist im Kinderlevel doppelt so hoch
            uint curChildsOffset = treeSizeInLevels[curLevel]; // nehme den Offset-Wert einen gr��er als vom aktuellen Wert, um aufs Child-Offset zu kommen
            
            for (uint x = 0; x < 2; x++) // es gibt 2x2x2 Kindzellen
            {
                for (uint y = 0; y < 2; y++)
                {
                    for (uint z = 0; z < 2; z++) // laufe �ber alle Dimensionen und berechne die Vektoren, die ausgehend von der Zelle unten links alle Kindzellen abdecken
                    {
                        uint3 curChild3DID = bottomLeftChildID + uint3(x, y, z); // berechne die aktuelle Kind-3D-ID aus der ID unten links und dem aktuellen Richtungs-Vektor
                        uint curChild1DID = get1DID(curChild3DID.x, curChild3DID.y, curChild3DID.z, curChildsRes, curChildsOffset); // berechne die aktuelle Kind-1D-ID
                        if (curParentType == LEAF) // sollte die Elternzelle ein Blatt sein
                            leafIndexTree[curChild1DID] = curParent1DID; // trage im LeafIndexTree in alle Kindzellen den Index der ELtern-ID ein
                        else if (curParentType == EMPTY) // sollte die ELternzelle leer sein, muss es in einem h�hren Level schon ein Blatt gegeben haben
                            leafIndexTree[curChild1DID] = curParentLeafIndex; // als kopiere den Wert aus leafIndexTree der Elternzelle auf die Kindzellen
                        else if (curParentType) // ansonsten muss es eine interne Zelle sein, die Kinder k�nnen! also Bl�tter sein
                            leafIndexTree[curChild1DID] = curChild1DID; // trage jedes Mal die Child-IDs ein, sollten die Kinder auch intern sein ist es zwar �berfl�ssig, 
                                                                        // schadet aber auch nicht und es entsteht weniger Threaddivergenz, da die if-Abfrage weggelassen wird
                    }
                }
            }
        }
        AllMemoryBarrierWithGroupSync(); // warte darauf, dass alle Gruppen in diesem Level fertig sind, da der n�chste Durchlauf Ergebnisse aus anderen Threads verarbeitet
        curLevel++; // im n�chsten Durchlauf wird das n�chsth�here Level bearbeitet, was sich auf Anzahl der arbeitenden Threads und nat�rlich die bearbeiteten Zellen auswirkt
    }
}
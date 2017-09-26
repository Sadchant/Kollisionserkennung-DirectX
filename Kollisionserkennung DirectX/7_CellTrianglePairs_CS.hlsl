#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<BoundingBox> boundingBoxes : register(u0);
RWStructuredBuffer<float3> sceneMinPoints : register(u1); // an Stelle 0 steht der kleinste Punkt der Szene
RWStructuredBuffer<float3> sceneMaxPoints : register(u2); // an Stelle 0 steht der gr��te Punkt der Szene
RWStructuredBuffer<uint> globalCounterTree : register(u3);
RWStructuredBuffer<uint> leafIndexTree : register(u4);

RWStructuredBuffer<CellTrianglePair> cellTrianglePairs : register(u5);

StructuredBuffer<uint> objectsLastIndices : register(t0);

cbuffer ObjectCount : register(b0)
{
    uint objectCount;
};
cbuffer TreeSizeInLevel : register(b1)
{
    uint4 treeSizeInLevel[SUBDIVS + 1]; // lese also nur von x! 
};

// Gehe f�r jede Bounding Box wie in _3_FillCounterTrees �ber alle Zellen, die von ihr �berlappt werden, diesmal aber
// nur im untersten Level. Dann werden Cell-Triangle-Pairs mit Hilfe des LeafIndex-Trees (also die CellIDs der optimierten 
// Struktur) erzeugt und im Cell-Triangle-AppendBuffer hinzugef�gt. Damit keine doppelten Eintr�ge gemacht werden
// (was passiert wenn eine Bounding Box viele Zellen auf dem untersten Level �berdeckt, die alle zum selben Blatt geh�ren
// und im leafIndexTree die gleiche Id haben), �berpr�fe, wenn die aktuell bearbeitete Zelle nicht am jeweiligen Rand liegt,
// ob die letzte Zelle in x-, y-, oder z- Richtung die selbe ID hat. Wenn ja wurde sie schon bearbeitet und der Eintrag 
// existiert schon.
[numthreads(LINEAR_XTHREADS, LINEAR_YTHREADS, LINEAR_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint id = DTid.x;
    // obligatorische �berpr�fung f�r den Block, der "zu wenig" zu tun hat
    uint bufferSize, stride;
    boundingBoxes.GetDimensions(bufferSize, stride);
    if (id >= bufferSize)
        return;

    // ### bei gro�er Objektanzahl sollte dieses Vorgehen noch optimiert werden!
    // Suche das Objekt, das gerade von diesem Thread bearbeitet wird
    uint objectID;
    // die for-Schleife l�uft �ber die Liste aller letzten Indices der Objekte    
    for (uint i = 0; i < objectCount; i++)
    {
        // wenn die ID gr��er ist als ein lastIndex eines Objektes, kann die ID nicht innerhalb
        // des Objektes liegen, sobald die ID also kleiner ist als ein lastIndex, haben wir mit i,
        // welches ja die Objekte durchnummeriert die aktuelle Objekt-ID gefunden
        if (id <= objectsLastIndices[i])
        {
            objectID = i;
            break; // sobald die ID gefunden wurde brauchen wir nicht weitersuchen
        }
    }

    // hole die sceneBoundingBox aus den beiden Ergebnisbuffern vom letzten Shader, wo jeweils der erste Eintrag Minimum, bzw Maximum sind
    BoundingBox sceneBoundingBox = { sceneMinPoints[0] - 0.00001, sceneMaxPoints[0] + 0.00001 }; // vergr��ere die Bounding Box minimal, um float-Ungenauigkeiten bei Bearbeitung der Dreiecke, die die Bounding Box definieren zu vermeiden
    float3 sceneBoundingBoxVolumeVector = sceneBoundingBox.maxPoint - sceneBoundingBox.minPoint;
    // hole die Bounding Box aus dem Buffer, die in diesem Thread bearbeitet wird
    BoundingBox curBoundingBox = boundingBoxes[id];

    uint resolution = pow(2, SUBDIVS); // wie viele Gridzellen gibt es im Level pro Dimension, dass die for-Schleife gerade bearbeitet, 2: Aufl�sung pro Dimension, 8 w�re Anzahl der Zellen im 3D-Raum
    uint offset = treeSizeInLevel[SUBDIVS - 1].x;
    // curScale: um welchen Wert m�ssen Koordinaten in der Szene skaliert werden, damit bei der aktuellen Aufl�sung die Gridzellen in jeder Dimension auf
    // den Koordinaten (pro Dimension) 0, 1, 2, 3, 4 anfangen und nicht bei Kommazahlen oder Ganzzahlen, die weiter auseinander liegen als 1
    float3 scale = resolution / sceneBoundingBoxVolumeVector;
    uint3 curBBMinGridPosition; // die 3D-ID der Gridzelle, in der der MinPoint der aktuell bearbeiteten Bounding Box liegt 
    uint3 curBBMaxGridPosition; // die 3D-ID der Gridzelle, in der der MaxPoint der aktuell bearbeiteten Bounding Box liegt 
    // ermittle die Gridzellen, in denen die beiden Positionen der aktuell bearbeiteten Bonding Box liegen, indem die Positionen der BoundingBox
    // in das "normalisierte" (Gridzellen fangen bei 0, 1, 2, 3 an) Grid projezert wird
    // mit floor wird dann die Position, die ja dann ein float ist, in eine 3D-ID im Grid umgewandelt
    curBBMinGridPosition = (uint3) floor((curBoundingBox.minPoint - sceneBoundingBox.minPoint) * scale);
    curBBMaxGridPosition = (uint3) floor((curBoundingBox.maxPoint - sceneBoundingBox.minPoint) * scale);
    // gibt an, wie viele Gridzellen in jeder Dimension von der BoundingBox �berdeckt werden
    uint3 overlapRange = curBBMaxGridPosition - curBBMinGridPosition;
    // laufe �ber alle Gridzellen, die von curBoundingBox �berlappt werden und erh�he in Ihnen den trieangle-Count um 1
    // warum <=? Weil bei overlapRange = 0 trotzdem einmal in die for-Schleifen gegangen werden soll f�r die Zelle, in der der MinPoint der curBoundingBox liegt
    for (uint x = 0; x <= overlapRange.x; x++)
    {
        for (uint y = 0; y <= overlapRange.y; y++)
        {
            for (uint z = 0; z <= overlapRange.z; z++)
            {
                uint3 curOverlapOffset = { x, y, z };
                uint3 cur3DID = curBBMinGridPosition + curOverlapOffset; // rechne die 3D-Position im Grid an der aktuellen Overlap-Stelle aus
                uint cur1ID = get1DID(cur3DID, resolution, offset);
                uint curLeafID = leafIndexTree[cur1ID]; // merke dir die aktuelle Leaf-ID
                if (globalCounterTree[curLeafID] > 0)
                {
                    // gehe jeweils �ber die 3 Achsen x, y, z und pr�fe, ob das gefundene Grid-Triangle-Pair schon eingetragen wurde
                    bool isAlreadyListed = false;
                    // k�nnte man leichter verst�ndlich f�r y, y und z auch hintereinander schreiben, hier die "elegante" Variante in einer for-Schleife
                    [unroll] // da vector3[x] Compiler-Warnungen erzeugt, unrolle die for-Schleife
                    for (uint axis = 0; axis < 3; axis++) // gehe �ber die 3 Achsen x, y, z (jede hat in jeweils einem Durchlauf den Wert 1, die anderen sind 0) 
                    {
                        // checke das letzte Feld nur, wenn es auch innerhalb der overlap-Range ist (ist es ab dem Zeitpunkt, in x-Richtung, wenn x gr��er als 1 ist, in y-Richtung, wenn ... usw.
                        if ((axis == 0 && x > 0) || (axis == 1 && y > 0) || (axis == 2 && z > 0))
                        {
                            uint3 checkInThisDirectionVector = { 0, 0, 0 }; // jeweils einer wird im n�chsten Schritt auf 1 gesetzt
                            checkInThisDirectionVector[axis] = 1;
                            uint3 checkedCell3DID = cur3DID - checkInThisDirectionVector; // gehe ein Feld zur�ck in der jeweiligen Achse
                            uint checkedCell1DID = get1DID(checkedCell3DID, resolution, offset); // berechne die 1D-ID
                            uint checkedCellLeafID = leafIndexTree[checkedCell1DID]; // hole den LeafIndex
                            if (curLeafID == checkedCellLeafID) // sollte der Leaf-Index gleich sein, wurde das aktuelle Cell-Triangle-Pair schon eingetragen
                            {
                                isAlreadyListed = true; // merke, dass das aktuelle Grid-Triangle-Paar schon eingetragen wurde
                                break; // die anderen Dimensionen brauchen nicht mehr �berpr�ft zu werden
                            }
                        }
                    }
                    if (!isAlreadyListed) // trage nur eine neue Zelle ein, wenn sie noch nicht eingetragen wurde
                    {
                        CellTrianglePair newPair = { curLeafID, id, objectID }; // baue das Cell-Triangle-Paar zusammen (objectID ist sp�ter beim Kollisionen suchen wichtig!)
                        uint curCount = cellTrianglePairs.IncrementCounter();
                        cellTrianglePairs[curCount] = newPair; // die Paare kommen zun�chst ungeordnet in den CellTrianglePair-Buffer
                    }
                }
            }
        }
    }
}
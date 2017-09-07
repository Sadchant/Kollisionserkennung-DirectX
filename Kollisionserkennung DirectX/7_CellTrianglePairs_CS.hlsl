#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<BoundingBox> boundingBoxes : register(u0);
RWStructuredBuffer<float3> sceneMinPoints : register(u1);
RWStructuredBuffer<float3> sceneMaxPoints : register(u2);
RWStructuredBuffer<uint> globalCounterTree : register(u3);
RWStructuredBuffer<uint> leafIndexTree : register(u4);

AppendStructuredBuffer<CellTrianglePair> cellTrianglePairs : register(u5);

StructuredBuffer<uint> objectsLastIndices : register(t0);

cbuffer ObjectCount : register(b0)
{
    uint objectCount;
};
cbuffer TreeSizeInLevel : register(b1)
{
    uint4 treeSizeInLevel[SUBDIVS + 1]; // lese also nur von x! 
};


[numthreads(_7_CELLTRIANGLEPAIRS_XTHREADS, _7_CELLTRIANGLEPAIRS_YTHREADS, _7_CELLTRIANGLEPAIRS_ZTHREADS)]
void main( uint3 DTid : SV_DispatchThreadID )
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
                uint curLeafID = leafIndexTree[cur1ID];
                if (globalCounterTree[curLeafID] > 0)
                {
                    bool isAlreadyListed = false;
                    for (uint axis = 0; axis < 3; axis++)
                    {
                        if ((axis == 0 && x > 0) || (axis == 1 && y > 0) || (axis == 2 && z > 0))
                        {
                            uint3 checkInThisDirectionVector = { 0, 0, 0 };
                            checkInThisDirectionVector[axis] = 1;
                            uint3 checkedCell3DID = cur3DID - checkInThisDirectionVector;
                            uint checkedCell1DID = get1DID(checkedCell3DID, resolution, offset);
                            uint checkedCellLeafID = leafIndexTree[checkedCell1DID];
                            if (curLeafID == checkedCellLeafID)
                            {
                                isAlreadyListed = true;
                                break;
                            }
                        }
                    }
                    if (!isAlreadyListed)
                    {
                        CellTrianglePair newPair = { curLeafID, id, objectID };
                        cellTrianglePairs.Append(newPair);
                    }
                }
            }
        }
    }
}
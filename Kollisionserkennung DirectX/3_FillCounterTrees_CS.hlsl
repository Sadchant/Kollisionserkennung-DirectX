#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<float3> sceneMinPoints : register(u0); // nur von Stelle 0 lesen, da steht der MinPoint der Szene!
RWStructuredBuffer<float3> sceneMaxPoints : register(u1); // nur von Stelle 1 lesen, da steht der MaxPoint der Szene!

RWStructuredBuffer<BoundingBox> boundingBoxBuffer : register(u2);

RWStructuredBuffer<int> counterTrees : register(u3); // nur von Stelle 1 lesen, da steht der MaxPoint der Szene!

StructuredBuffer<int> objectLastIndices : register(t0);

cbuffer fillCounterTreesData : register(b0)
{
    int objectCount;
    uint treeSizeUntilLevel[LEVELS];
};

// berechne aus 3D-Koordinaten, der aktuellen Größe des Grids und dem Level-Offset die 1-dimensionale ID
int get1DID(int x, int y, int z, int resolution, int offset)
{
    return x + y * resolution + z * resolution * resolution + offset;
}

[numthreads(C_FILLCOUNTERTREES_XTHREADS, C_FILLCOUNTERTREES_YTHREADS, C_FILLCOUNTERTREES_ZTHREADS)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    int id = DTid.x;
    // obligatorische Überprüfung für den Block, der "zu wenig" zu tun hat
    int bufferSize, stride;
    boundingBoxBuffer.GetDimensions(bufferSize, stride);    
    if (id > bufferSize)
        return;

    // ### bei großer Objektanzahl sollte dieses Vorgehen noch optimiert werden!
    // Suche das Objekt, das gerade von diesem Thread bearbeitet wird
    int objectID;
    // die for-Schleife läuft über die Liste aller letzten Indices der Objekte    
    for (int i = 0; i < objectCount; i++)
    {
        // wenn die ID größer ist als ein lastIndex eines Objektes, kann die ID nicht innerhalb
        // des Objektes liegen, sobald die ID also kleiner ist als ein lastIndex, haben wir mit i,
        // welches ja die Objekte durchnummeriert die aktuelle Objekt-ID gefunden
        if (id < objectLastIndices[i])
        {
            objectID = i;
            return; // sobald die ID gefunden wurde brauchen wir nicht weitersuchen
        }
    }
    // das objectOffset gilt für counterTrees, ab welcher Position der Countertree vom Objekt mit objectID anfängt
    // in treeSizeUntilLevel[LEVELS] steht die Gesamtgröße eines Countertrees
    uint objectOffset = objectID * treeSizeUntilLevel[LEVELS];

    // hole die sceneBoundingBox aus den beiden Ergebnisbuffern vom letzten Shader, wo jeweils der erste Eintrag Minimum, bzw Maximum sind
    BoundingBox sceneBoundingBox = { sceneMinPoints[0], sceneMaxPoints[0] };
    float3 sceneBoundingBoxVolumeVector = sceneBoundingBox.maxPoint - sceneBoundingBox.minPoint;
    // hole die Bounding Box aus dem Buffer, die in diesem Thread bearbeitet wird
    BoundingBox curBoundingBox = boundingBoxBuffer[id];
    // so viele Gridzellen beinhaltet der Tree, wenn man nur das höchste Level betrachtet
    uint maxRes = (uint)pow(8, LEVELS);

    // iteriere über alle Level im Tree
    for (int level = 0; level <= LEVELS; level++)
    {
        uint curRes = pow(8, i); // wie viele Gridzellen gibt es im Level, dass die for-Schleife gerade bearbeitet
        // curScale: um welchen Wert müssen Koordinaten in der Szene skaliert werden, damit bei der aktuellen Auflösung die Gridzellen in jeder Dimension auf
        // den Koordinaten (pro Dimension) 0, 1, 2, 3, 4 anfangen und nicht bei Kommazahlen oder Ganzzahlen, die weiter auseinander liegen als 1
        float3 curScale = { curRes / sceneBoundingBoxVolumeVector.x, curRes / sceneBoundingBoxVolumeVector.y, curRes / sceneBoundingBoxVolumeVector.z };
        uint3 curBBMinGridPosition; // die 3D-ID der Gridzelle, in der der MinPoint der aktuell bearbeiteten Bounding Box liegt 
        uint3 curBBMaxGridPosition; // die 3D-ID der Gridzelle, in der der MaxPoint der aktuell bearbeiteten Bounding Box liegt 
        // gehe über alle Dimensionen
        for (int v = 0; v < 3; v++)
        {
            // ermittle die Gridzellen, in denen die beiden Positionen der aktuell bearbeiteten Bonding Box liegen, indem die Positionen der BoundingBox
            // in das "normalisierte" (Gridzellen fangen bei 0, 1, 2, 3 an) Grid projezert wird
            // mit floor wird dann die Position, die ja dann ein float ist, in eine 3D-ID im Grid umgewandelt
            curBBMinGridPosition[v] = (uint) floor((curBoundingBox.minPoint[v] - sceneBoundingBox.minPoint[v]) * curScale[v]);
            curBBMaxGridPosition[v] = (uint) floor((curBoundingBox.maxPoint[v] - sceneBoundingBox.minPoint[v]) * curScale[v]);
        }
        // gibt an, wie viele Gridzellen in jeder Dimension von der BoundingBox überdeckt werden
        uint3 overlapRange = curBBMaxGridPosition - curBBMinGridPosition;

        // laufe über alle Gridzellen, die von curBoundingBox überlappt werden und erhöhe in Ihnen den trieangle-Count um 1
        for (int x = 0; x < overlapRange.x; x++)
        {
            for (int y = 0; y < overlapRange.y; y++)
            {
                for (int z = 0; z < overlapRange.z; z++)
                {
                    int3 curOverlapOffset = { x, y, z };
                    int3 cur3DID = curBBMinGridPosition + curOverlapOffset; // rechne die 3D-Position im Grid an der aktuellen Overlap-Stelle aus
                    int cur1ID = get1DID(cur3DID.x, cur3DID.y, cur3DID.z, curRes, treeSizeUntilLevel[i - 1]);
                    // offset = treeSizeUntilLevel[i - 1], weil in treeSizeUntilLevel[i] auch die Größe des aktuellen Levels steht, die aber nicht zum offset gehört
                    // Zwei Threads sollten nicht gleichzeitig in eine Gridzelle schreiben, also erhöhe den Counter atomar
                    InterlockedAdd(counterTrees[objectOffset + cur1ID], 1);
                }
            }
        }
    }
}
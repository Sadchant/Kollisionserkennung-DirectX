#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

// K�nnte man hier auch als read-only einlesen, Buffer wurde an anderer Stelle aber schon als Write gesetzt, macht performancetechnisch keinen Unterschied!
RWStructuredBuffer<float3> sceneMinPoints : register(u0); // nur von Stelle 0 lesen, da steht der MinPoint der Szene!
RWStructuredBuffer<float3> sceneMaxPoints : register(u1); // nur von Stelle 1 lesen, da steht der MaxPoint der Szene!
RWStructuredBuffer<BoundingBox> boundingBoxBuffer : register(u2);
RWStructuredBuffer<uint> counterTrees : register(u3); // ist mit 0en initialisiert und wird atomar bef�llt

StructuredBuffer<uint> objectsLastIndices : register(t0);

cbuffer ObjectCount : register(b0)
{
    uint objectCount;
};
cbuffer TreeSizeInLevel : register(b1)
{
    uint4 treeSizeInLevel[SUBDIVS + 1]; // uint4, da in Constant Buffers ein Array-Eintrag immer 16 Byte hat, lese also nur von x! 
    // (k�nnte man auch geschickter l�sen, aber an der Stelle lieber dass bisschen Speicher verschwenden als zus�tzliche Instruktionen
    // zum uint4 auseinanderbauen auszuf�hren)
};

[numthreads(_3_FILLCOUNTERTREES_XTHREADS, _3_FILLCOUNTERTREES_YTHREADS, _3_FILLCOUNTERTREES_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint id = DTid.x;
    // obligatorische �berpr�fung f�r den Block, der "zu wenig" zu tun hat
    uint bufferSize, stride;
    boundingBoxBuffer.GetDimensions(bufferSize, stride);
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
    // das objectOffset gilt f�r counterTrees, ab welcher Position der Countertree vom Objekt mit objectID anf�ngt
    // in treeSizeInLevel[SUBDIVS].x steht die Gesamtgr��e eines Countertrees (treeSizeInLvel[SUBDIVS+1] ist outofBounds!)

    uint objectOffset = objectID * treeSizeInLevel[SUBDIVS].x;
    

    // hole die sceneBoundingBox aus den beiden Ergebnisbuffern vom letzten Shader, wo jeweils der erste Eintrag Minimum, bzw Maximum sind
    BoundingBox sceneBoundingBox = { sceneMinPoints[0]-0.00001, sceneMaxPoints[0]+0.00001 }; // vergr��ere die Bounding Box minimal, um float-Ungenauigkeiten bei Bearbeitung der Dreiecke, die die Bounding Box definieren zu vermeiden
    float3 sceneBoundingBoxVolumeVector = sceneBoundingBox.maxPoint - sceneBoundingBox.minPoint;
    // hole die Bounding Box aus dem Buffer, die in diesem Thread bearbeitet wird
    BoundingBox curBoundingBox = boundingBoxBuffer[id];
    // so viele Gridzellen beinhaltet der Tree, wenn man nur das h�chste Level betrachtet
    //uint maxRes = (uint)pow(8, SUBDIVS);
    //pow(2^x, y) = 1 << x * y

    // iteriere �ber alle Level im Tree, <= weil: Bei SUBDIVS = 1 gibt es eine Unterteilung, also zwei unterschiedliche Level
    for (int level = 1; level <= SUBDIVS; level++)
    {
        uint curRes = pow(2, level); // wie viele Gridzellen gibt es im Level pro Dimension, dass die for-Schleife gerade bearbeitet, 2: Aufl�sung pro Dimension, 8 w�re Anzahl der Zellen im 3D-Raum
        // curScale: um welchen Wert m�ssen Koordinaten in der Szene skaliert werden, damit bei der aktuellen Aufl�sung die Gridzellen in jeder Dimension auf
        // den Koordinaten (pro Dimension) 0, 1, 2, 3, 4 anfangen und nicht bei Kommazahlen oder Ganzzahlen, die weiter auseinander liegen als 1
        uint curOffset; // Wieviel Platz im 1D-Array wird duch die vorangegangenen Level belegt?
        if (level == 0)
            curOffset = 0;
        else
            curOffset = treeSizeInLevel[level - 1].x;
        float3 curScale = curRes / sceneBoundingBoxVolumeVector;
        uint3 curBBMinGridPosition; // die 3D-ID der Gridzelle, in der der MinPoint der aktuell bearbeiteten Bounding Box liegt 
        uint3 curBBMaxGridPosition; // die 3D-ID der Gridzelle, in der der MaxPoint der aktuell bearbeiteten Bounding Box liegt 
        // ermittle die Gridzellen, in denen die beiden Positionen der aktuell bearbeiteten Bonding Box liegen, indem die Positionen der BoundingBox
        // in das "normalisierte" (Gridzellen fangen bei 0, 1, 2, 3 an) Grid projezert wird
        // mit floor wird dann die Position, die ja dann ein float ist, in eine 3D-ID im Grid umgewandelt
        curBBMinGridPosition = (uint3) floor((curBoundingBox.minPoint - sceneBoundingBox.minPoint) * curScale);
        curBBMaxGridPosition = (uint3) floor((curBoundingBox.maxPoint - sceneBoundingBox.minPoint) * curScale);
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
                    uint cur1ID = get1DID(cur3DID, curRes, curOffset);
                    // offset = treeSizeInLevel[i - 1], weil in treeSizeInLevel[i] auch die Gr��e des aktuellen Levels steht, die aber nicht zum offset geh�rt
                    // Zwei Threads sollten nicht gleichzeitig in eine Gridzelle schreiben, also erh�he den Counter atomar
                    InterlockedAdd(counterTrees[objectOffset + cur1ID], 1);
                }
            }
        }
    }
}
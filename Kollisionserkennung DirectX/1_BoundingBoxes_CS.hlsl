#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"


StructuredBuffer<int3> triangleBuffer : register(t1);

RWStructuredBuffer<BoundingBox> boundingBoxBuffer : register(u0);


[numthreads(A_BOUNDINGBOXES_XTHREADS, A_BOUNDINGBOXES_YTHREADS, A_BOUNDINGBOXES_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // merke dir die ThreadId dieses Threads in id (.x, da eindimensionale Struktur)
    uint id = DTid.x;

    // Gr��e des Buffers ermitteln
    uint numStructs, stride;
    triangleBuffer.GetDimensions(numStructs, stride);

    // es werden immer Bl�cke mit 1024 Threads gestartet, die Threads, welche Dreiecke mit
    // IDs au�erhalb des Dreiecksbuffers bearbeiten wollen, machen stattdessen nichts
    if (id > numStructs)
        return;

    // hole das aktuelle Dreieck aus dem Buffer
    int3 _triangle = triangleBuffer[id];
    // initialisiere min und max mit dem ersten Punkt des Dreiecks
    float3 min, max; 
    min = max = vertexBuffer[_triangle[0]];

    // durchlaufe Punkte b und c des Dreiecks, um den jeweils kleinsten/gr��ten Punkt pro 
    // Dimension zu finden und in min/max zu speichern
    for (int i = 1; i <= 2; i++)
    {
        // hole dir einmal den aktuellen Vertex um nicht immer auf den Buffer zugreifen zu m�ssen
        float3 curVertex = vertexBuffer[_triangle[i]];
        // korrigiere den min-Vertex
        if (curVertex.x < min.x) 
            min.x = curVertex.x;
        if (curVertex.y < min.y)
            min.y = curVertex.y;
        if (curVertex.z < min.z)
            min.x = curVertex.x;

        // korrigiere max-Vertex
        if (curVertex.x > max.x) 
            max.x = curVertex.x;
        if (curVertex.y > max.y)
            max.y = curVertex.y;
        if (curVertex.z > max.z)
            max.x = curVertex.x;
    }

    // speichere das Ergebnis im BoundingBoxBuffer
    BoundingBox boundingBox = { min, max };
    boundingBoxBuffer[id] = boundingBox;
}
#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"
#include "TriangleIntersections_HLSL.hlsl"

// da die tats�chlichen Dreiecke miteinander �berpr�ft werden, werden die �brps�nglichen Index- und Vertex-Buffer ben�tigt
StructuredBuffer<float3> vertexBuffer : register(t0);
StructuredBuffer<int3> triangleBuffer : register(t1);

RWStructuredBuffer<TrianglePair> trianglePairs : register(u0); // jeder Thread bearbeitet ein Cell-Triangle-Pair
RWStructuredBuffer<uint> intersectingObjects : register(u1); // Output: speichere an der Stelle der Objekt-ID, ob sich das Objekt mit einem anderen schneidet
RWStructuredBuffer<float3> intersectCenters : register(u2); // Output: alle Punkte, die die �berschneidung von zwei Dreiecken grob angeben

[numthreads(LINEAR_XTHREADS, LINEAR_YTHREADS, LINEAR_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint id = DTid.x;
    uint trianglePairsLength, stride, sortIndicesLength;
    trianglePairs.GetDimensions(trianglePairsLength, stride);

    if (id >= trianglePairsLength)
        return;

    TrianglePair curTrianglePair = trianglePairs[id]; // das von diesem Thread bearbeitete Dreiecks-Paar

    // l�sche f�r den n�chsten Durchlauf den Eintrag im Buffer
	TrianglePair emptyTrianglePair = { 0, 0, 0, 0 }; 
	trianglePairs[id] = emptyTrianglePair;

    if ((curTrianglePair.triangleID1 == 0) && (curTrianglePair.triangleID2 == 0)) // ignoriere den leeren Bereich im Buffer
        return;

    // hole die Vertex-IDs aus dem IndexBUffer
    int3 triangle1 = triangleBuffer[curTrianglePair.triangleID1]; 
    int3 triangle2 = triangleBuffer[curTrianglePair.triangleID2];

    // hole die Vertices mit den IDs aus dem Vertex-Buffer
    float3 triangleArray1[3] = { vertexBuffer[triangle1[0]], vertexBuffer[triangle1[1]], vertexBuffer[triangle1[2]] };
    float3 triangleArray2[3] = { vertexBuffer[triangle2[0]], vertexBuffer[triangle2[1]], vertexBuffer[triangle2[2]] };

    float3 intersectionPoint; // der Ergebnis-IntersectionPoint, der den Schnittmittelpunkt beinhaltet

    if (TrianglesIntersect(triangleArray1, triangleArray2, intersectionPoint)) // sollten sich die Dreiecke �berschneiden
    {
        // f�ge den Intersectionpoint als Ergebnis dem intersectCenters-Buffer hinzu
        uint curCount = intersectCenters.IncrementCounter();
        intersectCenters[curCount] = intersectionPoint;

        // markiere f�r beide Objekte, dass sie Kollisionen aufweisen
        intersectingObjects[curTrianglePair.objectID1] = 1;
        intersectingObjects[curTrianglePair.objectID2] = 1;
    }

}
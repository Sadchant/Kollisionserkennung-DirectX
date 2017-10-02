#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"
#include "TriangleIntersections_HLSL.hlsl"

StructuredBuffer<float3> vertexBuffer : register(t0);
StructuredBuffer<int3> triangleBuffer : register(t1);

RWStructuredBuffer<TrianglePair> trianglePairs : register(u0);
RWStructuredBuffer<uint> intersectingObjects : register(u1);
AppendStructuredBuffer<float3> intersectCenters : register(u2);

[numthreads(LINEAR_XTHREADS/64, LINEAR_YTHREADS, LINEAR_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint id = DTid.x;
	uint trianglePairsLength, stride, sortIndicesLength;
	trianglePairs.GetDimensions(trianglePairsLength, stride);

	if (id >= trianglePairsLength)
		return;

	TrianglePair curTrianglePair = trianglePairs[id];

	int3 triangle1 = triangleBuffer[curTrianglePair.triangleID1];
	int3 triangle2 = triangleBuffer[curTrianglePair.triangleID2];

	float3 intersectionPoint;

    float3 triangleArray1[3] = { vertexBuffer[triangle1[0]], vertexBuffer[triangle1[1]], vertexBuffer[triangle1[2]] };
    float3 triangleArray2[3] = { vertexBuffer[triangle2[0]], vertexBuffer[triangle2[1]], vertexBuffer[triangle2[2]] };

    if (TrianglesIntersect(triangleArray1, triangleArray2, intersectionPoint))
    {
        intersectCenters.Append(intersectionPoint);
        intersectingObjects[curTrianglePair.objectID1] = 1;
        intersectingObjects[curTrianglePair.objectID2] = 1;
    }
}

struct Vertex
{
	float x, y, z;
};
struct Vector
{
	float x, y, z;
};
struct Triangle
{
	int aIndex, bIndex, cIndex;
};
struct BoundingBox
{
	Vertex position;
	Vector volumeVector;
};

StructuredBuffer<Vertex> vertexBuffer;
StructuredBuffer<Triangle> triangleBuffer;
RWStructuredBuffer<BoundingBox> boundingBoxBuffer;

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	Vertex min, max;
	uint numStructs, stride;
	triangleBuffer.GetDimensions(numStructs, stride);

	if (DTid.x > numStructs) return;

	BoundingBox bb = { {1,2,3},{numStructs, numStructs, numStructs} };
	boundingBoxBuffer[DTid.x] = bb;
	
}
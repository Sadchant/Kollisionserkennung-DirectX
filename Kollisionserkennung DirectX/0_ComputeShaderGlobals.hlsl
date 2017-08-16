struct BoundingBox
{
    float3 position;
    float3 volumeVector;
};

StructuredBuffer<float3> vertexBuffer : register(t0);
static BoundingBox sceneBoundingBox;
struct BoundingBox
{
    float3 minPoint;
    float3 maxPoint;
};

StructuredBuffer<float3> vertexBuffer : register(t0);
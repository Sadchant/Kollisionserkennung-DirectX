struct BoundingBox
{
    float3 minPoint;
    float3 maxPoint;
};

// berechne aus 3D-Koordinaten, der aktuellen Größe des Grids und dem Level-Offset die 1-dimensionale ID
uint get1DID(uint x, uint y, uint z, uint resolution, uint offset)
{
    return x + y * resolution + z * resolution * resolution + offset;
}

#define EMPTY 0
#define INTERNAL 1
#define LEAF 2


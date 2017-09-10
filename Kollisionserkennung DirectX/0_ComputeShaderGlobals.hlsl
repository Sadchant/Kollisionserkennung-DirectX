struct BoundingBox
{
    float3 minPoint;
    float3 maxPoint;
};

struct CellTrianglePair
{
    uint cellID;
    uint triangleID;
    uint objectID;
};

// berechne aus 3D-Koordinaten, der aktuellen Größe des Grids und dem Level-Offset die 1-dimensionale ID
uint get1DID(uint3 cell3DID, uint resolution, uint offset)
{
    return cell3DID.x + cell3DID.y * resolution + cell3DID.z * resolution * resolution + offset;
}

#define EMPTY 0
#define INTERNAL 1
#define LEAF 2
#define COPYDOWN 9


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

struct SortIndices
{
    uint array[4];
};

struct TrianglePair
{
    uint triangleID1;
    uint triangleID2;
	uint objectID1;
	uint objectID2;
};

// berechne aus 3D-Koordinaten, der aktuellen Größe des Grids und dem Level-Offset die 1-dimensionale ID
uint get1DID(uint3 cell3DID, uint resolution, uint offset)
{
    return cell3DID.x + cell3DID.y * resolution + cell3DID.z * resolution * resolution + offset;
}

#define EMPTY 0
#define INTERNAL 1
#define LEAF 2

SortIndices getSortIndicesFromInput(uint id, uint cellTrianglePairsSize, uint read2BitsFromHere, RWStructuredBuffer<CellTrianglePair> cellTrianglePairs)
{

    SortIndices resultSortIndices;
    resultSortIndices.array[0] = 0;
    resultSortIndices.array[1] = 0;
    resultSortIndices.array[2] = 0;
    resultSortIndices.array[3] = 0;

    if (id < cellTrianglePairsSize)
    {
        CellTrianglePair cellTrianglePair = cellTrianglePairs[id];
        uint bit0 = (cellTrianglePair.cellID >> read2BitsFromHere) & 1;
        uint bit1 = (cellTrianglePair.cellID >> read2BitsFromHere + 1) & 1;
        if (bit1 == 0 && bit0 == 0)
            resultSortIndices.array[3] = 1;
        else if (bit1 == 0 && bit0 == 1)
            resultSortIndices.array[2] = 1;
        else if (bit1 == 1 && bit0 == 0)
            resultSortIndices.array[1] = 1;
        else if (bit1 == 1 && bit0 == 1)
            resultSortIndices.array[0] = 1;
    }
    return resultSortIndices;
}



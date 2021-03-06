#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

RWStructuredBuffer<CellTrianglePair> cellTrianglePairs : register(u0); // der Input, nach Zellen-IDs sortierte CellTrianglePairs
RWStructuredBuffer<BoundingBox> boundingBoxes : register(u1); // um zu �berpr�fen, ob sie sich �berschneiden
RWStructuredBuffer<TrianglePair> trianglePairs : register(u2); // Buffer mit COUNTER, hier wird das Ergebnis hineingeschrieben


// �berpr�fe, ob sich die beiden Bounding Boxes �berschneiden
bool checkBoundingBoxIntersection(uint boundingBoxID1, uint boundingBoxID2)
{
    BoundingBox boundingBox1 = boundingBoxes[boundingBoxID1];
    BoundingBox boundingBox2 = boundingBoxes[boundingBoxID2];
    // wenn die eine Bounding Box in einer Dimension komplett au�erhalb der anderen liegt gibt es keine �berschneidung
    return !(((boundingBox1.maxPoint.x < boundingBox2.minPoint.x) || (boundingBox1.minPoint.x > boundingBox2.maxPoint.x)) ||
             ((boundingBox1.maxPoint.y < boundingBox2.minPoint.y) || (boundingBox1.minPoint.y > boundingBox2.maxPoint.y)) ||
             ((boundingBox1.maxPoint.z < boundingBox2.minPoint.z) || (boundingBox1.minPoint.z > boundingBox2.maxPoint.z)));
}

// gehe vom bearbeiteten Punkt durch die Cell-Triangle-Pairs, solange die Zellen-ID gleich ist
// sollten die ObjektIDs unterschiedlich sein, �berpr�fe

// ****** sollte eigentlich mit effizientem Load-Balancing durchgef�hrt werden, das hier ist eine simple L�sung, die aber langsam ist *****
[numthreads(LINEAR_XTHREADS, LINEAR_YTHREADS, LINEAR_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint id = DTid.x;
    uint cellTrianglePairsLength, stride, sortIndicesLength;
    cellTrianglePairs.GetDimensions(cellTrianglePairsLength, stride);

    if (id >= cellTrianglePairsLength)
        return;
    // mit diesem cellTrianglePair werden alle darauf folgenden cellTrianglePairs im aktuellen Zellenblock verglichen
    CellTrianglePair startCellTrianglePair = cellTrianglePairs[id]; 
    
    if (startCellTrianglePair.cellID == 0) // id 0 beinhaltet die gesamte Szene, sie beinhaltet keine Dreiecke, man hat also den leeren Bereich im Buffer erreicht
        return;
    uint curID = id + 1; // curID ist die ID des Elements, das mit dem startCellTrianglePair verglichen werden wird
    CellTrianglePair nextCellTrianglePair = cellTrianglePairs[curID]; // nextCelltrianglePair ist das zu vergleichende Element
    // solange die ID des n�chsten Elements gleich der ID von startCellTrianglePair ist, durchlaufe weiter den Buffer
    while ((startCellTrianglePair.cellID == nextCellTrianglePair.cellID) && (nextCellTrianglePair.cellID != 0)) 
    {
        if (startCellTrianglePair.objectID != nextCellTrianglePair.objectID) // vergleiche nur Boundingboxes mit unterschiedlichen Objekt-IDs
        {
            if (checkBoundingBoxIntersection(startCellTrianglePair.triangleID, nextCellTrianglePair.triangleID)) // wenn sich die Bounding Boxes schneiden
            {
                // f�ge ein neues TrianglePair mit den beiden Triangle- und Objekt-IDs dem Ergebnis-Buffer hinzu
                TrianglePair newTrianglePair = { startCellTrianglePair.triangleID, nextCellTrianglePair.triangleID, startCellTrianglePair.objectID, nextCellTrianglePair.objectID };
                uint curCount = trianglePairs.IncrementCounter();
                trianglePairs[curCount] = newTrianglePair;
            }
        }
        curID++;
        nextCellTrianglePair = cellTrianglePairs[curID]; // ermittle mit der hochgez�hlten curID das n�chste Element
    }
}
#pragma once

#include <vector>
#include <math.h>
#include <chrono>

#include "modelclass.h"
#include "HlslSharedDefines.h"

#include "Util.h"

using namespace chrono;

class CollisionDetectionManager
{
public:
	CollisionDetectionManager();
	~CollisionDetectionManager();
	void Initialize(ID3D11Device * device, ID3D11DeviceContext* deviceContext, HWND hwnd, vector<ModelClass*>* objects);
	void Shutdown();
	void Frame();

	ID3D11Buffer* CreateStructuredBuffer(UINT count, UINT structsize, UINT bindFlags, D3D11_USAGE usage, UINT CPUAccessFlags, D3D11_SUBRESOURCE_DATA *pData);
	ID3D11UnorderedAccessView* CreateBufferUnorderedAccessView(ID3D11Resource* pResource, int elementCount);
	ID3D11UnorderedAccessView* CreateBufferUnorderedAccessView(ID3D11Resource* pResource, int elementCount, UINT flags);

	ID3D11ShaderResourceView* CreateBufferShaderResourceView(ID3D11Resource * pResource, int elementCount);
	void OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, WCHAR * shaderFilename);

private:
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
		int vertexIndices[3];
	};
	struct BoundingBox
	{
		Vertex minPoint;
		Vector maxPoint;
	};
	struct CellTrianglePair
	{
		UINT cellID;
		UINT triangleID;
		UINT objectID;
	};

	__declspec(align(16)) // Structs in einem ConstantBuffer müpssen auf 16 Byte aligned sein
		struct ReduceData
	{
		int firstStepStride;
		int inputSize;
		int bool_OutputIsInput;
	};

	__declspec(align(16)) // Structs in einem ConstantBuffer müpssen auf 16 Byte aligned sein
		struct SingleUINT
	{
		UINT value;
	};

	__declspec(align(16)) // Structs in einem ConstantBuffer müpssen auf 16 Byte aligned sein
		struct TreeSizeInLevel
	{
		XMUINT4 treeSizeInLevel[SUBDIVS + 1];
	};


	void InitComputeShaderVector();
	void CreateSceneBuffersAndViews();
	ID3D11ComputeShader* CreateComputeShader(WCHAR * csFilename);
	void CreateVertexAndTriangleArray(vector<ModelClass*>* objects);
	void ReleaseBuffersAndViews();
	ID3D11Buffer* CreateConstantBuffer(UINT elementSize, D3D11_USAGE usage, D3D11_SUBRESOURCE_DATA *pData);

	void _1_BoundingBoxes();
	void _2_SceneCoundingBox();
	void _3_FillCounterTrees();
	void _4_GlobalCounterTree();
	void _5_FillTypeTree();
	void _6_FillLeafIndexTree();

	void _1_BoundingBoxes_GetResult();
	void _2_SceneCoundingBox_GetResult();
	void _3_FillCounterTrees_GetResult();
	void _4_GlobalCounterTree_GetResult();
	void _5_FillTypeTree_GetResult();
	void _6_FillLeafIndexTree_GetResult();

	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;
	HWND m_hwnd;

	int m_VertexCount;
	int m_TriangleCount;
	int m_ObjectCount;
	int m_GroupResult_Count; // wie groß ist das Ergebnis nach einem Reduce von Buffern der Größe m_VertexCount
	int m_TreeSize;
	int m_CounterTreesSize;
	int m_CellTrianglePairsCount;

	Vertex* m_Vertices; // Array: beinhaltet alle Punkte, also dreimal so viele wie es indices gibt
	Triangle* m_Triangles;
	UINT* m_ObjectsLastIndices; // hält pro Objekt den letzten Index, der zu diesem Objekt gehört
	UINT* m_CounterTrees_0s;
	UINT m_TreeSizeInLevel[SUBDIVS + 1];

	vector<ID3D11ComputeShader*> m_ComputeShaderVector;
	ID3D11ComputeShader* m_curComputeShader; // wird immer mit dem gerade benötigten Compute Shader aus computeShaderVector befüllt

	ID3D11Buffer* m_Vertex_Buffer; // alle Punkte der Szene, deren Objekte kollidieren
	ID3D11Buffer* m_Triangle_Buffer; // alle Dreiecke der Szene, deren Objekte kollidieren
	ID3D11Buffer* m_ObjectsLastIndices_Buffer; // die Indices im Dreieck-Buffer, die das letzte Dreieck eines Objektes markieren
	ID3D11Buffer* m_BoundingBox_Buffer; // die Bounding Boxes für jedes Dreieck
	ID3D11Buffer* m_GroupMinPoint_Buffer; // Ergebnisbuffer einer Reduktion: beinhaltet nach einem Durchlauf die MinimalPunkte, die jede Gruppe berechnet hat
	ID3D11Buffer* m_GroupMaxPoint_Buffer; // das selbe für die MaximalPunkte
	ID3D11Buffer* m_CounterTrees_Buffer; // die Countertrees für alle Objekte
	ID3D11Buffer* m_GlobalCounterTree_Buffer; // die Countertrees für alle Objekte
	ID3D11Buffer* m_TypeTree_Buffer; // der Typetree für den globalen Tree
	ID3D11Buffer* m_LeafIndexTree_Buffer; // in diesem Tree steht an jeder Stelle die ID der Zelle, die in diesem Zweig Blatt ist
	ID3D11Buffer* m_CellTrianglePairs_Buffer; // in diesem Tree steht an jeder Stelle die ID der Zelle, die in diesem Zweig Blatt ist

	// ConstantBuffer:
	ID3D11Buffer* m_ReduceData_CBuffer;
	ID3D11Buffer* m_ObjectCount_CBuffer;
	ID3D11Buffer* m_TreeSizeInLevel_CBuffer;
	ID3D11Buffer* m_StartLevel_CBuffer;
	ID3D11Buffer* m_Loops_CBuffer;

	// Test-ResultBuffer
	BoundingBox* m_Results1; // wird von der GPU befüllt!
	Vertex* m_Results2_1; // wird von der GPU befüllt!
	Vertex* m_Results2_2; // wird von der GPU befüllt!
	UINT* m_Results3; // wird von der GPU befüllt!
	UINT* m_Results4; // wird von der GPU befüllt!
	UINT* m_Results5_1; // wird von der GPU befüllt!
	UINT* m_Results5_2; // wird von der GPU befüllt!
	UINT* m_Results6; // wird von der GPU befüllt!
	CellTrianglePair* m_Results7; // wird von der GPU befüllt!

	// langsame (CPU-Zugriff!) ResultBuffer, in die ein Ergebnis von der GPU kopiert wird
	ID3D11Buffer* m_Result_Buffer1;
	ID3D11Buffer* m_Result_Buffer2_1;
	ID3D11Buffer* m_Result_Buffer2_2;
	ID3D11Buffer* m_Result_Buffer3;
	ID3D11Buffer* m_Result_Buffer4;
	ID3D11Buffer* m_Result_Buffer5_1;
	ID3D11Buffer* m_Result_Buffer5_2;
	ID3D11Buffer* m_Result_Buffer6;
	ID3D11Buffer* m_Result_Buffer7;



	// Shader Resource Views und Unordered Access Views für die Buffer
	ID3D11ShaderResourceView* m_NULL_SRV;
	ID3D11UnorderedAccessView* m_NULL_UAV;

	ID3D11ShaderResourceView* m_Vertex_SRV;
	ID3D11ShaderResourceView* m_Triangle_SRV;
	ID3D11ShaderResourceView* m_ObjectsLastIndices_SRV;

	ID3D11UnorderedAccessView* m_BoundingBox_UAV;
	ID3D11UnorderedAccessView* m_GroupMinPoint_UAV;
	ID3D11UnorderedAccessView* m_GroupMaxPoint_UAV;
	ID3D11UnorderedAccessView* m_CounterTrees_UAV;
	ID3D11UnorderedAccessView* m_GlobalCounterTree_UAV;
	ID3D11UnorderedAccessView* m_TypeTree_UAV;
	ID3D11UnorderedAccessView* m_LeafIndexTree_UAV;
	ID3D11UnorderedAccessView* m_CellTrianglePairs_UAV;

};


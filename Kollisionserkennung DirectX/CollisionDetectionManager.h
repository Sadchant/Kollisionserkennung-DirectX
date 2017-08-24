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


	ID3D11Buffer * CreateStructuredBuffer(UINT count, UINT structsize, UINT bindFlags, D3D11_USAGE usage, UINT CPUAccessFlags, D3D11_SUBRESOURCE_DATA *pData);
	ID3D11UnorderedAccessView * CreateBufferUnorderedAccessView(ID3D11Resource * pResource, int elementCount);

	ID3D11ShaderResourceView * CreateBufferShaderResourceView(ID3D11Resource * pResource, int elementCount);

	void OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, WCHAR * shaderFilename);

	bool Frame();

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
	__declspec(align(16)) // Structs in einem ConstantBuffer müpssen auf 16 Byte aligned sein
	struct ReduceData 
	{
		int firstStepStride;
		int inputSize;
		int bool_OutputIsInput;
	};
	__declspec(align(16)) // Structs in einem ConstantBuffer müpssen auf 16 Byte aligned sein
	struct FillCounterTreesData
	{
		XMUINT4 objectCount;
		XMUINT4 treeSizeInLevel[LEVELS+1]; // weil hlsl 16 byte pro Array-Eintrag braucht, schreibe und lese immer nur den ersten Eintrag!
		// LEVELS + 1, weil man ja auch die Array-Größe vom höchsten Level kennen möchte
	};

	void InitComputeShaderVector();
	void CreateSceneBuffersAndViews();
	ID3D11ComputeShader* CreateComputeShader(WCHAR * csFilename);
	void CreateVertexAndTriangleArray(vector<ModelClass*>* objects);
	void ReleaseBuffersAndViews();
	ID3D11Buffer* CreateConstantBuffer(UINT elementSize, D3D11_USAGE usage, D3D11_SUBRESOURCE_DATA *pData);
	
	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;
	HWND m_hwnd;

	int m_VertexCount;
	int m_TriangleCount;
	int m_ObjectCount;
	int m_GroupResult_Count; // wie groß ist das Ergebnis nach einem Reduce von Buffern der Größe m_VertexCount
	int m_TreeSize;
	int m_CounterTreesSize;

	Vertex* m_Vertices; // Array: beinhaltet alle Punkte, also dreimal so viele wie es indices gibt
	Triangle* m_Triangles;
	UINT* m_ObjectsLastIndices; // hält pro Objekt den letzten Index, der zu diesem Objekt gehört
	UINT* m_CounterTrees_0s;
	FillCounterTreesData m_FillCounterTreesData;

	vector<ID3D11ComputeShader*> m_ComputeShaderVector;
	ID3D11ComputeShader* m_curComputeShader; // wird immer mit dem gerade benötigten Compute Shader aus computeShaderVector befüllt

	ID3D11Buffer* m_Vertex_Buffer; // alle Punkte der Szene, deren Objekte kollidieren
	ID3D11Buffer* m_Triangle_Buffer; // alle Dreiecke der Szene, deren Objekte kollidieren
	ID3D11Buffer* m_ObjectsLastIndices_Buffer; // die Indices im Dreieck-Buffer, die das letzte Dreieck eines Objektes markieren
	ID3D11Buffer* m_BoundingBox_Buffer; // die Bounding Boxes für jedes Dreieck
	ID3D11Buffer* m_GroupMinPoint_Buffer; // Ergebnisbuffer einer Reduktion: beinhaltet nach einem Durchlauf die MinimalPunkte, die jede Gruppe berechnet hat
	ID3D11Buffer* m_GroupMaxPoint_Buffer; // das selbe für die MaximalPunkte
	ID3D11Buffer* m_CounterTrees_Buffer; // die Countertrees für alle Objekte
	
	ID3D11Buffer* m_ReduceData_CBuffer; // ConstantBuffer, die den firstStepStride an den Shader weitergibt
	ID3D11Buffer* m_FillCounterTreesData_CBuffer; // ConstantBuffer, die den firstStepStride an den Shader weitergibt

	// Test-ResultBuffer
	BoundingBox* m_Results1; // wird von der GPU befüllt!
	Vertex* m_Results2_1; // wird von der GPU befüllt!
	Vertex* m_Results2_2; // wird von der GPU befüllt!
	UINT* m_Results3; // wird von der GPU befüllt!

	ID3D11Buffer* m_Result_Buffer1; // ein langsamer (CPU-Zugriff!) ResultBuffer, in den ein Ergebnis von der GPU kopiert wird
	ID3D11Buffer* m_Result_Buffer2_1; // ein langsamer (CPU-Zugriff!) ResultBuffer, in den ein Ergebnis von der GPU kopiert wird
	ID3D11Buffer* m_Result_Buffer2_2; // ein langsamer (CPU-Zugriff!) ResultBuffer, in den ein Ergebnis von der GPU kopiert wird
	ID3D11Buffer* m_Result_Buffer3; // ein langsamer (CPU-Zugriff!) ResultBuffer, in den ein Ergebnis von der GPU kopiert wird


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

};


#pragma once

#include <vector>
#include <math.h>
#include <chrono>

#include "modelclass.h"

#include "Util.h"

using namespace chrono;

class CollisionDetectionManager
{
public:
	CollisionDetectionManager();
	~CollisionDetectionManager();
	void Initialize(ID3D11Device * device, ID3D11DeviceContext* deviceContext, HWND hwnd);
	void Shutdown();

	bool CreateComputeShader(HWND hwnd, WCHAR * csFilename);

	ID3D11Buffer * CreateStructuredBuffer(UINT count, UINT structsize, UINT bindFlags, D3D11_USAGE usage, UINT CPUAccessFlags, D3D11_SUBRESOURCE_DATA *pData);
	ID3D11UnorderedAccessView * CreateBufferUnorderedAccessView(ID3D11Resource * pResource, int elementCount);

	ID3D11ShaderResourceView * CreateBufferShaderResourceView(ID3D11Resource * pResource, int elementCount);

	void OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, WCHAR * shaderFilename);

	bool Frame(vector<ModelClass*>* objects);

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
		Vertex position;
		Vector volumeVector;
	};

	void CreateVertexAndTriangleArray(vector<ModelClass*>* objects);
	void ReleaseBuffersAndViews();
	
	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;
	int m_VertexCount;
	int m_TriangleCount;
	int m_ObjectCount;
	Vertex* m_Vertices; // beinhaltet alle Punkte, also dreimal so viele wie es indices gibt
	Triangle* m_Triangles;
	int* m_ObjectsLastIndices; // hält pro Objekt den letzten Index, der zu diesem Objekt gehört
	BoundingBox* m_BoundingBoxes; // wird von der GPU befüllt!
	ID3D11ComputeShader* m_ComputeShader; // managed ein ausführbares Programm (einen Vertex-Shader)

	ID3D11Buffer* m_Vertex_Buffer;
	ID3D11Buffer* m_Triangle_Buffer;
	ID3D11Buffer* m_ObjectsLastIndices_Buffer;
	ID3D11Buffer* m_BoundingBox_Buffer;
	ID3D11Buffer* m_Result_Buffer;

	ID3D11ShaderResourceView* m_Vertex_SRV;
	ID3D11ShaderResourceView* m_Triangle_SRV;
	ID3D11ShaderResourceView* m_ObjectsLastIndices_SRV;

	ID3D11UnorderedAccessView* m_BoundingBox_UAV;



};


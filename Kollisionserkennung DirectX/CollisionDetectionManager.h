#pragma once

#include <vector>
#include <math.h>
#include "modelclass.h"

#include "Util.h"

class CollisionDetectionManager
{
public:
	CollisionDetectionManager();
	~CollisionDetectionManager();
	void Initialize(ID3D11Device * device, ID3D11DeviceContext* deviceContext, HWND hwnd);
	void CreateVertexAndTriangleArray(vector<ModelClass*>* objects);
	void ReleaseArrays();
	void Shutdown();

	bool CreateComputeShader(HWND hwnd, WCHAR * csFilename);

	ID3D11Buffer * CreateStructuredBuffer(UINT count, UINT structsize, UINT bindFlags, D3D11_USAGE usage, UINT CPUAccessFlags, D3D11_SUBRESOURCE_DATA *pData);
	ID3D11UnorderedAccessView * CreateBufferUnorderedAccessView(ID3D11Resource * pResource, int elementCount);

	ID3D11ShaderResourceView * CreateBufferShaderResourceView(ID3D11Resource * pResource, int elementCount);

	void RunComputeShader(ID3D11ComputeShader * computeShader, int uavCount, ID3D11UnorderedAccessView **unorderedAccessViews, int xThreadCount, int yThreadCount);

	void OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, WCHAR * shaderFilename);

	void Frame(vector<ModelClass*>* objects);

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
		int aIndex, bIndex, cIndex;
	};
	struct BoundingBox
	{
		Vertex position;
		Vector volumeVector;
	};

	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;
	int m_VertexCount;
	int m_TriangleCount;
	Vertex* m_Vertices; // beinhaltet alle Punkte, also dreimal so viele wie es indices gibt
	Triangle* m_Triangles;
	BoundingBox* m_BoundingBoxes; // wird von der GPU befüllt!
	ID3D11ComputeShader* m_computeShader; // managed ein ausführbares Programm (einen Vertex-Shader)

	ID3D11Buffer* m_vertexBuffer;
	ID3D11Buffer* m_triangleBuffer;
	ID3D11Buffer* m_boundingBoxBuffer;
	ID3D11Buffer* m_ResultBuffer;

	ID3D11ShaderResourceView* m_VertexShaderResourceView;
	ID3D11ShaderResourceView* m_TriangleShaderResourceView;
	ID3D11UnorderedAccessView* m_BoundingBoxUnorderedAccessView;

};


#pragma once

// As stated previously the ModelClass is responsible for encapsulating the geometry for 3D models.
// In this tutorial we will manually setup the data for a single green triangle. We will also create
// a vertex and index buffer for the triangle so that it can be rendered.

#include <d3d11.h>
#include <directxmath.h>
#include <fstream>
#include <iostream>

#include "lightshaderclass.h"
#include "textureclass.h"

using namespace DirectX;
using namespace std;

// The next change is the addition of a new structure to represent the model format.It is called ModelType.It contains position, texture, and normal vectors the same as our file format does.
struct VertexAndVertexDataType
{
	float x, y, z;
	float tu, tv;
	float nx, ny, nz;
};

class ModelClass
{
public:
	ModelClass();
	ModelClass(const ModelClass&);
	~ModelClass();

	// The functions here handle initializing and shutdown of the model's vertex and index buffers. The Render function puts the model
	// geometry on the video card to prepare it for drawing by the color shader.
	// The Initialize function will now take as input the character string file name of the model to be loaded.
	bool Initialize(ID3D11Device*, char*, HWND hwnd);
	void Shutdown();
	bool Render(ID3D11DeviceContext* deviceContext, XMMATRIX worldmatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, XMFLOAT3 lightDirection, XMFLOAT4 diffuseColor);

	int GetIndexCount();
	VertexAndVertexDataType* GetModelData() { return m_model; }

	ID3D11ShaderResourceView* GetTexture();

	


private:
	bool InitializeBuffers(ID3D11Device*);
	void ShutdownBuffers();
	void RenderBuffers(ID3D11DeviceContext*);

	ID3D11Buffer *m_vertexBuffer, *m_indexBuffer;
	int m_vertexCount, m_indexCount;
	LightShaderClass* m_LightShader;
	TextureClass* m_Texture;
	
	// The final change is a new private variable called m_model which is going to be an array of the new private structure ModelType. 
	// This variable will be used to read in and hold the model data before it is placed in the vertex buffer.
	VertexAndVertexDataType *m_model;

	bool LoadTexture(ID3D11Device*, WCHAR*);
	void ReleaseTexture();
	bool LoadModel(char*);
	void ReleaseModel();

	// Here is the definition of our vertex type that will be used with the vertex buffer in this ModelClass.
	// Also take note that this typedef must match the layout in the ColorShaderClass that will be looked at later in the tutorial.
	struct VertexType
	{
		// XMFLOAT3: Describes a 3D vector consisting of three single-precision floating-point values.
		XMFLOAT3 position;
		XMFLOAT2 texture;
		XMFLOAT3 normal;
	};

	
	
};
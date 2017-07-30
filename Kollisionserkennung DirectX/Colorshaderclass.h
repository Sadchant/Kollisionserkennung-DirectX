#ifndef _COLORSHADERCLASS_H_
#define _COLORSHADERCLASS_H_

// The ColorShaderClass is what we will use to invoke our HLSL shaders for drawing the 3D models that are on the GPU.

#pragma comment(lib, "D3DCompiler.lib")
//#pragma comment(lib, "d3d11.lib")
//#pragma comment(lib, "d3dx11.lib")

#include <d3d11.h>
#include <D3Dcompiler.h>
#include <directxmath.h>
#include <fstream>
#include <iostream>
using namespace DirectX;
using namespace std;


class ColorShaderClass
{
private:

	// Here is the definition of the cBuffer type that will be used with the vertex shader.
	// This typedef must be exactly the same as the one in the vertex shader as the model data needs to match the typedefs in the shader for proper rendering.

	// XMMMATRIX: Struct mit einem Array von 4 XMVECTOR (XNVECTOR: crazy DirectX-4-Vektor
	// Array of four vectors, representing the rows of the matrix
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

public:
	ColorShaderClass();
	ColorShaderClass(const ColorShaderClass&);
	~ColorShaderClass();

	// The functions here handle initializing and shutdown of the shader.The render function sets the shader parameters and then draws the prepared model vertices using the shader.

	bool Initialize(ID3D11Device*, HWND);
	void Shutdown();
	bool Render(ID3D11DeviceContext* , int , XMMATRIX& , XMMATRIX& , XMMATRIX& );

private:
	bool InitializeShader(ID3D11Device* device, HWND hwnd, WCHAR* vsFilename, WCHAR* psFilename);
	void ShutdownShader();
	void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);

	bool SetShaderParameters(ID3D11DeviceContext*, XMMATRIX&, XMMATRIX&, XMMATRIX&);
	void RenderShader(ID3D11DeviceContext*, int);

private:
	ID3D11VertexShader* m_vertexShader; // managed ein ausführbares Programm (einen Vertex-Shader)
	ID3D11PixelShader* m_pixelShader;	// managed den Pixel-Shader
	ID3D11InputLayout* m_layout;		// wird benötigt,um Punktdaten, die im Speicher liegen in die Grafikpipeline zu bekommen
	ID3D11Buffer* m_matrixBuffer;		// ruft Buffer-Resourcen ab (unstrukturierter Speicher mit Punkten und Indexen)
};

#endif

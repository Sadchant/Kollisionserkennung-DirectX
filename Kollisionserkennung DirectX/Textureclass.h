#pragma once

#include <d3d11.h>
#include <DDSTextureLoader.h>
#include <stdio.h>

using namespace DirectX;


class TextureClass
{
private:

	// We define the targa file header structure here to make reading in the data easier.

	struct TargaHeader
	{
		unsigned char data1[12];
		unsigned short width;
		unsigned short height;
		unsigned char bpp;
		unsigned char data2;
	};

public:
	TextureClass();
	TextureClass(const TextureClass&);
	~TextureClass();

	bool Initialize(ID3D11Device*, WCHAR*);
	void Shutdown();

	ID3D11ShaderResourceView* GetTexture();

private:

	// Here we have our targa reading function.If you wanted to support more formats you would add reading functions here.

	bool LoadTarga(char*, int&, int&);

private:

	// This class has three member variables. The first one holds the raw targa data read straight in from the file. 
	// The second variable called m_texture will hold the structured texture data that DirectX will use for rendering. 
	// And the third variable is the resource view that the shader uses to access the texture data when drawing.

	unsigned char* m_targaData;
	ID3D11Resource* m_texture;				// A 2D texture interface manages texel data, which is structured memory.
	ID3D11ShaderResourceView* m_textureView; // A shader-resource-view interface specifies the subresources a shader can access during rendering

};
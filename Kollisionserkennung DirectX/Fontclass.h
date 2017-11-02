#pragma once
////////////////////////////////////////////////////////////////////////////////
// Filename: fontclass.h
////////////////////////////////////////////////////////////////////////////////

// ***Quelle***: http://www.rastertek.com/tutdx11.html



//////////////
// INCLUDES //
//////////////
#include <d3d11.h>
#include <directxmath.h>
#include <fstream>
using namespace std;


///////////////////////
// MY CLASS INCLUDES //
///////////////////////
#include "textureclass.h"


////////////////////////////////////////////////////////////////////////////////
// Class name: FontClass
////////////////////////////////////////////////////////////////////////////////
class FontClass
{
private:
	// The FontType structure is used to hold the indexing data read in from the font index file.The left and right are the TU texture coordinates.
	// The size is the width of the character in pixels.
		struct FontType
	{
		float left, right;
		int size;
	};
	// The VertexType structure is for the actual vertex data used to build the square to render the text character on. The individual character
	// will require two triangles to make a square.Those triangles will have position and texture data only.
	struct VertexType
	{
			XMFLOAT3 position;
			XMFLOAT2 texture;
	};

public:
	FontClass();
	FontClass(const FontClass&);
	~FontClass();

	bool Initialize(ID3D11Device*, char*, WCHAR*);
	void Shutdown();

	ID3D11ShaderResourceView* GetTexture();
	// BuildVertexArray will handle building and returning a vertex array of triangles that will render the character sentence which was given as
	// input to this function.This function will be called by the new TextClass to build vertex arrays of all the sentences it needs to render.
	void BuildVertexArray(void*, char*, float, float);

private:
	bool LoadFontData(char*);
	void ReleaseFontData();
	bool LoadTexture(ID3D11Device*, WCHAR*);
	void ReleaseTexture();

private:
	FontType* m_Font;
	TextureClass* m_Texture;
};



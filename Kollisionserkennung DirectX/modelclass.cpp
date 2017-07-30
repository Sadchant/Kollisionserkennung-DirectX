#include "modelclass.h"


ModelClass::ModelClass()
{
	m_vertexBuffer = 0;
	m_indexBuffer = 0;
	m_Texture = 0;
	m_model = 0;
}

ModelClass::ModelClass(const ModelClass& other)
{
}


ModelClass::~ModelClass()
{
}


// The Initialize function will call the initialization functions for the vertex and index buffers.
bool ModelClass::Initialize(ID3D11Device* device, char* modelFilename, WCHAR* textureFilename)
{
	bool result;

	// Load in the model data,
	result = LoadModel(modelFilename);
	if (!result)
	{
		return false;
	}

	// Initialize the vertex and index buffers.
	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}

	// Load the texture for this model.
	result = LoadTexture(device, textureFilename);
	if (!result)
	{
		return false;
	}

	return true;
}

// The Shutdown function will call the shutdown functions for the vertex and index buffers.
void ModelClass::Shutdown()
{
	// Release the model texture.
	ReleaseTexture();

	// Shutdown the vertex and index buffers.
	ShutdownBuffers();

	// Release the model data.
	ReleaseModel();
	return;
}

// Render is called from the GraphicsClass::Render function.This function calls RenderBuffers to put the vertex and index buffers 
// on the graphics pipeline so the color shader will be able to render them.
void ModelClass::Render(ID3D11DeviceContext* deviceContext)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(deviceContext);
	return;
}


// GetIndexCount returns the number of indexes in the model. The color shader will need this information to draw this model.
int ModelClass::GetIndexCount()
{
	return m_indexCount;
}

ID3D11ShaderResourceView* ModelClass::GetTexture()
{
	return m_Texture->GetTexture();
}

// The InitializeBuffers function is where we handle creating the vertex and index buffers. Usually you would read in a model
// and create the buffers from that data file. For this tutorial we will just set the points in the vertex and index buffer manually
// since it is only a single triangle.
bool ModelClass::InitializeBuffers(ID3D11Device* device)
{
	// das struct mit position und Farbe
	VertexType* vertices;
	unsigned long* indices;
	// D3D11_BUFFER_DESC: Describes a buffer resource.
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	// D3D11_SUBRESOURCE_DATA: Specifies data for initializing a subresource.
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;

	// Create the vertex array.
	vertices = new VertexType[m_vertexCount];
	if (!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new unsigned long[m_indexCount];
	if (!indices)
	{
		return false;
	}

	//Loading the vertex and index arrays has changed a bit.Instead of setting the values manually we loop through all the elements in the new m_model array and copy that data 
	// from there into the vertex array.The index array is easy to build as each vertex we load has the same index number as the position in the array it was loaded into.
	// Load the vertex array and index array with data.
	for (int i = 0; i < m_vertexCount; i++)
	{
		vertices[i].position = XMFLOAT3(m_model[i].x, m_model[i].y, m_model[i].z);
		vertices[i].texture = XMFLOAT2(m_model[i].tu, m_model[i].tv);
		vertices[i].normal = XMFLOAT3(m_model[i].nx, m_model[i].ny, m_model[i].nz);

		indices[i] = i;
	}

	// With the vertex array and index array filled out we can now use those to create the vertex buffer and index buffer. Creating both
	// buffers is done in the same fashion. First fill out a description of the buffer. In the description the ByteWidth(size of the buffer)
	// and the BindFlags(type of buffer) are what you need to ensure are filled out correctly. After the description is filled out you need
	// to also fill out a subresource pointer which will point to either your vertex or index array you previously created. With the description
	// and subresource pointer you can call CreateBuffer using the D3D device and it will return a pointer to your new buffer.

	// Set up the description of the static vertex buffer.
	// gibt an, wie Grafikkarte und CPU auf dem Buffer lesen/schreiben sollen, DEFAULT: Grafikkarte kann lesen und schreiben
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	//Size of the buffer in bytes.
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
	// Identify how the buffer will be bound to the pipeline.
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	// 0 if no CPU access is necessary
	vertexBufferDesc.CPUAccessFlags = 0;
	// sonstige Flags, kann dem Buffer verschiedene Eigenschaften geben
	vertexBufferDesc.MiscFlags = 0;
	// The size of each element in the buffer structure (in bytes) when the buffer represents a structured buffer.
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	// Pointer to the initialization data.
	// Zeiger auf den Buffer
	vertexData.pSysMem = vertices;
	// The distance (in bytes) from the beginning of one line of a texture to the next line.
	vertexData.SysMemPitch = 0;
	// The distance(in bytes) from the beginning of one depth level to the next.
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	// Creates a buffer (vertex buffer, index buffer, or shader-constant buffer).
	// der dritte Parameter wird von CreateBuffer befüllt
	// This method returns E_OUTOFMEMORY if there is insufficient memory to create the buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// After the vertex buffer and index buffer have been created you can delete the vertex and index arrays as they are no longer needed since the data was copied into the buffers.

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;
}

// The ShutdownBuffers function just releases the vertex buffer and index buffer that were created in the InitializeBuffers function.
void ModelClass::ShutdownBuffers()
{
	// Release the index buffer.
	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = 0;
	}

	// Release the vertex buffer.
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}
	return;
}

// RenderBuffers is called from the Render function.The purpose of this function is to set the vertex buffer and index buffer as active
// on the input assembler in the GPU. Once the GPU has an active vertex buffer it can then use the shader to render that buffer. This
// function also defines how those buffers should be drawn such as triangles, lines, fans, and so forth. In this tutorial we set the vertex
// buffer and index buffer as active on the input assembler and tell the GPU that the buffers should be drawn as triangles using the IASetPrimitiveTopology DirectX function.

void ModelClass::RenderBuffers(ID3D11DeviceContext* deviceContext)
{
	unsigned int stride;
	unsigned int offset;


	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	// The Direct3D 10 and higher API separates functional areas of the pipeline into stages; the first stage in the pipeline is the input-assembler (IA) stage. 
	// The purpose of the input - assembler stage is to read primitive data(points, lines and / or triangles) from user - filled buffers and assemble the data 
	// into primitives that will be used by the other pipeline stages.
	deviceContext->IASetVertexBuffers(0, // The first input slot for binding. The first vertex buffer is explicitly bound to the start slot
		1,								// The number of vertex buffers in the array.
		&m_vertexBuffer,				// A pointer to an array of vertex buffers
		&stride,						// Pointer to an array of stride (Schritt) values; one stride value for each buffer in the vertex-buffer array. Each stride is the size (in bytes) of the elements that are to be used from that vertex buffer.
		&offset);						// Pointer to an array of offset values; one offset value for each buffer in the vertex-buffer array. Each offset is the number of bytes between the first element of a vertex buffer and the first element that will be used.

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(m_indexBuffer,	// A pointer to an ID3D11Buffer object, that contains indices.
		DXGI_FORMAT_R32_UINT,						// A DXGI_FORMAT that specifies the format of the data in the index buffer.
		0);											// Offset (in bytes) from the start of the index buffer to the first index to use.

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	// Bind information about the primitive type, and data order that describes input data for the input assembler stage.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}

bool ModelClass::LoadTexture(ID3D11Device* device, WCHAR* filename)
{
	bool result;


	// Create the texture object.
	m_Texture = new TextureClass;
	if (!m_Texture)
	{
		return false;
	}

	// Initialize the texture object.
	result = m_Texture->Initialize(device, filename);
	if (!result)
	{
		return false;
	}

	return true;
}

void ModelClass::ReleaseTexture()
{
	// Release the texture object.
	if (m_Texture)
	{
		m_Texture->Shutdown();
		delete m_Texture;
		m_Texture = 0;
	}

	return;
}

// This is the new LoadModel function which handles loading the model data from the text file into the m_model array variable.It opens the text file and reads in the vertex count first.
// After reading the vertex count it creates the ModelType array and then reads each line into the array.Both the vertex count and index count are now set in this function.
bool ModelClass::LoadModel(char* filename)
{
	ifstream fin;
	char input;
	int i;


	// Open the model file.
	fin.open(filename);

	// If it could not open the file then exit.
	if (fin.fail())
	{
		return false;
	}

	// Read up to the value of vertex count.
	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}

	// Read in the vertex count.
	fin >> m_vertexCount;

	// Set the number of indices to be the same as the vertex count.
	m_indexCount = m_vertexCount;

	// Create the model using the vertex count that was read in.
	m_model = new ModelType[m_vertexCount];
	if (!m_model)
	{
		return false;
	}

	// Read up to the beginning of the data.
	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}
	fin.get(input);
	fin.get(input);

	// Read in the vertex data.
	for (i = 0; i<m_vertexCount; i++)
	{
		fin >> m_model[i].x >> m_model[i].y >> m_model[i].z;
		fin >> m_model[i].tu >> m_model[i].tv;
		fin >> m_model[i].nx >> m_model[i].ny >> m_model[i].nz;
	}

	// Close the model file.
	fin.close();

	return true;
}

void ModelClass::ReleaseModel()
{
	if (m_model)
	{
		delete[] m_model;
		m_model = 0;
	}

	return;
}




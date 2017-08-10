#include "CollisionDetectionManager.h"



CollisionDetectionManager::CollisionDetectionManager()
{
	device = 0;
	deviceContext = 0;
	m_Vertices = 0;
	m_Triangles = 0;
	m_ObjectsLastIndices = 0;
	m_BoundingBoxes = 0;
	m_ComputeShader = 0;

	m_Vertex_Buffer = 0;
	m_Triangle_Buffer = 0;
	m_ObjectsLastIndices_Buffer = 0;
	m_BoundingBox_Buffer = 0;
	m_Result_Buffer = 0;

	m_Vertex_SRV = 0;
	m_Triangle_SRV = 0;
	m_ObjectsLastIndices_SRV = 0;

	m_BoundingBox_UAV = 0;
}


CollisionDetectionManager::~CollisionDetectionManager()
{
}

void CollisionDetectionManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd)
{
	this->device = device;
	this->deviceContext = deviceContext;
	CreateComputeShader(hwnd, L"../Kollisionserkennung DirectX/1_BoundingBox_ComputeShader.hlsl");
}

// kopiert Punkte und Dreiecke aller Objekte in objects in große Buffer zusammen, in LastObjectIndices wird sich gemerkt, welche Dreiecke
// zu welchem Objekt gehören
void CollisionDetectionManager::CreateVertexAndTriangleArray(vector<ModelClass*>* objects)
{
	//Zuerst ausrechnen, wie groß das Triangle-Array sein sollte
	m_VertexCount = 0;
	for (ModelClass *curModel : *objects) 
	{
		m_VertexCount += curModel->GetIndexCount();
	}

	m_TriangleCount = m_VertexCount / 3;  // es gibt 3 Punkte pro Dreieck, also /3
	m_ObjectCount = (int)objects->size();

	// vor dem neuen Speicher allokieren den alten freiegeben
	SAFEDELETEARRAY(m_Vertices);
	SAFEDELETEARRAY(m_Triangles);
	SAFEDELETEARRAY(m_BoundingBoxes);

	m_Vertices = new Vertex[m_VertexCount];
	m_Triangles = new Triangle[m_TriangleCount];
	m_ObjectsLastIndices = new int[objects->size()]; // es gibt so viele Einträge wie Objekte
	m_BoundingBoxes = new BoundingBox[m_TriangleCount]; // eine Bounding Box pro Dreieck

	int curGlobalPosition = 0;
	int curLastIndex = 0;
	// gehe über alle Objekte
	for (int i = 0; i < objects->size(); i++)
	{
		ModelClass *curModel = (*objects)[i];
		VertexAndVertexDataType* modelData = curModel->GetModelData(); // hole die rohen Objektdaten (ein Eintrag ist ein Punkt, seine Texturkoordinaten und Normale, wir wollen aber nur den Punkt)
		for (int i = 0; i < curModel->GetIndexCount(); i++) // iteriere über jeden Vertex in modelData
		{
			m_Vertices[curGlobalPosition] = { modelData[i].x,  modelData[i].y, modelData[i].z };
			if (curGlobalPosition % 3 == 0)
			{
				// im Format dieses Tutorials sind die Punkte so aufgelistet, dass der Reihe nach durchgezählt wird, um auf die Dreiecke zu kommen
				int curTriangleIndex = curGlobalPosition / 3;
				m_Triangles[curTriangleIndex] = { { curGlobalPosition,  curGlobalPosition + 1, curGlobalPosition + 2} };
			}
			curGlobalPosition++;
		}
		// schreibe außerdem den letzten Index des Objektes in m_ObjectLastIndices
		curLastIndex += curModel->GetIndexCount();
		m_ObjectsLastIndices[i] = curLastIndex;
	}
}

// gib alle Buffer, Shader Resource Views und Unordered Access Views frei
void CollisionDetectionManager::ReleaseBuffersAndViews()
{
	SAFERELEASE(m_Vertex_Buffer);
	SAFERELEASE(m_Triangle_Buffer);
	SAFERELEASE(m_ObjectsLastIndices_Buffer);
	SAFERELEASE(m_BoundingBox_Buffer);
	SAFERELEASE(m_Result_Buffer);

	SAFERELEASE(m_Vertex_SRV);
	SAFERELEASE(m_Triangle_SRV);
	SAFERELEASE(m_ObjectsLastIndices_SRV);

	SAFERELEASE(m_BoundingBox_UAV);
}

void CollisionDetectionManager::Shutdown()
{
	SAFERELEASE(m_ComputeShader);

	SAFEDELETEARRAY(m_Vertices);
	SAFEDELETEARRAY(m_Triangles);
	SAFEDELETEARRAY(m_ObjectsLastIndices);
	SAFEDELETEARRAY(m_BoundingBoxes);

	ReleaseBuffersAndViews();
}

bool CollisionDetectionManager::CreateComputeShader(HWND hwnd, WCHAR* csFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage; // wird benutzt um beliebige "Length-Data" zurückzugeben
							  // Blobs can be used as a data buffer, storing vertex, adjacency, and material information during mesh optimization and loading
							  // operations. Also, these objects are used to return object code and error messages in APIs that compile vertex, geometry and pixel shaders.

							  // also wird HLSL kompiliert und in den Blob kommt das was rauskommt
	ID3D10Blob* computeShaderBuffer;

	// D3D11_INPUT_ELEMENT_DESC: A description of a single element for the input-assembler stage.
	// dient vermutlich dazu, Daten in den Shader hineinzubekommen (nein in die Pipeline, also nur indirekt in den Shader)
	// D3D11_INPUT_ELEMENT_DESC polygonLayout[2];


	// Initialize the pointers this function will use to null.
	errorMessage = 0;
	computeShaderBuffer = 0;

	// Here is where we compile the shader programs into buffers. We give it the name of the shader file, the name of the shader, the shader 
	// version(5.0 in DirectX 11), and the buffer to compile the shader into. If it fails compiling the shader it will put an error message
	// inside the errorMessage string which we send to another function to write out the error.If it still fails and there is no errorMessage
	// string then it means it could not find the shader file in which case we pop up a dialog box saying so.

	// Compile the compute shader code, "ColorVertexShader": Name der Methode im Shader, die aufgerufen wird
	result = D3DCompileFromFile(csFilename, NULL, NULL, "main", "cs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		&computeShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, csFilename);
		}
		// If there was  nothing in the error message then it simply could not find the shader file itself.
		else
		{
			MessageBox(hwnd, csFilename, L"Missing Shader File", MB_OK);
		}

		return false;
	}


	// Once the compute shader and code has successfully compiled into a buffer we then use this buffer to create the shader object
	// itself. We will use this pointer to interface with the compute shader from this point forward.

	// Create the compute shader from the buffer.
	// Shader-Objekt wird in den m_computeShader gefüllt
	result = device->CreateComputeShader(computeShaderBuffer->GetBufferPointer(), computeShaderBuffer->GetBufferSize(), NULL, &m_ComputeShader);
	if (FAILED(result))
	{
		return false;
	}


	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	computeShaderBuffer->Release();
	computeShaderBuffer = 0;

	return true;
}

// Buffererzeugung ist dadurch, dass man erst einen D3D11_BUFFER_DESC braucht etwas lang, deswegen in FUnktion gekapselt
ID3D11Buffer* CollisionDetectionManager::CreateStructuredBuffer(UINT count, UINT elementSize, UINT bindFlags, D3D11_USAGE usage, UINT cpuAccessFlags, D3D11_SUBRESOURCE_DATA *pData)
{
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = count * elementSize;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = elementSize;

	desc.BindFlags = bindFlags;
	desc.Usage = usage;
	desc.CPUAccessFlags = cpuAccessFlags;

	// Create the buffer with the specified configuration
	ID3D11Buffer* pBuffer = 0;
	HRESULT hr = device->CreateBuffer(&desc, pData, &pBuffer);
	return pBuffer;
}

// Erzeugt unorderd Access View für normalen Structured Buffer
ID3D11UnorderedAccessView* CollisionDetectionManager::CreateBufferUnorderedAccessView(ID3D11Resource* pResource, int elementCount)
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC desc;

	// For structured buffers, DXGI_FORMAT_ UNKNOWN must be used!
	// For standard buffers, utilize the appropriate format
	desc.Format = DXGI_FORMAT_UNKNOWN;

	// als was soll die Resource angesehen werden? ALs Buffer oder 1-3-dimensionale Texturen und 1-3-dimensionale Texturen-Arrays
	desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER; 
	desc.Buffer.FirstElement = 0;
	desc.Buffer.NumElements = elementCount;

	// desc.Buffer.Flags:
	// D3D11_BUFFER_UAV_FLAG_RAW: Resource contains raw, unstructured data.Requires the UAV format to be DXGI_FORMAT_R32_TYPELESS.
	// D3D11_BUFFER_UAV_FLAG_APPEND: Allow data to be appended to the end of the buffer. D3D11_BUFFER_UAV_FLAG_APPEND flag must 
	//								 also be used for any view that will be used as a AppendStructuredBuffer or a 
	//								 ConsumeStructuredBuffer. Requires the UAV format to be DXGI_FORMAT_UNKNOWN.
	// D3D11_BUFFER_UAV_FLAG_COUNTER: Adds a counter to the unordered-access-view buffer. D3D11_BUFFER_UAV_FLAG_COUNTER can only 
	//								  be used on a UAV that is a RWStructuredBuffer and it enables the functionality needed for 
	//								  the IncrementCounter and DecrementCounter methods in HLSL. Requires the UAV format to be DXGI_FORMAT_UNKNOWN.
	desc.Buffer.Flags = 0;

	ID3D11UnorderedAccessView* pView = 0;
	HRESULT hr = device->CreateUnorderedAccessView(pResource, &desc, &pView);

	return pView;
}

// Erzeugt unorderd Access View für normalen Structured Buffer
ID3D11ShaderResourceView* CollisionDetectionManager::CreateBufferShaderResourceView(ID3D11Resource* pResource, int elementCount)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC desc;

	// For structured buffers, DXGI_FORMAT_ UNKNOWN must be used!
	// For standard buffers, utilize the appropriate format
	desc.Format = DXGI_FORMAT_UNKNOWN;

	// als was soll die Resource angesehen werden? ALs Buffer oder 1-3-dimensionale Texturen und 1-3-dimensionale Texturen-Arrays
	desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	desc.Buffer.FirstElement = 0;
	desc.Buffer.NumElements = elementCount;

	ID3D11ShaderResourceView* pView = 0;
	HRESULT hr = device->CreateShaderResourceView(pResource, &desc, &pView);

	return pView;
}

void CollisionDetectionManager::OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename)
{
	char* compileErrors;
	unsigned long long bufferSize, i;
	ofstream fout;


	// Get a pointer to the error message text buffer.
	compileErrors = (char*)(errorMessage->GetBufferPointer());

	// Get the length of the message.
	bufferSize = errorMessage->GetBufferSize();

	// Open a file to write the error message to.
	//fout.open("shader-error.txt");

	// Write out the error message.
	for (i = 0; i < bufferSize; i++)
	{
		cout << compileErrors[i];
	}

	// Close the file.
	//fout.close();

	// Release the error message.
	errorMessage->Release();
	errorMessage = 0;

	// Pop a message up on the screen to notify the user to check the text file for compile errors.
	MessageBox(hwnd, L"Error compiling shader.  Check console output.txt for message.", shaderFilename, MB_OK);

	return;
}

// führe die Kollisionsberechnung für das aktuelle Frame durch
bool CollisionDetectionManager::Frame(vector<ModelClass*>* objects)
{
	auto begin = high_resolution_clock::now();
	CreateVertexAndTriangleArray(objects);
	D3D11_SUBRESOURCE_DATA vertex_SubresourceData = D3D11_SUBRESOURCE_DATA { m_Vertices, 0, 0 };
	D3D11_SUBRESOURCE_DATA triangle_SubresourceData = D3D11_SUBRESOURCE_DATA{ m_Triangles, 0, 0 };
	D3D11_SUBRESOURCE_DATA objectLastIndices_SubresourceData = D3D11_SUBRESOURCE_DATA{ m_ObjectsLastIndices, 0, 0 };

	// Buffer, ShaderResourceViews und UnorderedAccessViews müssen released werden (falls etwas in ihnen ist), bevor sie neu created werden!
	ReleaseBuffersAndViews();

	m_Vertex_Buffer = CreateStructuredBuffer(m_VertexCount, sizeof(Vertex), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE, 0, &vertex_SubresourceData);
	m_Triangle_Buffer = CreateStructuredBuffer(m_TriangleCount, sizeof(Triangle), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE, 0, &triangle_SubresourceData);
	m_ObjectsLastIndices_Buffer = CreateStructuredBuffer(m_ObjectCount, sizeof(UINT), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE, 0, &objectLastIndices_SubresourceData);
	m_BoundingBox_Buffer = CreateStructuredBuffer(m_TriangleCount, sizeof(BoundingBox), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL); 
	m_Result_Buffer = CreateStructuredBuffer(m_TriangleCount, sizeof(BoundingBox), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	// BoundingBox_Buffer und Result_Buffer werd ja im Shader befüllt, müssen also nicht mit Daten initialisiert werden

	m_Vertex_SRV = CreateBufferShaderResourceView(m_Vertex_Buffer, m_VertexCount);
	m_Triangle_SRV = CreateBufferShaderResourceView(m_Triangle_Buffer, m_TriangleCount);
	m_ObjectsLastIndices_SRV = CreateBufferShaderResourceView(m_ObjectsLastIndices_Buffer, m_ObjectCount);

	m_BoundingBox_UAV = CreateBufferUnorderedAccessView(m_BoundingBox_Buffer, m_TriangleCount);
	

	int xThreadGroups = (int)ceil(m_TriangleCount / 1024.0f);

	deviceContext->CSSetShaderResources(0, 1, &m_Vertex_SRV);
	deviceContext->CSSetShaderResources(1, 1, &m_Triangle_SRV);

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_BoundingBox_UAV, 0);

	deviceContext->CSSetShader(m_ComputeShader, NULL, 0);
	deviceContext->Dispatch(xThreadGroups, 1, 1);

	auto end = high_resolution_clock::now(); 
	// cout << "Buffer erzeugt, Dispatch erfolgt" << ": " << duration_cast<milliseconds>(end - begin).count() << "ms" << endl;

	begin = high_resolution_clock::now();

	// Daten von der GPU kopieren
	D3D11_MAPPED_SUBRESOURCE MappedResource = { 0 };
	deviceContext->CopyResource(m_Result_Buffer, m_BoundingBox_Buffer);
	HRESULT result = deviceContext->Map(m_Result_Buffer, 0, D3D11_MAP_READ, 0, &MappedResource);
	RETURN_FALSE_IF_FAIL(result);

	_Analysis_assume_(MappedResource.pData);
	assert(MappedResource.pData);
	// m_BoundingBoxes wird in CreateVertexAndTriangleArray neu initialisiert
	memcpy(m_BoundingBoxes, MappedResource.pData, m_TriangleCount * sizeof(BoundingBox));
	deviceContext->Unmap(m_Result_Buffer, 0);

	end = high_resolution_clock::now();
	// cout << "Buffer erzeugt, Dispatch erfolgt" << ": " << duration_cast<milliseconds>(end - begin).count() << "ms" << endl;
	
	return true;
}

#include "CollisionDetectionManager.h"



CollisionDetectionManager::CollisionDetectionManager()
{
	device = 0;
	deviceContext = 0;
	m_Vertices = 0;
	m_Triangles = 0;
	m_BoundingBoxes = 0;
	m_computeShader = 0;

	m_vertexBuffer = 0;
	m_triangleBuffer = 0;
	m_boundingBoxBuffer = 0;

	m_VertexShaderResourceView = 0;
	m_TriangleShaderResourceView = 0;
	m_BoundingBoxUnorderedAccessView = 0;
	m_ResultBuffer = 0;

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

void CollisionDetectionManager::CreateVertexAndTriangleArray(vector<ModelClass*>* objects)
{
	//Zuerst ausrechnen, wie groß das Triangle-Array sein sollte
	m_VertexCount = 0;
	for (ModelClass *aktModel : *objects) 
	{
		m_VertexCount += aktModel->GetIndexCount();
	}

	m_TriangleCount = m_VertexCount / 3;  // es gibt 3 Punkte pro Dreieck, also /3

	// vor dem neuen Speicher allokieren den alten freiegeben
	SAFEDELETEARRAY(m_Vertices);
	SAFEDELETEARRAY(m_Triangles);
	SAFEDELETEARRAY(m_BoundingBoxes);

	m_Vertices = new Vertex[m_VertexCount];
	m_Triangles = new Triangle[m_TriangleCount];
	m_BoundingBoxes = new BoundingBox[m_TriangleCount]; // eine Bounding Box pro Dreieck

	int aktGlobalPosition = 0;
	// gehe über alle Objekte
	for (ModelClass *aktModel : *objects)
	{
		VertexAndVertexDataType* modelData = aktModel->GetModelData(); // hole die rohen Objektdaten (ein Eintrag ist ein Punkt, seine Texturkoordinaten und Normale, wir wollen aber nur den Punkt)
		for (int i = 0; i < aktModel->GetIndexCount(); i++) // iteriere über jeden Vertex in modelData
		{
			m_Vertices[aktGlobalPosition] = { modelData[i].x,  modelData[i].y, modelData[i].z };
			if (aktGlobalPosition % 3 == 0)
			{
				// im Format dieses Tutorials sind die Punkte so aufgelistet, dass der Reihe nach durchgezählt wird, um auf die Dreiecke zu kommen
				int curTriangleIndex = aktGlobalPosition / 3;
				m_Triangles[curTriangleIndex] = { aktGlobalPosition,  aktGlobalPosition + 1, aktGlobalPosition + 2 };
			}
			aktGlobalPosition++;
		}
	}
}

void CollisionDetectionManager::Shutdown()
{
	SAFERELEASE(m_computeShader);

	SAFEDELETEARRAY(m_Vertices);
	SAFEDELETEARRAY(m_Triangles);
	SAFEDELETEARRAY(m_BoundingBoxes);

	SAFERELEASE(m_vertexBuffer);
	SAFERELEASE(m_triangleBuffer);
	SAFERELEASE(m_boundingBoxBuffer);

	SAFERELEASE(m_VertexShaderResourceView);
	SAFERELEASE(m_TriangleShaderResourceView);
	SAFERELEASE(m_BoundingBoxUnorderedAccessView);
	SAFERELEASE(m_ResultBuffer);
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
	result = D3DCompileFromFile(csFilename, NULL, NULL, "main", "cs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
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
	result = device->CreateComputeShader(computeShaderBuffer->GetBufferPointer(), computeShaderBuffer->GetBufferSize(), NULL, &m_computeShader);
	if (FAILED(result))
	{
		return false;
	}


	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	computeShaderBuffer->Release();
	computeShaderBuffer = 0;

	return true;
}

ID3D11Buffer* CollisionDetectionManager::CreateStructuredBuffer(UINT count, UINT structsize, UINT bindFlags, D3D11_USAGE usage, UINT cpuAccessFlags, D3D11_SUBRESOURCE_DATA *pData)
{
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = count * structsize;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = structsize;

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

void CollisionDetectionManager::RunComputeShader(ID3D11ComputeShader* computeShader, int uavCount, ID3D11UnorderedAccessView **unorderedAccessViews, int xThreadCount, int yThreadCount)
{
	deviceContext->CSSetShader(computeShader, NULL, 0);
	deviceContext->CSSetUnorderedAccessViews(0,						// Index of the first element in the zero-based array to begin setting (ranges from 0 to D3D11_1_UAV_SLOT_COUNT - 1). D3D11_1_UAV_SLOT_COUNT is defined as 64.
											 uavCount,				// Number of views to set (ranges from 0 to D3D11_1_UAV_SLOT_COUNT - StartSlot). 
											 unorderedAccessViews,	// A pointer to an array of ID3D11UnorderedAccessView pointers to be set by the method. 
											 0);					// An array of append and consume buffer offsets. A value of -1 indicates to keep the 
																	//current offset. Any other values set the hidden counter for that appendable and 
																	// consumable UAV. pUAVInitialCounts is only relevant for UAVs that were created 
																	// with either D3D11_BUFFER_UAV_FLAG_APPEND or D3D11_BUFFER_UAV_FLAG_COUNTER 
																	// specified when the UAV was created; otherwise, the argument is ignored. 
	deviceContext->Dispatch(xThreadCount, yThreadCount, 1);

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
	fout.open("shader-error.txt");

	// Write out the error message.
	for (i = 0; i < bufferSize; i++)
	{
		fout << compileErrors[i];
	}

	// Close the file.
	fout.close();

	// Release the error message.
	errorMessage->Release();
	errorMessage = 0;

	// Pop a message up on the screen to notify the user to check the text file for compile errors.
	MessageBox(hwnd, L"Error compiling shader.  Check shader-error.txt for message.", shaderFilename, MB_OK);

	return;
}

void CollisionDetectionManager::Frame(vector<ModelClass*>* objects)
{
	CreateVertexAndTriangleArray(objects);
	D3D11_SUBRESOURCE_DATA vertexSubresourceData = D3D11_SUBRESOURCE_DATA { m_Vertices, 0, 0 };
	D3D11_SUBRESOURCE_DATA triangleSubresourceData = D3D11_SUBRESOURCE_DATA{ m_Triangles, 0, 0 };

	// Buffer müssen released werden (falls etwas in ihnen ist), bevor sie neu created werden
	SAFERELEASE(m_vertexBuffer);
	SAFERELEASE(m_triangleBuffer);
	SAFERELEASE(m_boundingBoxBuffer);
	SAFERELEASE(m_ResultBuffer);

	m_vertexBuffer = CreateStructuredBuffer(m_VertexCount, sizeof(Vertex), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, &vertexSubresourceData);
	m_triangleBuffer = CreateStructuredBuffer(m_TriangleCount, sizeof(Triangle), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, &vertexSubresourceData);
	m_boundingBoxBuffer = CreateStructuredBuffer(m_TriangleCount, sizeof(BoundingBox), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL); 
	m_ResultBuffer = CreateStructuredBuffer(m_TriangleCount, sizeof(BoundingBox), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	// Bounding Box wird ja im Shader befüllt, muss also nicht mit Daten initialisiert werden

	// auch ShaderResourceViews und UnorderedAccessViews müssen released werden!
	SAFERELEASE(m_VertexShaderResourceView);
	SAFERELEASE(m_TriangleShaderResourceView);
	SAFERELEASE(m_BoundingBoxUnorderedAccessView);

	m_VertexShaderResourceView = CreateBufferShaderResourceView(m_vertexBuffer, m_VertexCount);
	m_TriangleShaderResourceView = CreateBufferShaderResourceView(m_triangleBuffer, m_TriangleCount);
	m_BoundingBoxUnorderedAccessView = CreateBufferUnorderedAccessView(m_boundingBoxBuffer, m_TriangleCount);
	
	int xThreadGroups = (int)ceil(m_TriangleCount / 1024.0f);

	deviceContext->CSSetShaderResources(0, 1, &m_VertexShaderResourceView);
	deviceContext->CSSetShaderResources(0, 1, &m_TriangleShaderResourceView);
	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_BoundingBoxUnorderedAccessView, 0);

	deviceContext->CSSetShader(m_computeShader, NULL, 0);
	deviceContext->Dispatch(xThreadGroups, 1, 1);

	// Daten von der GPU kopieren
	D3D11_MAPPED_SUBRESOURCE MappedResource = { 0 };
	deviceContext->CopyResource(m_ResultBuffer, m_boundingBoxBuffer);
	HRESULT result = deviceContext->Map(m_ResultBuffer, 0, D3D11_MAP_READ, 0, &MappedResource);
	if (FAILED(result))
	{
		return;
	}
	_Analysis_assume_(MappedResource.pData);
	assert(MappedResource.pData);
	// m_BoundingBoxes wird in CreateVertexAndTriangleArray neu initialisiert
	memcpy(m_BoundingBoxes, MappedResource.pData, m_TriangleCount * sizeof(BoundingBox));
	deviceContext->Unmap(m_ResultBuffer, 0);
}

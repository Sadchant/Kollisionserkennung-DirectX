#include "CollisionDetectionManager.h"



CollisionDetectionManager::CollisionDetectionManager()
{
	device = 0;
	deviceContext = 0;
	m_Vertices = 0;
	m_Triangles = 0;
	m_ObjectsLastIndices = 0;
	m_Results1 = 0;
	m_Results2_1 = 0;
	m_Results2_2 = 0;
	m_Results3 = 0;
	m_Results4 = 0;
	m_Results5_1 = 0;
	m_Results5_2 = 0;
	m_Results6 = 0;
	m_Results7 = 0;

	m_curComputeShader = 0;

	m_Vertex_Buffer = 0;
	m_Triangle_Buffer = 0;
	m_ObjectsLastIndices_Buffer = 0;
	m_BoundingBox_Buffer = 0;
	m_GroupMinPoint_Buffer = 0;
	m_GroupMaxPoint_Buffer = 0;
	m_CounterTrees_Buffer = 0;
	m_GlobalCounterTree_Buffer = 0;
	m_TypeTree_Buffer = 0;
	m_LeafIndexTree_Buffer = 0;
	m_CellTrianglePairs_Buffer = 0;

	m_ReduceData_CBuffer = 0;
	m_ObjectCount_CBuffer = 0;
	m_TreeSizeInLevel_CBuffer = 0;
	m_StartLevel_CBuffer = 0;
	m_Loops_CBuffer = 0;

	m_Result_Buffer1 = 0;
	m_Result_Buffer2_1 = 0;
	m_Result_Buffer2_2 = 0;
	m_Result_Buffer3 = 0;
	m_Result_Buffer4 = 0;
	m_Result_Buffer5_1 = 0;
	m_Result_Buffer5_2 = 0;
	m_Result_Buffer6 = 0;
	m_Result_Buffer7 = 0;

	m_NULL_SRV = NULL;
	m_NULL_UAV = NULL;

	m_Vertex_SRV = 0;
	m_Triangle_SRV = 0;
	m_ObjectsLastIndices_SRV = 0;

	m_BoundingBox_UAV = 0;
	m_GroupMinPoint_UAV = 0;
	m_GroupMaxPoint_UAV = 0;
	m_CounterTrees_UAV = 0;
	m_GlobalCounterTree_UAV = 0;
	m_TypeTree_UAV = 0;
	m_LeafIndexTree_UAV = 0;
	m_CellTrianglePairs_UAV = 0;
}


CollisionDetectionManager::~CollisionDetectionManager()
{
}

void CollisionDetectionManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, vector<ModelClass*>* objects)
{
	this->device = device;
	this->deviceContext = deviceContext;

	m_ReduceData_CBuffer = CreateConstantBuffer(sizeof(ReduceData), D3D11_USAGE_DEFAULT, NULL);
	m_StartLevel_CBuffer = CreateConstantBuffer(sizeof(SingleUINT), D3D11_USAGE_DEFAULT, NULL);
	m_Loops_CBuffer = CreateConstantBuffer(sizeof(SingleUINT), D3D11_USAGE_DEFAULT, NULL);
	m_hwnd = hwnd;
	InitComputeShaderVector();

	m_TreeSize = 0;
	// Berechne aus LEVELS die TreeSize (es wird die Anzahl der Zellen pro Level zusammengerechnet)
	for (int i = 0; i <= SUBDIVS; i++)
	{
		m_TreeSize += (int)pow(8, i); // es gibt pro Level 8 hoch aktuelles Level Unterteilungen
		m_TreeSizeInLevel[i] = m_TreeSize;
	}

	CreateVertexAndTriangleArray(objects);
	CreateSceneBuffersAndViews();
}

// befülle m_ComputeShaderVector mit allen Compute Shadern
void CollisionDetectionManager::InitComputeShaderVector()
{
	ID3D11ComputeShader* pTempComputeShader;
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/1_BoundingBoxes_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/2_SceneBoundingBox_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/3_FillCounterTrees_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/4_GlobalCounterTree_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/5_FillTypeTree_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/6_FillLeafIndexTree_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
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
	SAFEDELETEARRAY(m_ObjectsLastIndices);

	m_Vertices = new Vertex[m_VertexCount];
	m_Triangles = new Triangle[m_TriangleCount];
	m_ObjectsLastIndices = new UINT[m_ObjectCount]; // es gibt so viele Einträge wie Objekte


	int curGlobalPosition = 0;
	int curLastIndex = 0;
	// gehe über alle Objekte
	for (int i = 0; i < m_ObjectCount; i++)
	{
		ModelClass *curModel = (*objects)[i];
		VertexAndVertexDataType* curModelData = curModel->GetModelData(); // hole die rohen Objektdaten (ein Eintrag ist ein Punkt, seine Texturkoordinaten und Normale, wir wollen aber nur den Punkt)
		// iteriere über jeden Vertex in modelData

		int curModelIndexCount = curModel->GetIndexCount();
		for (int j = 0; j < curModelIndexCount; j++)
		{
			m_Vertices[curGlobalPosition] = { curModelData[j].x,  curModelData[j].y, curModelData[j].z };
			if (curGlobalPosition % 3 == 0)
			{
				// im Format dieses Tutorials sind die Punkte so aufgelistet, dass der Reihe nach durchgezählt wird, um auf die Dreiecke zu kommen
				int curTriangleIndex = curGlobalPosition / 3;
				m_Triangles[curTriangleIndex] = { { curGlobalPosition,  curGlobalPosition + 1, curGlobalPosition + 2} };
			}
			curGlobalPosition++;
		}
		// schreibe außerdem den letzten Index des Objektes in m_objectsLastIndices
		curLastIndex += curModel->GetIndexCount() / 3; // Die lastIndexe beziehen sich auf Dreieck-IDS, also / 3
		m_ObjectsLastIndices[i] = curLastIndex - 1;
	}
	m_CellTrianglePairsCount = m_TriangleCount * 3;
}

// erzeugt alle Buffer, die bei Hinzufügen/Löschen von Objekten in der Szene ihre Größe ändern
void CollisionDetectionManager::CreateSceneBuffersAndViews()
{
	D3D11_SUBRESOURCE_DATA vertex_SubresourceData = D3D11_SUBRESOURCE_DATA{ m_Vertices, 0, 0 };
	D3D11_SUBRESOURCE_DATA triangle_SubresourceData = D3D11_SUBRESOURCE_DATA{ m_Triangles, 0, 0 };
	D3D11_SUBRESOURCE_DATA objectsLastIndices_SubresourceData = D3D11_SUBRESOURCE_DATA{ m_ObjectsLastIndices, 0, 0 };

	// Buffer, ShaderResourceViews und UnorderedAccessViews müssen released werden (falls etwas in ihnen ist), bevor sie neu created werden!
	ReleaseBuffersAndViews();
	m_Vertex_Buffer = CreateStructuredBuffer(m_VertexCount, sizeof(Vertex), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE, 0, &vertex_SubresourceData);
	m_Triangle_Buffer = CreateStructuredBuffer(m_TriangleCount, sizeof(Triangle), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE, 0, &triangle_SubresourceData);
	m_ObjectsLastIndices_Buffer = CreateStructuredBuffer(m_ObjectCount, sizeof(UINT), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE, 0, &objectsLastIndices_SubresourceData);
	m_BoundingBox_Buffer = CreateStructuredBuffer(m_TriangleCount, sizeof(BoundingBox), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);
	// BoundingBox_Buffer und Result_Buffer werd ja im Shader befüllt, müssen also nicht mit Daten initialisiert werdenS

	m_GroupResult_Count = (int)ceil((float)m_VertexCount / (2 * _2_SCENEBOUNDINGBOX_XTHREADS));
	m_GroupMinPoint_Buffer = CreateStructuredBuffer(m_GroupResult_Count, sizeof(Vertex), D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);
	m_GroupMaxPoint_Buffer = CreateStructuredBuffer(m_GroupResult_Count, sizeof(Vertex), D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);

	SingleUINT s_ObjectCount;
	TreeSizeInLevel s_TreeSizeInLevel;
	// Berechne aus LEVELS die TreeSize (es wird die Anzahl der Zellen pro Level zusammengerechnet)
	for (int i = 0; i <= SUBDIVS; i++)
	{
		s_TreeSizeInLevel.treeSizeInLevel[i] = { m_TreeSizeInLevel[i], 0, 0, 0 };
	}
	s_ObjectCount.value = (UINT)m_ObjectCount;
	D3D11_SUBRESOURCE_DATA objectCount_SubresourceData = D3D11_SUBRESOURCE_DATA{ &s_ObjectCount, 0, 0 };
	D3D11_SUBRESOURCE_DATA treeSizeInLevel_SubresourceData = D3D11_SUBRESOURCE_DATA{ &s_TreeSizeInLevel, 0, 0 };

	m_CounterTreesSize = m_ObjectCount*m_TreeSize;
	UINT* counterTrees_0s = new UINT[m_CounterTreesSize]{ 0 }; // Dient nur dazu, den counterTrees-Buffer mit 0en zu füllen
	D3D11_SUBRESOURCE_DATA counterTrees_SubresourceData = D3D11_SUBRESOURCE_DATA{ counterTrees_0s, 0, 0 };

	m_CounterTrees_Buffer = CreateStructuredBuffer(m_CounterTreesSize, sizeof(UINT), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, &counterTrees_SubresourceData);
	
	m_ObjectCount_CBuffer = CreateConstantBuffer(sizeof(SingleUINT), D3D11_USAGE_IMMUTABLE, &objectCount_SubresourceData);
	m_TreeSizeInLevel_CBuffer = CreateConstantBuffer(sizeof(TreeSizeInLevel), D3D11_USAGE_IMMUTABLE, &treeSizeInLevel_SubresourceData);
	
	SAFEDELETEARRAY(counterTrees_0s);

	m_GlobalCounterTree_Buffer = CreateStructuredBuffer(m_TreeSize, sizeof(UINT), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);
	
	m_TypeTree_Buffer = CreateStructuredBuffer(m_TreeSize, sizeof(UINT), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);
	m_LeafIndexTree_Buffer = CreateStructuredBuffer(m_TreeSize, sizeof(UINT), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);
	m_CellTrianglePairs_Buffer = CreateStructuredBuffer(m_CellTrianglePairsCount, sizeof(CellTrianglePair), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);


	m_Result_Buffer1 = CreateStructuredBuffer(m_TriangleCount, sizeof(BoundingBox), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer2_1 = CreateStructuredBuffer(m_GroupResult_Count, sizeof(Vertex), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer2_2 = CreateStructuredBuffer(m_GroupResult_Count, sizeof(Vertex), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer3 = CreateStructuredBuffer(m_CounterTreesSize, sizeof(UINT), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer4 = CreateStructuredBuffer(m_TreeSize, sizeof(UINT), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer5_1 = CreateStructuredBuffer(m_TreeSize, sizeof(UINT), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer5_2 = CreateStructuredBuffer(m_TreeSize, sizeof(UINT), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer6 = CreateStructuredBuffer(m_TreeSize, sizeof(UINT), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer7 = CreateStructuredBuffer(m_CellTrianglePairsCount, sizeof(CellTrianglePair), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);

	m_Vertex_SRV = CreateBufferShaderResourceView(m_Vertex_Buffer, m_VertexCount);
	m_Triangle_SRV = CreateBufferShaderResourceView(m_Triangle_Buffer, m_TriangleCount);
	m_ObjectsLastIndices_SRV = CreateBufferShaderResourceView(m_ObjectsLastIndices_Buffer, m_ObjectCount);

	m_BoundingBox_UAV = CreateBufferUnorderedAccessView(m_BoundingBox_Buffer, m_TriangleCount);
	m_GroupMinPoint_UAV = CreateBufferUnorderedAccessView(m_GroupMinPoint_Buffer, m_GroupResult_Count);
	m_GroupMaxPoint_UAV = CreateBufferUnorderedAccessView(m_GroupMaxPoint_Buffer, m_GroupResult_Count);
	m_CounterTrees_UAV = CreateBufferUnorderedAccessView(m_CounterTrees_Buffer, m_ObjectCount*m_TreeSize);
	m_GlobalCounterTree_UAV = CreateBufferUnorderedAccessView(m_GlobalCounterTree_Buffer, m_TreeSize);
	m_TypeTree_UAV = CreateBufferUnorderedAccessView(m_TypeTree_Buffer, m_TreeSize);
	m_LeafIndexTree_UAV = CreateBufferUnorderedAccessView(m_LeafIndexTree_Buffer, m_TreeSize);
	m_CellTrianglePairs_UAV = CreateBufferUnorderedAccessView(m_CellTrianglePairs_Buffer, m_CellTrianglePairsCount, D3D11_BUFFER_UAV_FLAG_APPEND);
}

// gib alle Buffer, Shader Resource Views und Unordered Access Views frei
void CollisionDetectionManager::ReleaseBuffersAndViews()
{
	SAFERELEASE(m_Vertex_Buffer);
	SAFERELEASE(m_Triangle_Buffer);
	SAFERELEASE(m_ObjectsLastIndices_Buffer);
	SAFERELEASE(m_BoundingBox_Buffer);
	SAFERELEASE(m_GroupMinPoint_Buffer);
	SAFERELEASE(m_GroupMaxPoint_Buffer);
	SAFERELEASE(m_CounterTrees_Buffer);
	SAFERELEASE(m_GlobalCounterTree_Buffer);
	SAFERELEASE(m_TypeTree_Buffer);
	SAFERELEASE(m_LeafIndexTree_Buffer);
	SAFERELEASE(m_CellTrianglePairs_Buffer);
	// Constant Buffer wird nur einmal erzeugt (weil sich seine Größe nicht ändert), also nur in Shutdown released!
	SAFERELEASE(m_Result_Buffer1);
	SAFERELEASE(m_Result_Buffer2_1);
	SAFERELEASE(m_Result_Buffer2_2);
	SAFERELEASE(m_Result_Buffer3);
	SAFERELEASE(m_Result_Buffer4);
	SAFERELEASE(m_Result_Buffer5_1);
	SAFERELEASE(m_Result_Buffer5_2);
	SAFERELEASE(m_Result_Buffer6);
	SAFERELEASE(m_Result_Buffer7);

	SAFERELEASE(m_Vertex_SRV);
	SAFERELEASE(m_Triangle_SRV);
	SAFERELEASE(m_ObjectsLastIndices_SRV);

	SAFERELEASE(m_BoundingBox_UAV);
	SAFERELEASE(m_GroupMinPoint_UAV);
	SAFERELEASE(m_GroupMaxPoint_UAV);
	SAFERELEASE(m_CounterTrees_UAV);
	SAFERELEASE(m_GlobalCounterTree_UAV);
	SAFERELEASE(m_TypeTree_UAV);
	SAFERELEASE(m_LeafIndexTree_UAV);
	SAFERELEASE(m_CellTrianglePairs_UAV);
	
}

void CollisionDetectionManager::Shutdown()
{
	SAFEDELETEARRAY(m_Vertices);
	SAFEDELETEARRAY(m_Triangles);
	SAFEDELETEARRAY(m_ObjectsLastIndices);
	SAFEDELETEARRAY(m_Results1);
	SAFEDELETEARRAY(m_Results2_1);
	SAFEDELETEARRAY(m_Results2_2);
	SAFEDELETEARRAY(m_Results3);
	SAFEDELETEARRAY(m_Results4);
	SAFEDELETEARRAY(m_Results5_1);
	SAFEDELETEARRAY(m_Results5_2);
	SAFEDELETEARRAY(m_Results6);
	SAFEDELETEARRAY(m_Results7);

	SAFERELEASE(m_ReduceData_CBuffer);
	SAFERELEASE(m_ObjectCount_CBuffer);
	SAFERELEASE(m_TreeSizeInLevel_CBuffer);
	SAFERELEASE(m_StartLevel_CBuffer);
	SAFERELEASE(m_Loops_CBuffer);

	ReleaseBuffersAndViews();

	// den Vector mit den Compute Shadern durchlaufen, alle releasen und danach den Vector leeren
	for (auto pCurComputeShader : m_ComputeShaderVector)
	{
		pCurComputeShader->Release();
	}
	m_ComputeShaderVector.clear();
}

ID3D11ComputeShader* CollisionDetectionManager::CreateComputeShader(WCHAR* csFilename)
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

	// Compile the compute shader code, D3D_COMPILE_STANDARD_FILE_INCLUDE bewirkt, dass man "#include" in hlsl benutzen kann
	result = D3DCompileFromFile(csFilename, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "cs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &computeShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, m_hwnd, csFilename);
		}
		// If there was  nothing in the error message then it simply could not find the shader file itself.
		else
		{
			MessageBox(m_hwnd, csFilename, L"Missing Shader File", MB_OK);
		}

		return NULL;
	}

	ID3D11ComputeShader* pComputeShader;

	// Once the compute shader and code has successfully compiled into a buffer we then use this buffer to create the shader object
	// itself. We will use this pointer to interface with the compute shader from this point forward.

	// Create the compute shader from the buffer.
	// Shader-Objekt wird in den m_computeShader gefüllt
	result = device->CreateComputeShader(computeShaderBuffer->GetBufferPointer(), computeShaderBuffer->GetBufferSize(), NULL, &pComputeShader);
	if (FAILED(result))
	{
		cout << "CreateComputeShader() Fehler!" << endl;
	}


	// Release error message and shader buffer
	SAFERELEASE(errorMessage);
	SAFERELEASE(computeShaderBuffer);

	return pComputeShader;
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

ID3D11Buffer* CollisionDetectionManager::CreateConstantBuffer(UINT elementSize, D3D11_USAGE usage, D3D11_SUBRESOURCE_DATA *pData)
{
	D3D11_BUFFER_DESC constant_buffer_desc;
	ZeroMemory(&constant_buffer_desc, sizeof(constant_buffer_desc));
	constant_buffer_desc.ByteWidth = elementSize;
	constant_buffer_desc.Usage = usage;
	constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constant_buffer_desc.CPUAccessFlags = 0;

	ID3D11Buffer* pBuffer = 0;
	HRESULT hr = device->CreateBuffer(&constant_buffer_desc, pData, &pBuffer);
	if (FAILED(hr))
		cout << "Fehler beim Erzeugen von Constant Buffer" << endl;
	return pBuffer;
}

// Erzeugt unorderd Access View für normalen Structured Buffer
ID3D11UnorderedAccessView* CollisionDetectionManager::CreateBufferUnorderedAccessView(ID3D11Resource* pResource, int elementCount)
{
	// der letzte ist sozusagen ein optionaler Parameter, wenn man den nicht angibt wird er automatisch auf 0 gesetzt
	return CreateBufferUnorderedAccessView(pResource, elementCount, 0);
}

// Erzeugt unorderd Access View für normalen Structured Buffer
ID3D11UnorderedAccessView* CollisionDetectionManager::CreateBufferUnorderedAccessView(ID3D11Resource* pResource, int elementCount, UINT flags)
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
	desc.Buffer.Flags = flags;

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
	MessageBox(hwnd, L"Error compiling shader.  Check console output for message.", shaderFilename, MB_OK);

	return;
}

// ******* 1. Berechne Bounding Boxes für jedes Dreieck *******
void CollisionDetectionManager::_1_BoundingBoxes()
{
	int xThreadGroups = (int)ceil(m_TriangleCount / 1024.0f);

	deviceContext->CSSetShaderResources(0, 1, &m_Vertex_SRV);
	deviceContext->CSSetShaderResources(1, 1, &m_Triangle_SRV);

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_BoundingBox_UAV, 0);

	m_curComputeShader = m_ComputeShaderVector[0];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);
	deviceContext->Dispatch(xThreadGroups, 1, 1);
}

// ******* 2. Berechne Bounding Box für die gesamte Szene *******
void CollisionDetectionManager::_2_SceneCoundingBox()
{
	m_curComputeShader = m_ComputeShaderVector[1];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

	deviceContext->CSSetShaderResources(0, 1, &m_Vertex_SRV);
	deviceContext->CSSetShaderResources(1, 1, &m_Vertex_SRV);
	// GroupMinMaxPoint: Output im ersten Durchgang, Input und Output in allen anderen
	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_GroupMinPoint_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_GroupMaxPoint_UAV, 0);

	int threadCount, groupCount, firstStepStride, inputSize;
	bool firstStep = true;
	bool outputIsInput = false; // teilt den Shader mit, dass er nicht mehr aus Vertices liest und stattdessen den Output als Input benutzen soll

								// Hier die einzigartige und selten gesehene do-while-Schleife:
	do
	{
		// im allerersten Durchlauf werden die Vertices als Input für die Berechnungen der Szenen-BoundingBox benutzt
		// der ThreadCount (wie viele Threads werden praktisch benötigt?) berechnet sich aus vertexCount
		if (firstStep)
		{
			threadCount = (int)ceil(m_VertexCount / 2.0);
			inputSize = m_VertexCount;
			firstStep = false;
		}
		// ansonsten wird als Input das Ergebnis des letzten Schleifen-Durchlaufs in den groupMin/MaxPoint_Buffern benutzt
		// der ThreadCount berechnet sich aus der Anzahl der Gruppen im letzten Schleifen-Durchlauf
		else
		{
			threadCount = (int)ceil(groupCount / 2.0);
			inputSize = groupCount; // ab dem zweiten Durchlauf werden ja gruppenErgebnisse weiterverarbeitet, also entspricht die InputSize dem letzten groupCount
			outputIsInput = true; // alle außer dem ersten Durchlauf dürfen den Input manipulieren
		}
		groupCount = (int)ceil((float)threadCount / _2_SCENEBOUNDINGBOX_XTHREADS);
		firstStepStride = groupCount * _2_SCENEBOUNDINGBOX_XTHREADS;
		// Struct für den Constant Buffer
		ReduceData reduceData = { firstStepStride, inputSize, outputIsInput };
		deviceContext->UpdateSubresource(m_ReduceData_CBuffer, 0, NULL, &reduceData, 0, 0);
		deviceContext->CSSetConstantBuffers(0, 1, &m_ReduceData_CBuffer);
		deviceContext->Dispatch(groupCount, 1, 1);
	} while (groupCount > 1);
	// solange mehr als eine Gruppe gestartet werden muss, werden die Min-MaxPoints nicht auf ein Ergebnis reduziert sein,
	// da es ja immer ein Ergebnis pro Gruppe berechnet wird
}

// ******* 3. Befülle Countertrees mit den Daten für jedes Objekt *******
void CollisionDetectionManager::_3_FillCounterTrees()
{
	m_curComputeShader = m_ComputeShaderVector[2];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_GroupMinPoint_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_GroupMaxPoint_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(2, 1, &m_BoundingBox_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(3, 1, &m_CounterTrees_UAV, 0);

	deviceContext->CSSetShaderResources(0, 1, &m_ObjectsLastIndices_SRV);
	deviceContext->CSSetConstantBuffers(0, 1, &m_ObjectCount_CBuffer);
	deviceContext->CSSetConstantBuffers(1, 1, &m_TreeSizeInLevel_CBuffer);
	int xThreadGroups = (int)ceil(m_TriangleCount / 1024.0f);
	deviceContext->Dispatch(xThreadGroups, 1, 1);

	// entferne die UAVs wieder von den Slots 0 - 2, damit sie wieder verwendet werden können
	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_NULL_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_NULL_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(2, 1, &m_NULL_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(3, 1, &m_NULL_UAV, 0);
}

// ******* 4.Befülle den GlobalCounterTree mit den Daten, wie viele Überschneidungstets es pro Zelle gibt *******
void CollisionDetectionManager::_4_GlobalCounterTree()
{
	m_curComputeShader = m_ComputeShaderVector[3];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_CounterTrees_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_GlobalCounterTree_UAV, 0);
	deviceContext->CSSetConstantBuffers(0, 1, &m_ObjectCount_CBuffer);
	deviceContext->CSSetConstantBuffers(1, 1, &m_TreeSizeInLevel_CBuffer);
	int xThreadGroups = (int)ceil(m_TreeSize / 1024.0f);
	deviceContext->Dispatch(xThreadGroups, 1, 1);

	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_NULL_UAV, 0);
}

// ******* 5. Trage im GlobalCounterTree die Werte der optimierten Struktur ein und speichere die Struktur in TypeTree *******
void CollisionDetectionManager::_5_FillTypeTree()
{
	m_curComputeShader = m_ComputeShaderVector[4];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_GlobalCounterTree_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_TypeTree_UAV, 0);
	deviceContext->CSSetConstantBuffers(0, 1, &m_TreeSizeInLevel_CBuffer);


	int _3DGroupCount, curStartLevel, curLevelResolution;
	// es gibt immer einen Level mehr als SUBDIVS, also wird mit dem Index SUBDIVS auf den letzten Level zugegriffen,
	// wir suchen den vorletzten Level, also SUBDIVS - 1!
	curStartLevel = SUBDIVS - 1;
	while (curStartLevel >= 0)
	{
		curLevelResolution = (int)pow(8, curStartLevel); // 8 hoch den aktuellen Level ergibt die Auflösung für den Level
													// die dritte Wurzel von der aktuellen Auflösung geteilt durch 512 ergibt die Anzahl an Gruppen die erzeugt werden müssen pro Dimension
		_3DGroupCount = (int)ceil(pow(curLevelResolution / 512.0, 1.0 / 3.0));

		// Constant Buffer updaten
		SingleUINT s_StartLevel = { (UINT)curStartLevel };
		deviceContext->UpdateSubresource(m_StartLevel_CBuffer, 0, NULL, &s_StartLevel, 0, 0);
		deviceContext->CSSetConstantBuffers(1, 1, &m_StartLevel_CBuffer);

		deviceContext->Dispatch(_3DGroupCount, _3DGroupCount, _3DGroupCount);

		curStartLevel -= 4; // der Shader kann mit 512 Threads / Gruppe die Eingabemenge um die Größe 4 reduzieren
	}
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_NULL_UAV, 0);

}

// ******* 6. Ziehe die Indexe der Zellen bis aufs unterste Level, die Blattzellen sind *******
void CollisionDetectionManager::_6_FillLeafIndexTree()
{
	m_curComputeShader = m_ComputeShaderVector[5];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_TypeTree_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_LeafIndexTree_UAV, 0);
	deviceContext->CSSetConstantBuffers(0, 1, &m_TreeSizeInLevel_CBuffer);


	int _3DGroupCount, curStartLevel, curLevelResolution, firstDispatchLoops, curLoops;
	bool firstStep = true;
	// berechne curLoops, für den ersten Dispatch ist das wichtig!
	firstDispatchLoops = SUBDIVS % 4;
	if (firstDispatchLoops == 0)
		firstDispatchLoops = 4;

	// es gibt immer einen Level mehr als SUBDIVS, also wird mit dem Index SUBDIVS auf den letzten Level zugegriffen,
	// wir suchen den vorletzten Level, also SUBDIVS - 1!
	curStartLevel = 0;
	while (curStartLevel < SUBDIVS)
	{
		if (firstStep)
		{
			curLoops = firstDispatchLoops;
			firstStep = false;
		}
		else
			curLoops = 4;

		// rechne curStartLevel + 4, da die Resolution benutzt wird um zu ermitteln, wie viele Threads gespawnt werden sollen
		// es werden aber so viele Threads gebraucht, wie es Zellen im viertnächsten Level gibt
		curLevelResolution = (int)pow(8, curStartLevel + 3); // 8 hoch den aktuellen Level ergibt die Auflösung für den Level
													// die dritte Wurzel von der aktuellen Auflösung geteilt durch 512 ergibt die Anzahl an Gruppen die erzeugt werden müssen pro Dimension
		_3DGroupCount = (int)ceil(pow(curLevelResolution / 512.0, 1.0 / 3.0));

		// Constant Buffer updaten
		SingleUINT s_StartLevel = { (UINT)curStartLevel };
		SingleUINT s_Loops = { (UINT)curLoops };
		deviceContext->UpdateSubresource(m_StartLevel_CBuffer, 0, NULL, &s_StartLevel, 0, 0);
		deviceContext->UpdateSubresource(m_Loops_CBuffer, 0, NULL, &s_Loops, 0, 0);
		deviceContext->CSSetConstantBuffers(1, 1, &m_StartLevel_CBuffer);
		deviceContext->CSSetConstantBuffers(2, 1, &m_Loops_CBuffer);

		deviceContext->Dispatch(_3DGroupCount, _3DGroupCount, _3DGroupCount);

		curStartLevel += curLoops; // der Shader kann mit 512 Threads / Gruppe die Eingabemenge um die Größe 4 reduzieren
	}
}




// führe die Kollisionsberechnung für das aktuelle Frame durch
void CollisionDetectionManager::Frame()
{
	//auto begin = high_resolution_clock::now();

	//auto end = high_resolution_clock::now();
	//cout << "Buffer created" << ": " << duration_cast<milliseconds>(end - begin).count() << "ms" << endl;

	_1_BoundingBoxes();
	//_1_BoundingBoxes_GetResult();

	_2_SceneCoundingBox();
	//_2_SceneCoundingBox_GetResult();

	_3_FillCounterTrees();
	//_3_FillCounterTrees_GetResult();

	_4_GlobalCounterTree();
	//_4_GlobalCounterTree_GetResult();

	_5_FillTypeTree();
	//_5_FillTypeTree_GetResult();

	_6_FillLeafIndexTree();
	//_6_FillLeafIndexTree_GetResult();
}


void CollisionDetectionManager::_1_BoundingBoxes_GetResult()
{

}

void CollisionDetectionManager::_2_SceneCoundingBox_GetResult()
{
	HRESULT result;
	SAFEDELETEARRAY(m_Results2_1);
	SAFEDELETEARRAY(m_Results2_2);

	m_Results2_1 = new Vertex[m_GroupResult_Count];
	m_Results2_2 = new Vertex[m_GroupResult_Count];

	D3D11_MAPPED_SUBRESOURCE MappedResource1 = { 0 };
	D3D11_MAPPED_SUBRESOURCE MappedResource2 = { 0 };
	deviceContext->CopyResource(m_Result_Buffer2_1, m_GroupMinPoint_Buffer);
	deviceContext->CopyResource(m_Result_Buffer2_2, m_GroupMaxPoint_Buffer);
	result = deviceContext->Map(m_Result_Buffer2_1, 0, D3D11_MAP_READ, 0, &MappedResource1);
	result = deviceContext->Map(m_Result_Buffer2_2, 0, D3D11_MAP_READ, 0, &MappedResource2);

	_Analysis_assume_(MappedResource1.pData);
	assert(MappedResource1.pData);

	_Analysis_assume_(MappedResource2.pData);
	assert(MappedResource2.pData);
	// m_BoundingBoxes wird in CreateVertexAndTriangleArray neu initialisiert
	memcpy(m_Results2_1, MappedResource1.pData, m_GroupResult_Count * sizeof(Vertex));
	memcpy(m_Results2_2, MappedResource2.pData, m_GroupResult_Count * sizeof(Vertex));
	deviceContext->Unmap(m_Result_Buffer2_1, 0);
	deviceContext->Unmap(m_Result_Buffer2_2, 0);
}

void CollisionDetectionManager::_3_FillCounterTrees_GetResult()
{
	SAFEDELETEARRAY(m_Results3);
	m_Results3 = new UINT[m_CounterTreesSize];
	D3D11_MAPPED_SUBRESOURCE MappedResource3 = { 0 };
	deviceContext->CopyResource(m_Result_Buffer3, m_CounterTrees_Buffer);
	deviceContext->Map(m_Result_Buffer3, 0, D3D11_MAP_READ, 0, &MappedResource3);
	_Analysis_assume_(MappedResource3.pData);
	assert(MappedResource3.pData);
	// m_BoundingBoxes wird in CreateVertexAndTriangleArray neu initialisiert
	memcpy(m_Results3, MappedResource3.pData, m_CounterTreesSize * sizeof(UINT));
	deviceContext->Unmap(m_Result_Buffer3, 0);
}

void CollisionDetectionManager::_4_GlobalCounterTree_GetResult()
{
	SAFEDELETEARRAY(m_Results4);
	m_Results4 = new UINT[m_TreeSize];
	D3D11_MAPPED_SUBRESOURCE MappedResource4 = { 0 };
	deviceContext->CopyResource(m_Result_Buffer4, m_GlobalCounterTree_Buffer);
	deviceContext->Map(m_Result_Buffer4, 0, D3D11_MAP_READ, 0, &MappedResource4);
	_Analysis_assume_(MappedResource4.pData);
	assert(MappedResource4.pData);
	// m_BoundingBoxes wird in CreateVertexAndTriangleArray neu initialisiert
	memcpy(m_Results4, MappedResource4.pData, m_TreeSize * sizeof(UINT));
	deviceContext->Unmap(m_Result_Buffer4, 0);
}

void CollisionDetectionManager::_5_FillTypeTree_GetResult()
{
	SAFEDELETEARRAY(m_Results5_1);
	SAFEDELETEARRAY(m_Results5_2);

	m_Results5_1 = new UINT[m_TreeSize];
	m_Results5_2 = new UINT[m_TreeSize];

	D3D11_MAPPED_SUBRESOURCE MappedResource5_1 = { 0 };
	D3D11_MAPPED_SUBRESOURCE MappedResource5_2 = { 0 };
	deviceContext->CopyResource(m_Result_Buffer5_1, m_GlobalCounterTree_Buffer);
	deviceContext->CopyResource(m_Result_Buffer5_2, m_TypeTree_Buffer);
	deviceContext->Map(m_Result_Buffer5_1, 0, D3D11_MAP_READ, 0, &MappedResource5_1);
	deviceContext->Map(m_Result_Buffer5_2, 0, D3D11_MAP_READ, 0, &MappedResource5_2);

	_Analysis_assume_(MappedResource5_1.pData);
	assert(MappedResource5_1.pData);

	_Analysis_assume_(MappedResource5_2.pData);
	assert(MappedResource5_2.pData);
	// m_BoundingBoxes wird in CreateVertexAndTriangleArray neu initialisiert
	memcpy(m_Results5_1, MappedResource5_1.pData, m_TreeSize * sizeof(UINT));
	memcpy(m_Results5_2, MappedResource5_2.pData, m_TreeSize * sizeof(UINT));
	deviceContext->Unmap(m_Result_Buffer5_1, 0);
	deviceContext->Unmap(m_Result_Buffer5_2, 0);

	//int leafCountperLevel[SUBDIVS+1] = { 0 };
	//for (int i = 0; i < m_TreeSizeInLevel[SUBDIVS]; i++)
	//{
	//	if (m_Results5_2[i] == 2)
	//	{
	//		for (int j = 0; j < SUBDIVS+1; j++)
	//		{
	//			/*if (i >= 37449) 
	//				cout << i << " " ;*/
	//			if (i < m_TreeSizeInLevel[j]) // guck in welchem level hochgezählt werden muss
	//			{
	//				leafCountperLevel[j] += 1;
	//				if (j == 6)
	//					cout << i << endl;
	//				break;
	//			}
	//		}
	//	}
	//}
	//for (int j = 0; j < SUBDIVS+1; j++)
	//{
	//	cout << j << ": " << leafCountperLevel[j] << endl;
	//}
	//cout << "fertig" << endl;
}

void CollisionDetectionManager::_6_FillLeafIndexTree_GetResult()
{
	SAFEDELETEARRAY(m_Results6);

	m_Results6 = new UINT[m_TreeSize];

	D3D11_MAPPED_SUBRESOURCE MappedResource6 = { 0 };
	deviceContext->CopyResource(m_Result_Buffer6, m_LeafIndexTree_Buffer);
	deviceContext->Map(m_Result_Buffer6, 0, D3D11_MAP_READ, 0, &MappedResource6);

	_Analysis_assume_(MappedResource6.pData);
	assert(MappedResource6.pData);

	_Analysis_assume_(MappedResource6.pData);
	assert(MappedResource6.pData);
	memcpy(m_Results6, MappedResource6.pData, m_TreeSize * sizeof(UINT));
	deviceContext->Unmap(m_Result_Buffer6, 0);

	/*int counter = 0;
	for (int i = 0; i < m_TreeSizeInLevel[SUBDIVS]; i++)
	{
		if (m_Results6[i] >= 37449)
		{
			cout << m_Results6[i] << ", ID:" << i << endl;
			counter++;
		}
			
	}
	cout << "counter:" << counter << endl;*/
}
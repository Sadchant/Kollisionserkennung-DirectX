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

	m_curComputeShader = 0;

	m_Vertex_Buffer = 0;
	m_Triangle_Buffer = 0;
	m_ObjectsLastIndices_Buffer = 0;
	m_BoundingBox_Buffer = 0;
	m_CounterTrees_Buffer = 0;
	m_GroupMinPoint_Buffer = 0;
	m_GroupMaxPoint_Buffer = 0;

	m_ReduceData_CBuffer = 0;
	m_FillCounterTreesData_CBuffer = 0;

	m_Result_Buffer1 = 0;
	m_Result_Buffer2_1 = 0;
	m_Result_Buffer2_2 = 0;
	m_Result_Buffer3 = 0;

	m_NULL_SRV = NULL;
	m_NULL_UAV = NULL;

	m_Vertex_SRV = 0;
	m_Triangle_SRV = 0;
	m_ObjectsLastIndices_SRV = 0;

	m_BoundingBox_UAV = 0;
	m_GroupMinPoint_UAV = 0;
	m_GroupMaxPoint_UAV = 0;
	m_CounterTrees_UAV = 0;
}


CollisionDetectionManager::~CollisionDetectionManager()
{
}

void CollisionDetectionManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd, vector<ModelClass*>* objects)
{
	this->device = device;
	this->deviceContext = deviceContext;

	m_ReduceData_CBuffer = CreateConstantBuffer(sizeof(ReduceData), D3D11_USAGE_DEFAULT, NULL);
	m_hwnd = hwnd;
	InitComputeShaderVector();

	CreateVertexAndTriangleArray(objects);
	CreateSceneBuffersAndViews();
}

// bef�lle m_ComputeShaderVector mit allen Compute Shadern
void CollisionDetectionManager::InitComputeShaderVector()
{
	ID3D11ComputeShader* pTempComputeShader;
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/1_BoundingBoxes_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/2_SceneBoundingBox_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/3_FillCounterTrees_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
}

// kopiert Punkte und Dreiecke aller Objekte in objects in gro�e Buffer zusammen, in LastObjectIndices wird sich gemerkt, welche Dreiecke
// zu welchem Objekt geh�ren
void CollisionDetectionManager::CreateVertexAndTriangleArray(vector<ModelClass*>* objects)
{
	//Zuerst ausrechnen, wie gro� das Triangle-Array sein sollte
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
	m_ObjectsLastIndices = new UINT[objects->size()]; // es gibt so viele Eintr�ge wie Objekte


	int curGlobalPosition = 0;
	int curLastIndex = 0;
	// gehe �ber alle Objekte
	for (int i = 0; i < objects->size(); i++)
	{
		ModelClass *curModel = (*objects)[i];
		VertexAndVertexDataType* modelData = curModel->GetModelData(); // hole die rohen Objektdaten (ein Eintrag ist ein Punkt, seine Texturkoordinaten und Normale, wir wollen aber nur den Punkt)
		for (int i = 0; i < curModel->GetIndexCount(); i++) // iteriere �ber jeden Vertex in modelData
		{
			m_Vertices[curGlobalPosition] = { modelData[i].x,  modelData[i].y, modelData[i].z };
			if (curGlobalPosition % 3 == 0)
			{
				// im Format dieses Tutorials sind die Punkte so aufgelistet, dass der Reihe nach durchgez�hlt wird, um auf die Dreiecke zu kommen
				int curTriangleIndex = curGlobalPosition / 3;
				m_Triangles[curTriangleIndex] = { { curGlobalPosition,  curGlobalPosition + 1, curGlobalPosition + 2} };
			}
			curGlobalPosition++;
		}
		// schreibe au�erdem den letzten Index des Objektes in m_ObjectLastIndices
		curLastIndex += curModel->GetIndexCount();
		m_ObjectsLastIndices[i] = curLastIndex;
	}
}

// erzeugt alle Buffer, die bei Hinzuf�gen/L�schen von Objekten in der Szene ihre Gr��e �ndern
void CollisionDetectionManager::CreateSceneBuffersAndViews()
{
	D3D11_SUBRESOURCE_DATA vertex_SubresourceData = D3D11_SUBRESOURCE_DATA{ m_Vertices, 0, 0 };
	D3D11_SUBRESOURCE_DATA triangle_SubresourceData = D3D11_SUBRESOURCE_DATA{ m_Triangles, 0, 0 };
	D3D11_SUBRESOURCE_DATA objectLastIndices_SubresourceData = D3D11_SUBRESOURCE_DATA{ m_ObjectsLastIndices, 0, 0 };

	// Buffer, ShaderResourceViews und UnorderedAccessViews m�ssen released werden (falls etwas in ihnen ist), bevor sie neu created werden!
	ReleaseBuffersAndViews();
	m_Vertex_Buffer = CreateStructuredBuffer(m_VertexCount, sizeof(Vertex), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE, 0, &vertex_SubresourceData);
	m_Triangle_Buffer = CreateStructuredBuffer(m_TriangleCount, sizeof(Triangle), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE, 0, &triangle_SubresourceData);
	m_ObjectsLastIndices_Buffer = CreateStructuredBuffer(m_ObjectCount, sizeof(UINT), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE, 0, &objectLastIndices_SubresourceData);
	m_BoundingBox_Buffer = CreateStructuredBuffer(m_TriangleCount, sizeof(BoundingBox), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);
	// BoundingBox_Buffer und Result_Buffer werd ja im Shader bef�llt, m�ssen also nicht mit Daten initialisiert werdenS

	m_GroupResult_Count = (int)ceil((float)m_VertexCount / (2 * B_SCENEBOUNDINGBOX_XTHREADS));
	m_GroupMinPoint_Buffer = CreateStructuredBuffer(m_GroupResult_Count, sizeof(Vertex), D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);
	m_GroupMaxPoint_Buffer = CreateStructuredBuffer(m_GroupResult_Count, sizeof(Vertex), D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);

	m_TreeSize = 0;
	// Berechne aus LEVELS die TreeSize (es wird die Anzahl der Zellen pro Level zusammengerechnet)
	for (int i = 0; i < LEVELS; i++)
	{
		m_TreeSize += (int)pow(8, i); // es gibt pro Level 8 hoch aktuelles Level Unterteilungen
		m_FillCounterTreesData.treeSizeInLevel[i] = XMUINT4((UINT)m_TreeSize, 0, 0, 0 );
	}
	m_FillCounterTreesData.objectCount = m_ObjectCount;
	D3D11_SUBRESOURCE_DATA fillCounterTreesData_SubresourceData = D3D11_SUBRESOURCE_DATA{ &m_FillCounterTreesData, 0, 0 };

	m_CounterTreesSize = m_ObjectCount*m_TreeSize;	m_CounterTrees_Buffer = CreateStructuredBuffer(m_CounterTreesSize, sizeof(UINT), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);
	m_FillCounterTreesData_CBuffer = CreateConstantBuffer(sizeof(FillCounterTreesData), D3D11_USAGE_IMMUTABLE, &fillCounterTreesData_SubresourceData);

	m_Result_Buffer1 = CreateStructuredBuffer(m_TriangleCount, sizeof(BoundingBox), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer2_1 = CreateStructuredBuffer(m_GroupResult_Count, sizeof(Vertex), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer2_2 = CreateStructuredBuffer(m_GroupResult_Count, sizeof(Vertex), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer3 = CreateStructuredBuffer(m_CounterTreesSize, sizeof(UINT), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);

	m_Vertex_SRV = CreateBufferShaderResourceView(m_Vertex_Buffer, m_VertexCount);
	m_Triangle_SRV = CreateBufferShaderResourceView(m_Triangle_Buffer, m_TriangleCount);
	m_ObjectsLastIndices_SRV = CreateBufferShaderResourceView(m_ObjectsLastIndices_Buffer, m_ObjectCount);

	m_BoundingBox_UAV = CreateBufferUnorderedAccessView(m_BoundingBox_Buffer, m_TriangleCount);

	m_GroupMinPoint_UAV = CreateBufferUnorderedAccessView(m_GroupMinPoint_Buffer, m_GroupResult_Count);
	m_GroupMaxPoint_UAV = CreateBufferUnorderedAccessView(m_GroupMaxPoint_Buffer, m_GroupResult_Count);

	m_CounterTrees_UAV = CreateBufferUnorderedAccessView(m_CounterTrees_Buffer, m_ObjectCount*m_TreeSize);

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
	// Constant Buffer wird nur einmal erzeugt (weil sich seine Gr��e nicht �ndert), also nur in Shutdown released!
	SAFERELEASE(m_Result_Buffer1);
	SAFERELEASE(m_Result_Buffer2_1);
	SAFERELEASE(m_Result_Buffer2_2);
	SAFERELEASE(m_Result_Buffer3);

	SAFERELEASE(m_Vertex_SRV);
	SAFERELEASE(m_Triangle_SRV);
	SAFERELEASE(m_ObjectsLastIndices_SRV);

	SAFERELEASE(m_BoundingBox_UAV);
	SAFERELEASE(m_GroupMinPoint_UAV);
	SAFERELEASE(m_GroupMaxPoint_UAV);
	SAFERELEASE(m_CounterTrees_UAV);
	
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

	SAFERELEASE(m_ReduceData_CBuffer);
	SAFERELEASE(m_FillCounterTreesData_CBuffer);

	ReleaseBuffersAndViews();

	// den Vector mit den Compute Shadern durchlaufen, alle releasen und danach den Vector leeren
	for (int i = 0; i < m_ComputeShaderVector.size(); i++)
	{
		//SAFERELEASE(m_ComputeShaderVector[i]);
		m_ComputeShaderVector[i]->Release();
	}
	cout << m_ComputeShaderVector.size() << ", " << m_ComputeShaderVector[0] << endl;
	/*for (auto pCurComputeShader : m_ComputeShaderVector)
	{
		pCurComputeShader->Release();
	}*/
	m_ComputeShaderVector.clear();
}



ID3D11ComputeShader* CollisionDetectionManager::CreateComputeShader(WCHAR* csFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage; // wird benutzt um beliebige "Length-Data" zur�ckzugeben
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
	// Shader-Objekt wird in den m_computeShader gef�llt
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

// Erzeugt unorderd Access View f�r normalen Structured Buffer
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

// Erzeugt unorderd Access View f�r normalen Structured Buffer
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

// f�hre die Kollisionsberechnung f�r das aktuelle Frame durch
bool CollisionDetectionManager::Frame()
{
	// ####### Berechne Bounding Boxes f�r jedes Dreieck #######
	auto begin = high_resolution_clock::now();

	auto end = high_resolution_clock::now();
	//cout << "Buffer created" << ": " << duration_cast<milliseconds>(end - begin).count() << "ms" << endl;
	begin = high_resolution_clock::now();

	int xThreadGroups = (int)ceil(m_TriangleCount / 1024.0f);

	deviceContext->CSSetShaderResources(0, 1, &m_Vertex_SRV);
	deviceContext->CSSetShaderResources(1, 1, &m_Triangle_SRV);

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_BoundingBox_UAV, 0);

	m_curComputeShader = m_ComputeShaderVector[0];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);
	deviceContext->Dispatch(xThreadGroups, 1, 1);


	// ####### Berechne Bounding Box f�r die gesamte Szene #######
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
		// im allerersten Durchlauf werden die Vertices als Input f�r die Berechnungen der Szenen-BoundingBox benutzt
		// der ThreadCount (wie viele Threads werden praktisch ben�tigt?) berechnet sich aus vertexCount
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
			outputIsInput = true; // alle au�er dem ersten Durchlauf d�rfen den Input manipulieren
		}
		groupCount = (int)ceil((float)threadCount / B_SCENEBOUNDINGBOX_XTHREADS);
		firstStepStride = groupCount * B_SCENEBOUNDINGBOX_XTHREADS;
		// Struct f�r den Constant Buffer
		ReduceData reduceData = { firstStepStride, inputSize, outputIsInput };
		deviceContext->UpdateSubresource(m_ReduceData_CBuffer, 0, NULL, &reduceData, 0, 0);
		deviceContext->CSSetConstantBuffers(0, 1, &m_ReduceData_CBuffer);
		deviceContext->Dispatch(groupCount, 1, 1);
	} while (groupCount > 1);
	// solange mehr als eine Gruppe gestartet werden muss, werden die Min-MaxPoints nicht auf ein Ergebnis reduziert sein,
	// da es ja immer ein Ergebnis pro Gruppe berechnet wird

	////####### Daten von der GPU kopieren #######
	//SAFEDELETEARRAY(m_Results2_1);
	//SAFEDELETEARRAY(m_Results2_2);

	//m_Results2_1 = new Vertex[m_GroupResult_Count];
	//m_Results2_2 = new Vertex[m_GroupResult_Count];

	//D3D11_MAPPED_SUBRESOURCE MappedResource1 = { 0 };
	//D3D11_MAPPED_SUBRESOURCE MappedResource2 = { 0 };
	//deviceContext->CopyResource(m_Result_Buffer2_1, m_GroupMinPoint_Buffer);
	//deviceContext->CopyResource(m_Result_Buffer2_2, m_GroupMaxPoint_Buffer);
	//HRESULT result = deviceContext->Map(m_Result_Buffer2_1, 0, D3D11_MAP_READ, 0, &MappedResource1);
	//result = deviceContext->Map(m_Result_Buffer2_2, 0, D3D11_MAP_READ, 0, &MappedResource2);

	//RETURN_FALSE_IF_FAIL(result);

	//_Analysis_assume_(MappedResource1.pData);
	//assert(MappedResource1.pData);

	//_Analysis_assume_(MappedResource2.pData);
	//assert(MappedResource2.pData);
	//// m_BoundingBoxes wird in CreateVertexAndTriangleArray neu initialisiert
	//memcpy(m_Results2_1, MappedResource1.pData, m_GroupResult_Count * sizeof(Vertex));
	//memcpy(m_Results2_2, MappedResource2.pData, m_GroupResult_Count * sizeof(Vertex));
	//deviceContext->Unmap(m_Result_Buffer2_1, 0);
	//deviceContext->Unmap(m_Result_Buffer2_2, 0);

	//end = high_resolution_clock::now();
	////cout << "Shader 1 + 2 (+ copy-back): " << duration_cast<milliseconds>(end - begin).count() << "ms" << endl;
	//begin = high_resolution_clock::now();


	// ####### Bef�lle Countertrees mit den Daten f�r jedes Objekt #######

	m_curComputeShader = m_ComputeShaderVector[2];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_GroupMinPoint_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_GroupMaxPoint_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(2, 1, &m_BoundingBox_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(3, 1, &m_CounterTrees_UAV, 0);
	
	deviceContext->CSSetShaderResources(0, 1, &m_ObjectsLastIndices_SRV);
	deviceContext->CSSetConstantBuffers(0, 1, &m_FillCounterTreesData_CBuffer);
	deviceContext->Dispatch(xThreadGroups, 1, 1);

	// entferne die UAVs wieder von den Slots 0 - 2, damit sie wieder verwendet werden k�nnen
	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_NULL_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_NULL_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(2, 1, &m_NULL_UAV, 0);

	//####### Daten von der GPU kopieren #######
	SAFEDELETEARRAY(m_Results3);

	m_Results3 = new UINT[m_CounterTreesSize];

	D3D11_MAPPED_SUBRESOURCE MappedResource3 = { 0 };
	deviceContext->CopyResource(m_Result_Buffer3, m_CounterTrees_Buffer);
	HRESULT result = deviceContext->Map(m_Result_Buffer3, 0, D3D11_MAP_READ, 0, &MappedResource3);

	RETURN_FALSE_IF_FAIL(result);

	_Analysis_assume_(MappedResource3.pData);
	assert(MappedResource3.pData);

	// m_BoundingBoxes wird in CreateVertexAndTriangleArray neu initialisiert
	memcpy(m_Results3, MappedResource3.pData, m_CounterTreesSize * sizeof(UINT));
	deviceContext->Unmap(m_Result_Buffer3, 0);

	/*end = high_resolution_clock::now();
	cout << "Shader 3 (+ copy-back): " << duration_cast<milliseconds>(end - begin).count() << "ms" << endl;*/

	return true;
}

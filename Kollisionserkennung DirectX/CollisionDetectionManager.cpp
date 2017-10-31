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
	m_Results8_1 = 0;
	m_Results8_2 = 0;
	m_Results8_3 = 0;
	m_Results9 = 0;
	m_Results10_1_IntersectingObjects = 0;
	m_Results10_2_IntersectionPoints = 0;

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
	m_SortIndices_Buffer = 0;
	m_CellTrianglePairsBackBuffer_Buffer = 0;
	m_TrianglePairs_Buffer = 0;
	m_IntersectingObjects_Buffer = 0;
	m_IntersectCenters_Buffer = 0;

	m_ReduceData_CBuffer = 0;
	m_ObjectCount_CBuffer = 0;
	m_TreeSizeInLevel_CBuffer = 0;
	m_StartLevel_CBuffer = 0;
	m_Loops_CBuffer = 0;
	m_RadixSort_ExclusivePrefixSumData_CBuffer = 0;
	m_RadixSort_ExclusivePrefixSumData2_CBuffer = 0;

	m_Result_Buffer1 = 0;
	m_Result_Buffer2_1 = 0;
	m_Result_Buffer2_2 = 0;
	m_Result_Buffer3 = 0;
	m_Result_Buffer4 = 0;
	m_Result_Buffer5_1 = 0;
	m_Result_Buffer5_2 = 0;
	m_Result_Buffer6 = 0;
	m_Result_Buffer7 = 0;
	m_Result_Buffer8_1 = 0;
	m_Result_Buffer8_2 = 0;
	m_Result_Buffer8_3 = 0;
	m_Result_Buffer9 = 0;
	m_IntersectingObjects_Result_Buffer = 0;
	m_IntersectCenters_Result1_Buffer = 0;
	m_IntersectCenters_Result2_Buffer = 0;

	m_NULL_SRV = NULL;
	m_NULL_UAV = NULL;

	m_Vertices_SRV = 0;
	m_Triangles_SRV = 0;
	m_ObjectsLastIndices_SRV = 0;

	m_BoundingBoxes_UAV = 0;
	m_GroupMinPoints_UAV = 0;
	m_GroupMaxPoints_UAV = 0;
	m_CounterTrees_UAV = 0;
	m_GlobalCounterTree_UAV = 0;
	m_TypeTree_UAV = 0;
	m_LeafIndexTree_UAV = 0;
	m_CellTrianglePairs_UAV = 0;
	m_SortIndices_UAV = 0;
	m_CellTrianglePairsBackBuffer_UAV = 0;
	m_TrianglePairs_UAV = 0;
	m_IntersectingObjects_UAV = 0;
	m_IntersectCenters_UAV = 0;

	m_CopyBackThreadExists = false;
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
	m_RadixSort_ExclusivePrefixSumData_CBuffer = CreateConstantBuffer(sizeof(RadixSort_ExclusivePrefixSumData), D3D11_USAGE_DEFAULT, NULL);
	m_RadixSort_ExclusivePrefixSumData2_CBuffer = CreateConstantBuffer(sizeof(RadixSort_ExclusivePrefixSumData2), D3D11_USAGE_DEFAULT, NULL);
		
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

	m_SortedBitsCount = (int)ceil(log2(m_TreeSize));

	m_CopyTo1 = true;

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
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/7_CellTrianglePairs_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/8_1_RadixSort_ExclusivePrefixSum_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/8_2_RadixSort_ExclusivePrefixSum_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/8_3_RadixSort_Sort_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/9_FindTrianglePairs_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/10_TriangleIntersections_CS.hlsl");
	m_ComputeShaderVector.push_back(pTempComputeShader);
	pTempComputeShader = CreateComputeShader(L"../Kollisionserkennung DirectX/11_ZeroIntersectionCenters_CS.hlsl");
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
	m_CellTrianglePairsCount = (int)ceil(m_TriangleCount * 3.5);
	m_SortIndicesCount = (int)pow(2, (int)ceil(log2(m_CellTrianglePairsCount))) + 1; // + 1 für eine komplette exklusive Prefix Summe
	m_TrianglePairsCount = m_TriangleCount * 30;
	m_IntersectionCentersCount = m_TriangleCount * 8;
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

	m_GroupResult_Count = (int)ceil((float)m_VertexCount / (2 * LINEAR_XTHREADS));
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
	m_SortIndices_Buffer = CreateStructuredBuffer(m_SortIndicesCount, sizeof(SortIndices), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);
	m_CellTrianglePairsBackBuffer_Buffer = CreateStructuredBuffer(m_CellTrianglePairsCount, sizeof(CellTrianglePair), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);
	m_TrianglePairs_Buffer = CreateStructuredBuffer(m_TrianglePairsCount, sizeof(TrianglePair), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);
	m_IntersectingObjects_Buffer = CreateStructuredBuffer(m_ObjectCount, sizeof(UINT), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);
	m_IntersectCenters_Buffer = CreateStructuredBuffer(m_IntersectionCentersCount, sizeof(Vertex), D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, NULL);


	m_Result_Buffer1 = CreateStructuredBuffer(m_TriangleCount, sizeof(BoundingBox), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer2_1 = CreateStructuredBuffer(m_GroupResult_Count, sizeof(Vertex), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer2_2 = CreateStructuredBuffer(m_GroupResult_Count, sizeof(Vertex), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer3 = CreateStructuredBuffer(m_CounterTreesSize, sizeof(UINT), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer4 = CreateStructuredBuffer(m_TreeSize, sizeof(UINT), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer5_1 = CreateStructuredBuffer(m_TreeSize, sizeof(UINT), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer5_2 = CreateStructuredBuffer(m_TreeSize, sizeof(UINT), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer6 = CreateStructuredBuffer(m_TreeSize, sizeof(UINT), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer7 = CreateStructuredBuffer(m_CellTrianglePairsCount, sizeof(CellTrianglePair), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer8_1 = CreateStructuredBuffer(m_SortIndicesCount, sizeof(SortIndices), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer8_2 = CreateStructuredBuffer(m_CellTrianglePairsCount, sizeof(CellTrianglePair), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer8_3 = CreateStructuredBuffer(m_CellTrianglePairsCount, sizeof(CellTrianglePair), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_Result_Buffer9 = CreateStructuredBuffer(m_TrianglePairsCount, sizeof(TrianglePair), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_IntersectingObjects_Result_Buffer = CreateStructuredBuffer(m_ObjectCount, sizeof(UINT), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_IntersectCenters_Result1_Buffer = CreateStructuredBuffer(m_IntersectionCentersCount, sizeof(Vertex), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	m_IntersectCenters_Result2_Buffer = CreateStructuredBuffer(m_IntersectionCentersCount, sizeof(Vertex), 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, NULL);
	
	m_Vertices_SRV = CreateBufferShaderResourceView(m_Vertex_Buffer, m_VertexCount);
	m_Triangles_SRV = CreateBufferShaderResourceView(m_Triangle_Buffer, m_TriangleCount);
	m_ObjectsLastIndices_SRV = CreateBufferShaderResourceView(m_ObjectsLastIndices_Buffer, m_ObjectCount);

	m_BoundingBoxes_UAV = CreateBufferUnorderedAccessView(m_BoundingBox_Buffer, m_TriangleCount);
	m_GroupMinPoints_UAV = CreateBufferUnorderedAccessView(m_GroupMinPoint_Buffer, m_GroupResult_Count);
	m_GroupMaxPoints_UAV = CreateBufferUnorderedAccessView(m_GroupMaxPoint_Buffer, m_GroupResult_Count);
	m_CounterTrees_UAV = CreateBufferUnorderedAccessView(m_CounterTrees_Buffer, m_ObjectCount*m_TreeSize);
	m_GlobalCounterTree_UAV = CreateBufferUnorderedAccessView(m_GlobalCounterTree_Buffer, m_TreeSize);
	m_TypeTree_UAV = CreateBufferUnorderedAccessView(m_TypeTree_Buffer, m_TreeSize);
	m_LeafIndexTree_UAV = CreateBufferUnorderedAccessView(m_LeafIndexTree_Buffer, m_TreeSize);
	m_CellTrianglePairs_UAV = CreateBufferUnorderedAccessView(m_CellTrianglePairs_Buffer, m_CellTrianglePairsCount, D3D11_BUFFER_UAV_FLAG_COUNTER);
	m_SortIndices_UAV = CreateBufferUnorderedAccessView(m_SortIndices_Buffer, m_SortIndicesCount);
	m_CellTrianglePairsBackBuffer_UAV = CreateBufferUnorderedAccessView(m_CellTrianglePairsBackBuffer_Buffer, m_CellTrianglePairsCount);
	m_TrianglePairs_UAV = CreateBufferUnorderedAccessView(m_TrianglePairs_Buffer, m_TrianglePairsCount, D3D11_BUFFER_UAV_FLAG_COUNTER);
	m_IntersectingObjects_UAV = CreateBufferUnorderedAccessView(m_IntersectingObjects_Buffer, m_ObjectCount);
	m_IntersectCenters_UAV = CreateBufferUnorderedAccessView(m_IntersectCenters_Buffer, m_IntersectionCentersCount, D3D11_BUFFER_UAV_FLAG_COUNTER);

	m_CellTrianglePairs_Zero = new CellTrianglePair[m_CellTrianglePairsCount];
	ZeroMemory(m_CellTrianglePairs_Zero, m_CellTrianglePairsCount * sizeof(CellTrianglePair));
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
	SAFERELEASE(m_SortIndices_Buffer);
	SAFERELEASE(m_CellTrianglePairsBackBuffer_Buffer);
	SAFERELEASE(m_TrianglePairs_Buffer);
	SAFERELEASE(m_IntersectingObjects_Buffer);
	SAFERELEASE(m_IntersectCenters_Buffer);
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
	SAFERELEASE(m_Result_Buffer8_1);
	SAFERELEASE(m_Result_Buffer8_2);
	SAFERELEASE(m_Result_Buffer8_3);
	SAFERELEASE(m_Result_Buffer9);
	SAFERELEASE(m_IntersectingObjects_Result_Buffer);
	SAFERELEASE(m_IntersectCenters_Result1_Buffer);
	SAFERELEASE(m_IntersectCenters_Result2_Buffer);

	SAFERELEASE(m_Vertices_SRV);
	SAFERELEASE(m_Triangles_SRV);
	SAFERELEASE(m_ObjectsLastIndices_SRV);

	SAFERELEASE(m_BoundingBoxes_UAV);
	SAFERELEASE(m_GroupMinPoints_UAV);
	SAFERELEASE(m_GroupMaxPoints_UAV);
	SAFERELEASE(m_CounterTrees_UAV);
	SAFERELEASE(m_GlobalCounterTree_UAV);
	SAFERELEASE(m_TypeTree_UAV);
	SAFERELEASE(m_LeafIndexTree_UAV);
	SAFERELEASE(m_CellTrianglePairs_UAV);
	SAFERELEASE(m_SortIndices_UAV);
	SAFERELEASE(m_CellTrianglePairsBackBuffer_UAV);
	SAFERELEASE(m_TrianglePairs_UAV);
	SAFERELEASE(m_IntersectingObjects_UAV);
	SAFERELEASE(m_IntersectCenters_UAV);	
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
	SAFEDELETEARRAY(m_Results8_1);
	SAFEDELETEARRAY(m_Results8_2);
	SAFEDELETEARRAY(m_Results8_3);
	SAFEDELETEARRAY(m_Results9);
	SAFEDELETEARRAY(m_Results10_1_IntersectingObjects);
	SAFEDELETEARRAY(m_Results10_2_IntersectionPoints);

	SAFERELEASE(m_ReduceData_CBuffer);
	SAFERELEASE(m_ObjectCount_CBuffer);
	SAFERELEASE(m_TreeSizeInLevel_CBuffer);
	SAFERELEASE(m_StartLevel_CBuffer);
	SAFERELEASE(m_Loops_CBuffer);
	SAFERELEASE(m_RadixSort_ExclusivePrefixSumData_CBuffer);
	SAFERELEASE(m_RadixSort_ExclusivePrefixSumData2_CBuffer);

	SAFEDELETEARRAY(m_CellTrianglePairs_Zero);

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
	result = D3DCompileFromFile(csFilename, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "cs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION /*| D3DCOMPILE_WARNINGS_ARE_ERRORS*/,
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

	deviceContext->CSSetShaderResources(0, 1, &m_Vertices_SRV);
	deviceContext->CSSetShaderResources(1, 1, &m_Triangles_SRV);

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_BoundingBoxes_UAV, 0);

	m_curComputeShader = m_ComputeShaderVector[0];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);
	deviceContext->Dispatch(xThreadGroups, 1, 1);
}

// ******* 2. Berechne Bounding Box für die gesamte Szene *******
void CollisionDetectionManager::_2_SceneCoundingBox()
{
	m_curComputeShader = m_ComputeShaderVector[1];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

	deviceContext->CSSetShaderResources(0, 1, &m_Vertices_SRV);
	deviceContext->CSSetShaderResources(1, 1, &m_Vertices_SRV);
	// GroupMinMaxPoint: Output im ersten Durchgang, Input und Output in allen anderen
	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_GroupMinPoints_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_GroupMaxPoints_UAV, 0);

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
		groupCount = (int)ceil((float)threadCount / LINEAR_XTHREADS);
		firstStepStride = groupCount * LINEAR_XTHREADS;
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

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_GroupMinPoints_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_GroupMaxPoints_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(2, 1, &m_BoundingBoxes_UAV, 0);
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

// ******* 7. Befülle Countertrees mit den Daten für jedes Objekt *******
void CollisionDetectionManager::_7_CellTrianglePairs()
{
	m_curComputeShader = m_ComputeShaderVector[6];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

	UINT zero = 0;
	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_BoundingBoxes_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_GroupMinPoints_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(2, 1, &m_GroupMaxPoints_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(3, 1, &m_GlobalCounterTree_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(4, 1, &m_LeafIndexTree_UAV, 0);

	deviceContext->CSSetUnorderedAccessViews(5, 1, &m_CellTrianglePairs_UAV, &zero);

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
	deviceContext->CSSetUnorderedAccessViews(4, 1, &m_NULL_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(5, 1, &m_NULL_UAV, 0);
}

// ******* 8. Sortiere cellTrianglePairs nach Zellen-IDs *******
bool CollisionDetectionManager::_8_SortCellTrianglePairs()
{ 

	int sortIndicesCountPow2 = m_SortIndicesCount - 1; // m_SortIndices ist ja eins größer als die Zweierpotenz, sortIndicesPow2 ist die Zweierpotenz, mit der Schrittweiten und loop-Werte ebrechnet werden
	int read2BitsFromHere = 0;
	bool backBufferIsInput = false;

	while (read2BitsFromHere < m_SortedBitsCount)
	{
		// ####################################################_8_1_####################################################
		m_curComputeShader = m_ComputeShaderVector[7];
		deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

		if (backBufferIsInput)
			deviceContext->CSSetUnorderedAccessViews(0, 1, &m_CellTrianglePairsBackBuffer_UAV, 0);
		else
			deviceContext->CSSetUnorderedAccessViews(0, 1, &m_CellTrianglePairs_UAV, 0); 

		deviceContext->CSSetUnorderedAccessViews(1, 1, &m_SortIndices_UAV, 0);

		int _1_curInputSize = sortIndicesCountPow2;
		int _1_curWorkSize = _1_curInputSize / 2;
		UINT _1_curLoops;
		UINT _1_combineDistance = 1;
		bool readFromInput = true;
		int curRead2BitsFromHere; // hier wird entweder read2BitsFromHere eingetragen oder -1, wenn es der erste Durchgang ist
		while (_1_curWorkSize > 2)
		{
			int groupCount = (int)ceil(_1_curWorkSize / 1024.0f);
			if (_1_curWorkSize >= 1024)
				_1_curLoops = 11; // 11 Druchläufe verkleinern 1024 auf 1, wir wollen lediglich im letzten Durchlauf auf 2 verkleinern, wichtiger Unterschied!, also hier 11 statt 10
			else
				_1_curLoops = (UINT)log2(_1_curInputSize) - 1; // weil bei 2 Elementen aufgehört werden kann, laufe einmal weniger (ansonsten wäre es bis 1 Element gegangen)
			if (!readFromInput) // sollte es der erste Durchlauf sein, müssen die Bits eingelesen werden, dem Shader wird curRead2BitsFromHere = -1 übergeben
				curRead2BitsFromHere = -1;
			else // ansonsten wurden die Bits schon eingelesen und die relevanten read2BitsFromHere werden an den Shader übergeben
				curRead2BitsFromHere = read2BitsFromHere;
			RadixSort_ExclusivePrefixSumData radixSort_ExclusivePrefixSum_Data = { _1_curLoops, curRead2BitsFromHere, _1_combineDistance };
			deviceContext->UpdateSubresource(m_RadixSort_ExclusivePrefixSumData_CBuffer, 0, NULL, &radixSort_ExclusivePrefixSum_Data, 0, 0);
			deviceContext->CSSetConstantBuffers(0, 1, &m_RadixSort_ExclusivePrefixSumData_CBuffer);
			deviceContext->Dispatch(groupCount, 1, 1);
			_1_curInputSize /= 2048;
			_1_curWorkSize = _1_curInputSize / 2;
			_1_combineDistance *= 2048;
			readFromInput = false;
		}

		deviceContext->CSSetUnorderedAccessViews(0, 1, &m_NULL_UAV, 0);
		deviceContext->CSSetUnorderedAccessViews(1, 1, &m_NULL_UAV, 0);
		
		// ####################################################_8_2_####################################################

		// Phase 2 der exklusive Prefix Summe
		m_curComputeShader = m_ComputeShaderVector[8];
		deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

		deviceContext->CSSetUnorderedAccessViews(0, 1, &m_SortIndices_UAV, 0);
		if (backBufferIsInput)
			deviceContext->CSSetUnorderedAccessViews(1, 1, &m_CellTrianglePairsBackBuffer_UAV, 0);
		else
			deviceContext->CSSetUnorderedAccessViews(1, 1, &m_CellTrianglePairs_UAV, 0); 

		unsigned long long _2_curInputSize;
		UINT _2_curLoops, _2_curThreadDistance/*, _2_curStartCombineDistance*/;

		// ermittle die InputSize des ersten Dispatches
		_2_curInputSize = sortIndicesCountPow2; // DURCH 2 HÖCHSTWAHRSCHEINLICH HIER GUCKEN!!!
		while (_2_curInputSize > 2048) // teile solange durch 2048, bis ein Wert kleiner als 2048 herauskommt, das ist die inputSize für den ersten Dispatch
		{
			_2_curInputSize /= 2048;
		}
		int _2_curWorkSize = (int)_2_curInputSize / 2;
		_2_curThreadDistance = sortIndicesCountPow2 / _2_curWorkSize; // * 2, weil ein Thread ja am Ende 2 Inputs bearbeitet, die Distanz ist also doppelt so groß
		_2_curLoops = (int)log2(_2_curInputSize);// curInputSize wird nicht durch 2 geteilt, da log2 ja die Basis 2 hat, wir aber am Ende auf 1 kommen wollen, also das Ergebnis nochmal durch 2 teilen
		bool firstStep = true;
		while (_2_curInputSize <= (UINT)sortIndicesCountPow2) // beim letzten Schritt ist die inputSize = m_SortIndicesCount, deswegen das <=
		{
			int groupCount = (int)ceil(_2_curWorkSize / 1024.0f);
			RadixSort_ExclusivePrefixSumData2 radixSort_ExclusivePrefixSum_Data2 = { (UINT)firstStep, _2_curThreadDistance, _2_curLoops, read2BitsFromHere };
			deviceContext->UpdateSubresource(m_RadixSort_ExclusivePrefixSumData2_CBuffer, 0, NULL, &radixSort_ExclusivePrefixSum_Data2, 0, 0);
			deviceContext->CSSetConstantBuffers(0, 1, &m_RadixSort_ExclusivePrefixSumData2_CBuffer);
			deviceContext->Dispatch(groupCount, 1, 1);
			_2_curInputSize *= 2048;
			_2_curWorkSize = (int)_2_curInputSize / 2;
			_2_curThreadDistance /= 2048;
			_2_curLoops = 11; // ab dem ersten Schritt werden immer 11 Schritte (soviel kann eine Gruppe reduzieren) ausgeführt
			//_2_curStartCombineDistance /= (UINT)pow (2, _2_curLoops);
			firstStep = false;
		}

		deviceContext->CSSetUnorderedAccessViews(0, 1, &m_NULL_UAV, 0);
		deviceContext->CSSetUnorderedAccessViews(1, 1, &m_NULL_UAV, 0);

		// ####################################################_8_3_####################################################
		// sortiere mit Hilfe der exklusiven Prefix-Summen
		m_curComputeShader = m_ComputeShaderVector[9];
		deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

		deviceContext->CSSetUnorderedAccessViews(0, 1, &m_SortIndices_UAV, 0);
		if (backBufferIsInput)
		{
			deviceContext->CSSetUnorderedAccessViews(1, 1, &m_CellTrianglePairsBackBuffer_UAV, 0);
			deviceContext->CSSetUnorderedAccessViews(2, 1, &m_CellTrianglePairs_UAV, 0); 
		}
		else
		{
			deviceContext->CSSetUnorderedAccessViews(1, 1, &m_CellTrianglePairs_UAV, 0); 
			deviceContext->CSSetUnorderedAccessViews(2, 1, &m_CellTrianglePairsBackBuffer_UAV, 0);
		}

		int groupCount = (int)ceil(m_CellTrianglePairsCount / 1024.0f);
		deviceContext->Dispatch(groupCount, 1, 1);

		deviceContext->CSSetUnorderedAccessViews(0, 1, &m_NULL_UAV, 0);
		deviceContext->CSSetUnorderedAccessViews(1, 1, &m_NULL_UAV, 0);
		deviceContext->CSSetUnorderedAccessViews(2, 1, &m_NULL_UAV, 0);

		backBufferIsInput = !backBufferIsInput;
		read2BitsFromHere += 2;
	}

	return backBufferIsInput;
}

// ******* 9. Finde im sortierten cellTrianglePairsBuffer alle Dreieckspaare, deren Bounding Boxes sich überschneiden *******
void CollisionDetectionManager::_9_FindTrianglePairs(bool backBufferIsInput)
{
	m_curComputeShader = m_ComputeShaderVector[10];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

	UINT zero = 0;

	if (backBufferIsInput)
		deviceContext->CSSetUnorderedAccessViews(0, 1, &m_CellTrianglePairsBackBuffer_UAV, 0);
	else
		deviceContext->CSSetUnorderedAccessViews(0, 1, &m_CellTrianglePairs_UAV, 0);

	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_BoundingBoxes_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(2, 1, &m_TrianglePairs_UAV, &zero); // setze den Buffer-Counter auf 0 zurück

	int groupCount = (int)ceil(m_TrianglePairsCount / 1024.0);
	deviceContext->Dispatch(groupCount, 1, 1);

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_NULL_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_NULL_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(2, 1, &m_NULL_UAV, 0);


	deviceContext->UpdateSubresource(m_CellTrianglePairs_Buffer, 0, NULL, m_CellTrianglePairs_Zero, m_CellTrianglePairsCount * sizeof(CellTrianglePair), 0);
	deviceContext->UpdateSubresource(m_CellTrianglePairsBackBuffer_Buffer, 0, NULL, m_CellTrianglePairs_Zero, m_CellTrianglePairsCount * sizeof(CellTrianglePair), 0);
	//Arguments: The buffer, The subresource (0), A destination box(NULL), The data to write to the buffer, the size of the buffer, the depth of the buffer
}

// ******* 10. Überprüfe alle Dreiecke in Triangle-Pairs, ob sie sich tatsächlich überschneiden  *******
void CollisionDetectionManager::_10_TriangleIntersections()
{
	m_curComputeShader = m_ComputeShaderVector[11];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

	deviceContext->CSSetShaderResources(0, 1, &m_Vertices_SRV);
	deviceContext->CSSetShaderResources(1, 1, &m_Triangles_SRV);

	UINT zero = 0;

	//deviceContext->CSSetUnorderedAccessViews(0, 1, &m_CellTrianglePairs_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_TrianglePairs_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_IntersectingObjects_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(2, 1, &m_IntersectCenters_UAV, &zero); // setzte den Buffer-internen Counter auf 0 zurück

	int groupCount = (int)ceil(m_TrianglePairsCount / 1024.0);
	deviceContext->Dispatch(groupCount, 1, 1);

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_NULL_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(1, 1, &m_NULL_UAV, 0);
	deviceContext->CSSetUnorderedAccessViews(2, 1, &m_NULL_UAV, 0);

}

// ******* 11. Überschreibe den Ergebnis-Buffer mit 0en  *******
void CollisionDetectionManager::_11_ZeroIntersectionCenters()
{
	m_curComputeShader = m_ComputeShaderVector[12];
	deviceContext->CSSetShader(m_curComputeShader, NULL, 0);

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_IntersectCenters_UAV, 0); // setzte den Buffer-internen Counter auf 0 zurück

	int groupCount = (int)ceil(m_TrianglePairsCount / 1024.0);
	deviceContext->Dispatch(groupCount, 1, 1);

	deviceContext->CSSetUnorderedAccessViews(0, 1, &m_NULL_UAV, 0);
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

	_7_CellTrianglePairs();
	//_7_CellTrianglePairs_GetResult();

	bool backBufferIsInput = _8_SortCellTrianglePairs();
	//_8_SortCellTrianglePairs_GetResult();

	_9_FindTrianglePairs(backBufferIsInput);
	//_9_FindTrianglePairs_GetResult();

	_10_TriangleIntersections();
	return;
	_10_TriangleIntersections_GetFinalResult();

	_11_ZeroIntersectionCenters();

	m_CopyTo1 = !m_CopyTo1;
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

	cout << "countertree" << endl;
	int cellCounter = 0;
	for (UINT i = 0; i < m_TreeSizeInLevel[SUBDIVS]*2; i++)
	{
		if (i >= 4681 + 37449 && i < 4681 + 1024 + 37449)
		{
			if (cellCounter % 32 == 0)
				cout << endl;
			cout << m_Results3[i] << " ";
			cellCounter++;
		}
	}
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

	cout << "GlobalCountertree:" << endl;
	int cellCounter = 0;
	for (UINT i = 0; i < m_TreeSizeInLevel[SUBDIVS]; i++)
	{
		if (i >= 4681 && i < 4681 + 1024)
		{
			if (cellCounter % 32 == 0)
				cout << endl;
			cout << m_Results4[i] << " ";
			cellCounter++;
		}
	}
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

	int leafCountperLevel[SUBDIVS+1] = { 0 };
	for (UINT i = 0; i < m_TreeSizeInLevel[SUBDIVS]; i++)
	{
		if (m_Results5_2[i] == 2)
		{
			for (UINT j = 0; j < SUBDIVS+1; j++)
			{
				/*if (i >= 37449) 
					cout << i << " " ;*/
				if (i < m_TreeSizeInLevel[j]) // guck in welchem level hochgezählt werden muss
				{
					leafCountperLevel[j] += 1;
					if (j == 6)
						cout << i << endl;
					break;
				}
			}
		}
	}
	for (int j = 0; j < SUBDIVS+1; j++)
	{
		cout << j << ": " << leafCountperLevel[j] << endl;
	}
	cout << "fertig" << endl;

	cout << "TypeTree:" << endl;
	int cellCounter = 0;
	for (UINT i = 0; i < m_TreeSizeInLevel[SUBDIVS]; i++)
	{
		if (i >= 4681 && i < 4681 + 1024)
		{
			if (cellCounter % 32 == 0)
				cout << endl;
			cout << m_Results5_2[i] << " ";
			cellCounter++;
		}
	}

	// globalcountertree
	cout << " globalCountertree optimiert:" << endl;
	cellCounter = 0;
	for (UINT i = 0; i < m_TreeSizeInLevel[SUBDIVS]; i++)
	{
		if (i >= 4681 && i < 4681 + 1024)
		{
			if (cellCounter % 32 == 0)
				cout << endl;
			cout << m_Results5_1[i] << " ";
			cellCounter++;
		}
	}
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

	cout << "LeafIndexTree:" << endl;
	int counter = 0;
	for (UINT i = 0; i < m_TreeSizeInLevel[SUBDIVS]; i++)
	{
		if (m_Results6[i] >= 4681)
		{
			cout << m_Results6[i] << ", ID:" << i << endl;
			counter++;
		}
			
	}
	cout << "counter:" << counter << endl;

	/*int cellCounter = 0;
	for (int i = 0; i < m_TreeSizeInLevel[SUBDIVS]; i++)
	{
		if (i >= 4681 && i < 4681 + 1024)
		{
			if (cellCounter % 32 == 0) 
				cout << endl;
			cout << m_Results6[i] << "\t";
			cellCounter++;
		}
	}*/
}

void CollisionDetectionManager::_7_CellTrianglePairs_GetResult()
{
	SAFEDELETEARRAY(m_Results7);
	m_Results7 = new CellTrianglePair[m_CellTrianglePairsCount];
	D3D11_MAPPED_SUBRESOURCE MappedResource7 = { 0 };
	deviceContext->CopyResource(m_Result_Buffer7, m_CellTrianglePairs_Buffer);
	deviceContext->Map(m_Result_Buffer7, 0, D3D11_MAP_READ, 0, &MappedResource7);
	_Analysis_assume_(MappedResource7.pData);
	assert(MappedResource7.pData);
	memcpy(m_Results7, MappedResource7.pData, m_CellTrianglePairsCount * sizeof(CellTrianglePair));
	deviceContext->Unmap(m_Result_Buffer7, 0);

	int i;
	for (i = 0; i < m_CellTrianglePairsCount; i++) 
	{
		if (m_Results7[i].cellID == 0 && m_Results7[i].objectID == 0 && m_Results7[i].triangleID == 0)
		{
			break;
		}
	}
	cout << "CellTriangleCount: " << i << endl;
}

void CollisionDetectionManager::_8_SortCellTrianglePairs_GetResult()
{
	SAFEDELETEARRAY(m_Results8_1);
	SAFEDELETEARRAY(m_Results8_2);
	SAFEDELETEARRAY(m_Results8_3);
	m_Results8_1 = new SortIndices[m_SortIndicesCount];
	m_Results8_2 = new CellTrianglePair[m_CellTrianglePairsCount];
	m_Results8_3 = new CellTrianglePair[m_CellTrianglePairsCount];
	D3D11_MAPPED_SUBRESOURCE MappedResource8_1 = { 0 };
	D3D11_MAPPED_SUBRESOURCE MappedResource8_2 = { 0 };
	D3D11_MAPPED_SUBRESOURCE MappedResource8_3 = { 0 };
	deviceContext->CopyResource(m_Result_Buffer8_1, m_SortIndices_Buffer);
	deviceContext->CopyResource(m_Result_Buffer8_2, m_CellTrianglePairsBackBuffer_Buffer);
	deviceContext->CopyResource(m_Result_Buffer8_3, m_CellTrianglePairs_Buffer);
	deviceContext->Map(m_Result_Buffer8_1, 0, D3D11_MAP_READ, 0, &MappedResource8_1);
	deviceContext->Map(m_Result_Buffer8_2, 0, D3D11_MAP_READ, 0, &MappedResource8_2);
	deviceContext->Map(m_Result_Buffer8_3, 0, D3D11_MAP_READ, 0, &MappedResource8_3);
	_Analysis_assume_(MappedResource8_1.pData);
	_Analysis_assume_(MappedResource8_2.pData);
	_Analysis_assume_(MappedResource8_3.pData);
	assert(MappedResource8_1.pData);
	assert(MappedResource8_2.pData);
	assert(MappedResource8_3.pData);
	// m_BoundingBoxes wird in CreateVertexAndTriangleArray neu initialisiert
	memcpy(m_Results8_1, MappedResource8_1.pData, m_SortIndicesCount * sizeof(SortIndices));
	memcpy(m_Results8_2, MappedResource8_2.pData, m_CellTrianglePairsCount * sizeof(CellTrianglePair));
	memcpy(m_Results8_3, MappedResource8_3.pData, m_CellTrianglePairsCount * sizeof(CellTrianglePair));
	deviceContext->Unmap(m_Result_Buffer8_1, 0);
	deviceContext->Unmap(m_Result_Buffer8_2, 0);
	deviceContext->Unmap(m_Result_Buffer8_3, 0);

	int counter = 0;
	int maxCellSize = 0;
	CellTrianglePair tempCellTrianglePair;
	CellTrianglePair crazyCellTrianglePair;
	for (int i = 0; i < m_CellTrianglePairsCount-1; i++)
	{
		CellTrianglePair curCellTrianglePair = m_Results8_2[i];
		CellTrianglePair nextCellTrianglePair = m_Results8_2[i + 1];
		if (curCellTrianglePair.cellID == nextCellTrianglePair.cellID)
		{
			counter++;
			tempCellTrianglePair = curCellTrianglePair;
		}
		else
		{
			//cout << counter << endl;
			if (counter > maxCellSize) 
			{
				maxCellSize = counter;
				crazyCellTrianglePair = tempCellTrianglePair;
			}
			counter = 0;
		}
	}
	cout << "MaxCellSize: " << maxCellSize << endl;

	/*int wrongIDCounter = 0;
	for (int i = 0; i < m_SortIndicesCount; i++)
	{
		SortIndices curSortIndices = m_Results8_1[i];
		SortIndices nextSortIndices = m_Results8_1[i + 1];
		int doubleCounter = 0;
		for (int j = 0; j < 4; j++) 
		{
			if (curSortIndices.array[j] != nextSortIndices.array[j])
				doubleCounter++;
		}
		if (doubleCounter != 1)
			wrongIDCounter++;
	}
	cout << "wrong IDs: " << wrongIDCounter << endl;*/
 }

void CollisionDetectionManager::_9_FindTrianglePairs_GetResult()
{
	SAFEDELETEARRAY(m_Results9);
	m_Results9 = new TrianglePair[m_TrianglePairsCount];
	D3D11_MAPPED_SUBRESOURCE MappedResource9 = { 0 };
	deviceContext->CopyResource(m_Result_Buffer9, m_TrianglePairs_Buffer);
	deviceContext->Map(m_Result_Buffer9, 0, D3D11_MAP_READ, 0, &MappedResource9);
	_Analysis_assume_(MappedResource9.pData);
	assert(MappedResource9.pData);
	memcpy(m_Results9, MappedResource9.pData, m_TrianglePairsCount * sizeof(TrianglePair));
	deviceContext->Unmap(m_Result_Buffer9, 0);


	int i = 0;
	for (i = 0; i < m_TrianglePairsCount; i++)
	{
		if (m_Results9[i].triangleID1 == 0 && m_Results9[i].triangleID2 == 0)
			break;
	}
	cout << "m_TrianglePairsCount : " << m_TrianglePairsCount << endl;
	cout << "tatsächlicher Count  : " << i << endl;
 }

void AsyncCopyFromGPU(bool copyTo1, ID3D11Buffer* intersectCenters_Result1_Buffer, ID3D11Buffer* intersectCenters_Result2_Buffer, ID3D11Buffer* intersectCenters_Buffer, ID3D11DeviceContext* deviceContext)
{
	D3D11_MAPPED_SUBRESOURCE MappedResource10_2 = { 0 };
	if (copyTo1)
	{
		deviceContext->CopyResource(intersectCenters_Result1_Buffer, intersectCenters_Buffer);
		deviceContext->Map(intersectCenters_Result2_Buffer, 0, D3D11_MAP_READ, 0, &MappedResource10_2);
	}
	else
	{
		deviceContext->CopyResource(intersectCenters_Result2_Buffer, intersectCenters_Buffer);
		deviceContext->Map(intersectCenters_Result1_Buffer, 0, D3D11_MAP_READ, 0, &MappedResource10_2);
	}
	
	_Analysis_assume_(MappedResource10_2.pData);
	assert(MappedResource10_2.pData);
	Vertex* results10_2_IntersectionPoints = (Vertex*)MappedResource10_2.pData;
	if (copyTo1)
		deviceContext->Unmap(intersectCenters_Result2_Buffer, 0);
	else
		deviceContext->Unmap(intersectCenters_Result1_Buffer, 0);
}

void blobb(bool copyTo1, ID3D11Buffer* intersectCenters_Result1_Buffer, ID3D11Buffer* intersectCenters_Result2_Buffer, ID3D11Buffer* intersectCenters_Buffer, ID3D11DeviceContext* deviceContext)
{}

void CollisionDetectionManager::_10_TriangleIntersections_GetFinalResult()
{
	SAFEDELETEARRAY(m_Results10_1_IntersectingObjects);
	m_Results10_1_IntersectingObjects = new UINT[m_ObjectCount];
	D3D11_MAPPED_SUBRESOURCE MappedResource10_1 = { 0 };
	deviceContext->CopyResource(m_IntersectingObjects_Result_Buffer, m_IntersectingObjects_Buffer);
	deviceContext->Map(m_IntersectingObjects_Result_Buffer, 0, D3D11_MAP_READ, 0, &MappedResource10_1);
	_Analysis_assume_(MappedResource10_1.pData);
	assert(MappedResource10_1.pData);
	memcpy(m_Results10_1_IntersectingObjects, MappedResource10_1.pData, m_ObjectCount * sizeof(UINT));
	deviceContext->Unmap(m_IntersectingObjects_Result_Buffer, 0);

	//m_CopyBackThread = new thread(AsyncCopyFromGPU, m_CopyTo1, m_IntersectCenters_Result1_Buffer, m_IntersectCenters_Result2_Buffer, m_IntersectCenters_Buffer, deviceContext);

	if (m_CopyBackThread != NULL)
		m_CopyBackThread->join();
	delete m_CopyBackThread;
	m_CopyBackThread = new thread(AsyncCopyFromGPU, m_CopyTo1, m_IntersectCenters_Result1_Buffer, m_IntersectCenters_Result2_Buffer, m_IntersectCenters_Buffer, deviceContext);

	/*D3D11_MAPPED_SUBRESOURCE MappedResource10_2 = { 0 };

	if (m_CopyTo1)
	{
		deviceContext->CopyResource(m_IntersectCenters_Result1_Buffer, m_IntersectCenters_Buffer);
		deviceContext->Map(m_IntersectCenters_Result2_Buffer, 0, D3D11_MAP_READ, 0, &MappedResource10_2);
	}
	else
	{
		deviceContext->CopyResource(m_IntersectCenters_Result2_Buffer, m_IntersectCenters_Buffer);
		deviceContext->Map(m_IntersectCenters_Result1_Buffer, 0, D3D11_MAP_READ, 0, &MappedResource10_2);
	}
	
	_Analysis_assume_(MappedResource10_2.pData);
	assert(MappedResource10_2.pData);
	m_Results10_2_IntersectionPoints = (Vertex*)MappedResource10_2.pData;
	if (m_CopyTo1)
		deviceContext->Unmap(m_IntersectCenters_Result2_Buffer, 0);
	else
		deviceContext->Unmap(m_IntersectCenters_Result1_Buffer, 0);*/


	/*int i = 0;
	for (i = 0; i < m_IntersectionCentersCount; i++)
	{
		if (m_Results10_2_IntersectionPoints[i].x == 0 && m_Results10_2_IntersectionPoints[i].y == 0 && m_Results10_2_IntersectionPoints[i].z == 0)
			break;
	}
	cout << "intersectionPoints : " << i << endl;*/

	
}
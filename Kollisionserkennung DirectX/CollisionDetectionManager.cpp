#include "CollisionDetectionManager.h"



CollisionDetectionManager::CollisionDetectionManager()
{
	m_SceneTriangles = 0;
}


CollisionDetectionManager::~CollisionDetectionManager()
{

}

void CollisionDetectionManager::CreateTriangleArray(vector<ModelClass*>* objects)
{
	Vertex *triangleVertices; // beinhaltet alle Punkte, also dreimal so viele wie es indices gibt
	int *indices;

	//Zuerst ausrechnen, wie groß das Triangle-Array sein sollte
	int vertexCount = 0;
	for (ModelClass *aktModel : *objects) 
	{
		vertexCount += aktModel->GetIndexCount();
	}

	triangleVertices = new Vertex[vertexCount];
	indices = new int[vertexCount]; // pro Dreieck ein Index (Index zeigt immer auf Anfang eines Dreierblocks in triangleVertices)

	int aktGlobalPosition = 0;
	// gehe über alle Objekte
	for (ModelClass *aktModel : *objects)
	{
		VertexAndVertexDataType* modelData = aktModel->GetModelData(); // hole die rohen Objektdaten (ein Eintrag ist ein Punkt, seine Texturkoordinaten und Normale, wir wollen aber nur den Punkt)
		for (int i = 0; i < aktModel->GetIndexCount(); i++) // iteriere über jeden Vertex in modelData
		{
			triangleVertices[aktGlobalPosition] = { modelData[i].x,  modelData[i].y, modelData[i].z };
			indices[aktGlobalPosition] = aktGlobalPosition; // teile durch 3, um die Indexe der Dreiecke zu speichern, nicht wo ein Dreieck im VertexBuffer anfängt
			aktGlobalPosition++;
		}
	}

	if (triangleVertices)
		delete[] triangleVertices;
	if (indices)
		delete[] indices;
}

void CollisionDetectionManager::ReleaseArrays()
{
	if (m_SceneTriangles)
	{
		delete[] m_SceneTriangles;
		m_SceneTriangles = 0;
	}
	return;
}

void CollisionDetectionManager::Shutdown()
{
}

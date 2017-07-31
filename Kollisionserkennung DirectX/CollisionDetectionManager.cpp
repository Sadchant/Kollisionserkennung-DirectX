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
	vector<ModelClass*> localObjects = *objects;

	//Zuerste ausrechnen, wie groß das Triangle-Array sein sollte
	int triangleCount = 0;
	for (ModelClass *aktModel : *objects) 
	{
		triangleCount += aktModel->GetIndexCount();
	}
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

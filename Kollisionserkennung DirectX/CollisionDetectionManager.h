#pragma once

#include <vector>
#include "modelclass.h"

class CollisionDetectionManager
{
public:
	CollisionDetectionManager();
	~CollisionDetectionManager();
	void CreateTriangleArray(vector<ModelClass*>* objects);
	void ReleaseArrays();
	void Shutdown();

private:
	struct Vertex
	{
		float x, y, z;
	};

	Vertex* m_SceneTriangles;
};


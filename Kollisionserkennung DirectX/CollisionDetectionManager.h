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

private:
	struct Triangle
	{
		float x, y, z;
	};

	Triangle* m_SceneTriangles;
};


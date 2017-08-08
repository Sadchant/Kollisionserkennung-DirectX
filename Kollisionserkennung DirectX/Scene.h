#ifndef _Scene_H_
#define _Scene_H_

#include <vector>
#include "d3dclass.h"
#include "cameraclass.h"
#include "modelclass.h"
//#include "colorshaderclass.h"
//#include "textureshaderclass.h"

#include "lightshaderclass.h"
#include "lightclass.h"
#include "CollisionDetectionManager.h"

using namespace std;

const bool FULL_SCREEN = false;
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.1f;

class Scene
{
public:
	Scene();
	Scene(const Scene&);
	~Scene();

	bool Initialize(int, int, HWND);
	void Shutdown();
	bool Frame();

private:
	bool Render(float);
	bool LoadObjects(ID3D11Device* device, HWND hwnd);

private:
	D3DClass* m_Direct3D;
	CameraClass* m_Camera;
	// ModelClass* m_Model;
	vector<ModelClass*> m_Objects;
	//ColorShaderClass* m_ColorShader;
	//TextureShaderClass* m_TextureShader;
	// LightShaderClass* m_LightShader;
	LightClass* m_Light;

	CollisionDetectionManager* m_CollisionDetectionManager;

};

#endif
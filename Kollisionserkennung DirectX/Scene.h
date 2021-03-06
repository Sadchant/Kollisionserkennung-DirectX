#ifndef _Scene_H_
#define _Scene_H_

// ***Quelle***: http://www.rastertek.com/tutdx11.html


#include <vector>
#include "d3dclass.h"
#include "cameraclass.h"
#include "modelclass.h"
//#include "colorshaderclass.h"
//#include "textureshaderclass.h"

#include "lightshaderclass.h"
#include "lightclass.h"
#include "CollisionDetectionManager.h"
#include "TextClass.h"
#include "GlobalConfig.h"

using namespace std;

const bool FULL_SCREEN = false;
const bool VSYNC_ENABLED = false;
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
	bool Frame(int, int, float);

private:
	bool Render(float);
	bool LoadScene(ID3D11Device* device, HWND hwnd);

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

	// Geh�rt wegen der Umbennenung (hie� ja eig GraphicsClass) zwar nicht mehr ganz in eine Szene, aber was solls
	TextClass* m_Text;

	int m_Scene;
};

#endif
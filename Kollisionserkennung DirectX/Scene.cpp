#include "Scene.h"


Scene::Scene()
{
	m_Direct3D = 0;
	m_Camera = 0;
	//m_ColorShader = 0;
	//m_TextureShader = 0;
	//m_LightShader = 0;
	m_Light = 0;
	m_CollisionDetectionManager = 0;
	m_Text = 0;
}

Scene::Scene(const Scene& other)
{
}


Scene::~Scene()
{
}


bool Scene::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
	bool result;
	XMMATRIX baseViewMatrix;

	// Create the Direct3D object.
	m_Direct3D = new D3DClass;
	if (!m_Direct3D)
	{
		return false;
	}

	// Initialize the Direct3D object.
	result = m_Direct3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	// Create the camera object.
	m_Camera = new CameraClass;
	if (!m_Camera)
	{
		return false;
	}

	// Initialize a base view matrix with the camera for 2D user interface rendering.
	m_Camera->SetPosition(0.0f, 0.0f, -5.0f);
	m_Camera->Render();
	m_Camera->GetViewMatrix(baseViewMatrix);

	// Initialize the model object.
	if (LARGESCENE)
		result = LoadBigObjects(m_Direct3D->GetDevice(), hwnd);
	else
		result = LoadSmallObjects(m_Direct3D->GetDevice(), hwnd);

	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the model object.", L"Error", MB_OK);
		return false;
	}

	//// Create the color shader object.
	//m_ColorShader = new ColorShaderClass;
	//if (!m_ColorShader)
	//{
	//	return false;
	//}

	//// Initialize the color shader object.
	//result = m_ColorShader->Initialize(m_Direct3D->GetDevice(), hwnd);
	//if (!result)
	//{
	//	MessageBox(hwnd, L"Could not initialize the color shader object.", L"Error", MB_OK);
	//	return false;
	//}


	//// Create the texture shader object.
	//m_TextureShader = new TextureShaderClass;
	//if (!m_TextureShader)
	//{
	//	return false;
	//}

	//// Initialize the texture shader object.
	//result = m_TextureShader->Initialize(m_Direct3D->GetDevice(), hwnd);
	//if (!result)
	//{
	//	MessageBox(hwnd, L"Could not initialize the texture shader object.", L"Error", MB_OK);
	//	return false;
	//}

	// The new light shader object is created and initialized here.
	// Create the light shader object.
	//m_LightShader = new LightShaderClass;
	//if (!m_LightShader)
	//{
	//	return false;
	//}

	//// Initialize the light shader object.
	//result = m_LightShader->Initialize(m_Direct3D->GetDevice(), hwnd);
	//if (!result)
	//{
	//	MessageBox(hwnd, L"Could not initialize the light shader object.", L"Error", MB_OK);
	//	return false;
	//}
	// The new light object is created here.
		// Create the light object.
	m_Light = new LightClass;
	if (!m_Light)
	{
		return false;
	}
	// The color of the light is set to purple and the light direction is set to point down the positive Z axis.
	// Initialize the light object.
	m_Light->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_Light->SetDirection(0.0f, 1.0f, 1.0f);

	// Create the text object.
	m_Text = new TextClass;
	if (!m_Text)
	{
		return false;
	}

	// Initialize the text object.
	result = m_Text->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), hwnd, screenWidth, screenHeight, baseViewMatrix);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the text object.", L"Error", MB_OK);
		return false;
	}



	m_CollisionDetectionManager = new CollisionDetectionManager();
	m_CollisionDetectionManager->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), hwnd, &m_Objects);

	return true;
}


void Scene::Shutdown()
{
	//// Release the color shader object.
	//if (m_ColorShader)
	//{
	//	m_ColorShader->Shutdown();
	//	delete m_ColorShader;
	//	m_ColorShader = 0;
	//}

	// Release the texture shader object.
	/*if (m_TextureShader)
	{
		m_TextureShader->Shutdown();
		delete m_TextureShader;
		m_TextureShader = 0;
	}*/

	// Release the light object.
	if (m_Light)
	{
		delete m_Light;
		m_Light = 0;
	}

	// Release the light shader object.
	/*if (m_LightShader)
	{
		m_LightShader->Shutdown();
		delete m_LightShader;
		m_LightShader = 0;
	}*/


	for (int i = 0; i < m_Objects.size(); i++)
	{
		ModelClass* curModelClass = m_Objects[i];
		curModelClass->Shutdown();
		delete curModelClass;
	}
	m_Objects.clear();

	// Release the camera object.
	if (m_Camera)
	{
		delete m_Camera;
		m_Camera = 0;
	}
	if (m_Direct3D)
	{
		m_Direct3D->Shutdown();
		delete m_Direct3D;
		m_Direct3D = 0;
	}
	// Release the text object.
	if (m_Text)
	{
		m_Text->Shutdown();
		delete m_Text;
		m_Text = 0;
	}

	if (m_CollisionDetectionManager) 
	{
		m_CollisionDetectionManager->Shutdown();
		delete m_CollisionDetectionManager;
		m_CollisionDetectionManager = 0;
	}
	return;
}


bool Scene::Frame(int fps, int cpu, float frameTime)
{
	bool result;

	// Set the frames per second.
	result = m_Text->SetFps(fps, m_Direct3D->GetDeviceContext());
	if (!result)
	{
		return false;
	}

	// Set the cpu usage.
	result = m_Text->SetCpu(cpu, m_Direct3D->GetDeviceContext());
	if (!result)
	{
		return false;
	}


	static float rotation = 0.0f;


	// Update the rotation variable each frame.
	/*rotation += (float)XM_PI * 0.01f;
	if (rotation > 360.0f)
	{
		rotation -= 360.0f;
	}*/


	// Render the graphics scene.
	result = Render(rotation);
	if (!result)
	{
		return false;
	}
	return true;
}


bool Scene::Render(float rotation)
{
	XMMATRIX worldMatrix, viewMatrix, projectionMatrix, orthoMatrix;
	bool result;


	// Clear the buffers to begin the scene.
	m_Direct3D->BeginScene(1.0f, 1.0f, 1.0f, 1.0f);

	// Generate the view matrix based on the camera's position.
	m_Camera->Render();

	// Get the world, view, and projection matrices from the camera and d3d objects.
	m_Direct3D->GetWorldMatrix(worldMatrix);
	m_Camera->GetViewMatrix(viewMatrix);
	m_Direct3D->GetProjectionMatrix(projectionMatrix);
	m_Direct3D->GetOrthoMatrix(orthoMatrix);


	// ######################### 2D #########################

	// Wireframe-Fillmode für 2D-Rendering deaktivieren
	m_Direct3D->TurnOffWireframeFillMode();


	// Turn off the Z buffer to begin all 2D rendering.
	m_Direct3D->TurnZBufferOff();

	// Here we turn on alpha blending so the text will blend with the background.
	// Turn on the alpha blending before rendering the text.
	m_Direct3D->TurnOnAlphaBlending();
	// We call the text object to render all its sentences to the screen here.And just like with 2D images we disable the Z buffer before drawing and then enable it again after all the 2D has been drawn.
	// Render the text strings.
	result = m_Text->Render(m_Direct3D->GetDeviceContext(), worldMatrix, orthoMatrix);
	if (!result)
	{
		return false;
	}
	// Here we turn off alpha blending so anything else that is drawn will not alpha blend with the objects behind it.
	// Turn off alpha blending after rendering the text.
	m_Direct3D->TurnOffAlphaBlending();

	// Turn the Z buffer back on now that all 2D rendering has completed.
	m_Direct3D->TurnZBufferOn();

	// Wireframe-Fillmode wieder aktivieren
	m_Direct3D->TurnOnWireframeFillMode();

	//Here we rotate the world matrix by the rotation value so that when we render the triangle using this updated world matrix it will spin the triangle by the rotation amount.
	// Rotate the world matrix by the rotation value so that the triangle will spin.
	worldMatrix = XMMatrixRotationY(rotation);

	// Put the model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	for (ModelClass* object : m_Objects)
	{
		result = object->Render(m_Direct3D->GetDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, m_Light->GetDirection(), m_Light->GetDiffuseColor());
		if (!result)
		{
			return false;
		}
	}

	// ######################### 2D #########################
	

	// Present the rendered scene to the screen.
	m_Direct3D->EndScene();

	m_CollisionDetectionManager->Frame();

	return true;
}

bool Scene::LoadSmallObjects(ID3D11Device *device, HWND hwnd)
{
	bool result;
	ModelClass *curModel = new ModelClass(); // braucht am Ende nicht deleted zu werden weil m_Objects die Referenz hält
	result = curModel->Initialize(device, "../Kollisionserkennung DirectX/data/Objekt_1.txt", hwnd);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the first object.", L"Error", MB_OK);
		return false;
	}
	m_Objects.push_back(curModel);

	curModel = new ModelClass();
	result = curModel->Initialize(device, "../Kollisionserkennung DirectX/data/Objekt_2.txt", hwnd);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the first object.", L"Error", MB_OK);
		return false;
	}
	m_Objects.push_back(curModel);

	return true;
}

bool Scene::LoadBigObjects(ID3D11Device *device, HWND hwnd)
{
	bool result;
	ModelClass *curModel = new ModelClass(); // braucht am Ende nicht deleted zu werden weil m_Objects die Referenz hält
	result = curModel->Initialize(device, "../Kollisionserkennung DirectX/data/Kran.txt", hwnd);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the first object.", L"Error", MB_OK);
	}

	for (int i = 0; i < 2; i++)
	{
		m_Objects.push_back(curModel);
	}

	return true;
}
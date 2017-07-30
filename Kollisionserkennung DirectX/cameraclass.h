#pragma once
// We have examined how to code HLSL shaders, how to setup vertex and index buffers, and how to invoke the HLSL shaders to draw those buffers using the ColorShaderClass.
// The one thing we are missing however is the view point to draw them from. For this we will require a camera class to let DirectX 11 know from where and also how we are
// viewing the scene. The camera class will keep track of where the camera is and its current rotation. It will use the position and rotation information to generate a view
// matrix which will be passed into the HLSL shader for rendering.


#include <directxmath.h>
using namespace DirectX;


class CameraClass
{
public:
	CameraClass();
	CameraClass(const CameraClass&);
	~CameraClass();

	void SetPosition(float, float, float);
	void SetRotation(float, float, float);

	XMFLOAT3 GetPosition();
	XMFLOAT3 GetRotation();

	void Render();
	void GetViewMatrix(XMMATRIX&);

private:
	float m_positionX, m_positionY, m_positionZ;
	float m_rotationX, m_rotationY, m_rotationZ;
	XMMATRIX m_viewMatrix;
};

// The CameraClass header is quite simple with just four functions that will be used. The SetPosition and SetRotation functions will be used to set the position and rotation
// of the camera object. Render will be used to create the view matrix based on the position and rotation of the camera. And finally GetViewMatrix will be used to retrieve the
// view matrix from the camera object so that the shaders can use it for rendering. 
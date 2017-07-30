#include "cameraclass.h"

// The class constructor will initialize the position and rotation of the camera to be at the origin of the scene.
CameraClass::CameraClass()
{
	m_positionX = 0.0f;
	m_positionY = 0.0f;
	m_positionZ = 0.0f;

	m_rotationX = 0.0f;
	m_rotationY = 0.0f;
	m_rotationZ = 0.0f;
}


CameraClass::CameraClass(const CameraClass& other)
{
}


CameraClass::~CameraClass()
{
}

// The SetPosition and SetRotation functions are used for setting up the position and rotation of the camera.

void CameraClass::SetPosition(float x, float y, float z)
{
	m_positionX = x;
	m_positionY = y;
	m_positionZ = z;
	return;
}


void CameraClass::SetRotation(float x, float y, float z)
{
	m_rotationX = x;
	m_rotationY = y;
	m_rotationZ = z;
	return;
}

// The GetPosition and GetRotation functions return the location and rotation of the camera to calling functions.

XMFLOAT3 CameraClass::GetPosition()
{
	return XMFLOAT3(m_positionX, m_positionY, m_positionZ);
}


XMFLOAT3 CameraClass::GetRotation()
{
	return XMFLOAT3(m_rotationX, m_rotationY, m_rotationZ);
}

// The Render function uses the position and rotation of the camera to build and update the view matrix.We first setup our variables for up, position, rotation,
// and so forth. Then at the origin of the scene we first rotate the camera based on the x, y, and z rotation of the camera. Once it is properly rotated when then
// translate the camera to the position in 3D space. With the correct values in the position, lookAt, and up we can then use the XMMatrixLookAtLH function to create
// the view matrix to represent the current camera rotation and translation.
void CameraClass::Render()
{
	XMFLOAT3 up, position, lookAt;
	XMVECTOR upVector, positionVector, lookAtVector; // XMVECTOR: A portable type used to represent a vector of four 32 - bit floating - point or integer components, 
													 // each aligned optimally and mapped to a hardware vector register.
	float yaw, pitch, roll;
	XMMATRIX rotationMatrix;


	// Setup the vector that points upwards.
	up.x = 0.0f;
	up.y = 1.0f;
	up.z = 0.0f;

	// Load it into a XMVECTOR structure.
	upVector = XMLoadFloat3(&up);

	// Setup the position of the camera in the world.
	position.x = m_positionX;
	position.y = m_positionY;
	position.z = m_positionZ;

	// Load it into a XMVECTOR structure.
	positionVector = XMLoadFloat3(&position);

	// Setup where the camera is looking by default.
	lookAt.x = 0.0f;
	lookAt.y = 0.0f;
	lookAt.z = 1.0f;

	// Load it into a XMVECTOR structure.
	lookAtVector = XMLoadFloat3(&lookAt);

	// Set the yaw (Y axis), pitch (X axis), and roll (Z axis) rotations in radians.
	pitch = m_rotationX * 0.0174532925f;	// x-Achse
	yaw = m_rotationY * 0.0174532925f;		// y-Achse
	roll = m_rotationZ * 0.0174532925f;		// z-Achse

	// Create the rotation matrix from the yaw, pitch, and roll values.
	// XMMatrixRotationRollPitchYaw: Builds a rotation matrix based on a given pitch, yaw, and roll (Euler angles).
	rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

	// Transform the lookAt and up vector by the rotation matrix so the view is correctly rotated at the origin.
	// XMVector3TransformCoord: Transforms a 3D vector by a given matrix
	lookAtVector = XMVector3TransformCoord(lookAtVector, rotationMatrix);
	upVector = XMVector3TransformCoord(upVector, rotationMatrix);

	// Translate the rotated camera position to the location of the viewer.
	// XMVectorAdd: Computes the sum of two vectors.
	lookAtVector = XMVectorAdd(positionVector, lookAtVector);

	// Finally create the view matrix from the three updated vectors.
	// XMMatrixLookAtLH: Builds a view matrix for a left-handed coordinate system using a camera position, an up direction, and a focal point.
	m_viewMatrix = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);

	return;
}

// After the Render function has been called to create the view matrix we can provide the updated view matrix to calling functions using this GetViewMatrix function.
// The view matrix will be one of the three main matrices used in the HLSL vertex shader.

void CameraClass::GetViewMatrix(XMMATRIX& viewMatrix)
{
	viewMatrix = m_viewMatrix;
	return;
}

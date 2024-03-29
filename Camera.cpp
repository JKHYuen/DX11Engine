#include "Camera.h"
#include <cmath>
#include <algorithm>

void Camera::SetPosition(float x, float y, float z) {
	m_PositionX = x;
	m_PositionY = y;
	m_PositionZ = z;
}

void Camera::SetRotation(float x, float y, float z) {
	m_RotationX = std::fmod(x, 360.0f);
	m_RotationY = std::clamp(y, -89.9f, 89.9f);
	m_RotationZ = std::fmod(z, 360.0f);
}

XMFLOAT3 Camera::GetPosition() {
	return XMFLOAT3(m_PositionX, m_PositionY, m_PositionZ);
}

XMFLOAT3 Camera::GetRotation() {
	return XMFLOAT3(m_RotationX, m_RotationY, m_RotationZ);
}

void Camera::Render() {
	XMFLOAT3 up, position, lookAt;
	XMVECTOR upVector, positionVector, lookAtVector;
	float yaw, pitch, roll;
	XMMATRIX rotationMatrix;

	// Setup the vector that points upwards.
	up.x = 0.0f;
	up.y = 1.0f;
	up.z = 0.0f;

	// Load it into a XMVECTOR structure.
	upVector = XMLoadFloat3(&up);

	// Setup the position of the camera in the world.
	position.x = m_PositionX;
	position.y = m_PositionY;
	position.z = m_PositionZ;

	// Load it into a XMVECTOR structure.
	positionVector = XMLoadFloat3(&position);

	// Setup where the camera is looking by default.
	lookAt.x = 0.0f;
	lookAt.y = 0.0f;
	lookAt.z = 1.0f;

	// Load it into a XMVECTOR structure.
	lookAtVector = XMLoadFloat3(&lookAt);

	// Set the yaw (Y axis), pitch (X axis), and roll (Z axis) rotations in radians.
	pitch = XMConvertToRadians(m_RotationY);
	yaw   = XMConvertToRadians(m_RotationX);
	roll  = XMConvertToRadians(m_RotationZ);

	// Create the rotation matrix from the yaw, pitch, and roll values.
	rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

	// Transform the lookAt vector by the rotation matrix so the view is correctly rotated at the origin.
	lookAtVector = XMVector3TransformCoord(lookAtVector, rotationMatrix);
	//upVector = XMVector3TransformCoord(upVector, rotationMatrix);

	XMStoreFloat3(&m_LookAtDir, XMVector3Normalize(lookAtVector));
	XMStoreFloat3(&m_RightDir, XMVector3Normalize(XMVector3Cross(upVector, lookAtVector)));

	// Translate the rotated camera position to the location of the viewer.
	lookAtVector = XMVectorAdd(positionVector, lookAtVector);

	// Finally create the view matrix from the three updated vectors.
	m_ViewMatrix = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);
}

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

void Camera::Update() {
	// Default up direction
	static XMFLOAT3 up {0.0f, 1.0f, 0.0f};
	XMVECTOR upVector = XMLoadFloat3(&up);

	// Default lookat direction
	static XMFLOAT3 lookAt {0.0f, 0.0f, 1.0f};
	XMVECTOR lookAtVector = XMLoadFloat3(&lookAt);

	// Setup the position of the camera in the world
	XMFLOAT3 position {m_PositionX, m_PositionY, m_PositionZ};
	XMVECTOR positionVector = XMLoadFloat3(&position);

	// Set the yaw (Y axis), pitch (X axis), and roll (Z axis) rotations in radians
	float pitch = XMConvertToRadians(m_RotationY);
	float yaw = XMConvertToRadians(m_RotationX);
	float roll = XMConvertToRadians(m_RotationZ);

	// Rotate look at vector *before translating*
	lookAtVector = XMVector3Rotate(lookAtVector, XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));

	XMStoreFloat3(&m_LookAtDir, XMVector3Normalize(lookAtVector));
	XMStoreFloat3(&m_RightDir, XMVector3Normalize(XMVector3Cross(upVector, lookAtVector)));

	// Translate the rotated camera position
	lookAtVector = XMVectorAdd(positionVector, lookAtVector);

	// Create the view matrix
	m_ViewMatrix = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);
}

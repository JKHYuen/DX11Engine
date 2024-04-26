#include "Camera.h"
#include <cmath>
#include <algorithm>
#include <iostream>

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

// From https://rastertek.com/dx11win10tut23.html
void Camera::UpdateFrustum(XMMATRIX projectionMatrix, float screenDepth) {
    // Load the projection matrix into a XMFLOAT4X4 structure.
    XMFLOAT4X4 projMatrix {};
    XMStoreFloat4x4(&projMatrix, projectionMatrix);

    // Calculate the minimum Z distance in the frustum.
    float zMinimum = -projMatrix._43 / projMatrix._33;
    float r = screenDepth / (screenDepth - zMinimum);
    projMatrix._33 = r;
    projMatrix._43 = -r * zMinimum;

    // Load the updated XMFLOAT4X4 back into the original projection matrix.
    projectionMatrix = XMLoadFloat4x4(&projMatrix);

    // Create the frustum matrix from the view matrix and updated projection matrix.
    XMMATRIX tempMatrix = XMMatrixMultiply(m_ViewMatrix, projectionMatrix);
    XMFLOAT4X4 finalMatrix {};
    XMStoreFloat4x4(&finalMatrix, tempMatrix);

    // Get the near plane of the frustum.
    m_FrustumPlanes[0].x = finalMatrix._13;
    m_FrustumPlanes[0].y = finalMatrix._23;
    m_FrustumPlanes[0].z = finalMatrix._33;
    m_FrustumPlanes[0].w = finalMatrix._43;

    // Normalize it.
    float t = (float)sqrt((m_FrustumPlanes[0].x * m_FrustumPlanes[0].x) + (m_FrustumPlanes[0].y * m_FrustumPlanes[0].y) + (m_FrustumPlanes[0].z * m_FrustumPlanes[0].z));
    m_FrustumPlanes[0].x /= t;
    m_FrustumPlanes[0].y /= t;
    m_FrustumPlanes[0].z /= t;
    m_FrustumPlanes[0].w /= t;

    // Calculate the far plane of frustum.
    m_FrustumPlanes[1].x = finalMatrix._14 - finalMatrix._13;
    m_FrustumPlanes[1].y = finalMatrix._24 - finalMatrix._23;
    m_FrustumPlanes[1].z = finalMatrix._34 - finalMatrix._33;
    m_FrustumPlanes[1].w = finalMatrix._44 - finalMatrix._43;

    // Normalize it.
    t = (float)sqrt((m_FrustumPlanes[1].x * m_FrustumPlanes[1].x) + (m_FrustumPlanes[1].y * m_FrustumPlanes[1].y) + (m_FrustumPlanes[1].z * m_FrustumPlanes[1].z));
    m_FrustumPlanes[1].x /= t;
    m_FrustumPlanes[1].y /= t;
    m_FrustumPlanes[1].z /= t;
    m_FrustumPlanes[1].w /= t;

    // Calculate the left plane of frustum.
    m_FrustumPlanes[2].x = finalMatrix._14 + finalMatrix._11;
    m_FrustumPlanes[2].y = finalMatrix._24 + finalMatrix._21;
    m_FrustumPlanes[2].z = finalMatrix._34 + finalMatrix._31;
    m_FrustumPlanes[2].w = finalMatrix._44 + finalMatrix._41;

    // Normalize it.
    t = (float)sqrt((m_FrustumPlanes[2].x * m_FrustumPlanes[2].x) + (m_FrustumPlanes[2].y * m_FrustumPlanes[2].y) + (m_FrustumPlanes[2].z * m_FrustumPlanes[2].z));
    m_FrustumPlanes[2].x /= t;
    m_FrustumPlanes[2].y /= t;
    m_FrustumPlanes[2].z /= t;
    m_FrustumPlanes[2].w /= t;

    // Calculate the right plane of frustum.
    m_FrustumPlanes[3].x = finalMatrix._14 - finalMatrix._11;
    m_FrustumPlanes[3].y = finalMatrix._24 - finalMatrix._21;
    m_FrustumPlanes[3].z = finalMatrix._34 - finalMatrix._31;
    m_FrustumPlanes[3].w = finalMatrix._44 - finalMatrix._41;

    // Normalize it.
    t = (float)sqrt((m_FrustumPlanes[3].x * m_FrustumPlanes[3].x) + (m_FrustumPlanes[3].y * m_FrustumPlanes[3].y) + (m_FrustumPlanes[3].z * m_FrustumPlanes[3].z));
    m_FrustumPlanes[3].x /= t;
    m_FrustumPlanes[3].y /= t;
    m_FrustumPlanes[3].z /= t;
    m_FrustumPlanes[3].w /= t;

    // Calculate the top plane of frustum.
    m_FrustumPlanes[4].x = finalMatrix._14 - finalMatrix._12;
    m_FrustumPlanes[4].y = finalMatrix._24 - finalMatrix._22;
    m_FrustumPlanes[4].z = finalMatrix._34 - finalMatrix._32;
    m_FrustumPlanes[4].w = finalMatrix._44 - finalMatrix._42;

    // Normalize it.
    t = (float)sqrt((m_FrustumPlanes[4].x * m_FrustumPlanes[4].x) + (m_FrustumPlanes[4].y * m_FrustumPlanes[4].y) + (m_FrustumPlanes[4].z * m_FrustumPlanes[4].z));
    m_FrustumPlanes[4].x /= t;
    m_FrustumPlanes[4].y /= t;
    m_FrustumPlanes[4].z /= t;
    m_FrustumPlanes[4].w /= t;

    // Calculate the bottom plane of frustum.
    m_FrustumPlanes[5].x = finalMatrix._14 + finalMatrix._12;
    m_FrustumPlanes[5].y = finalMatrix._24 + finalMatrix._22;
    m_FrustumPlanes[5].z = finalMatrix._34 + finalMatrix._32;
    m_FrustumPlanes[5].w = finalMatrix._44 + finalMatrix._42;

    // Normalize it.
    t = (float)sqrt((m_FrustumPlanes[5].x * m_FrustumPlanes[5].x) + (m_FrustumPlanes[5].y * m_FrustumPlanes[5].y) + (m_FrustumPlanes[5].z * m_FrustumPlanes[5].z));
    m_FrustumPlanes[5].x /= t;
    m_FrustumPlanes[5].y /= t;
    m_FrustumPlanes[5].z /= t;
    m_FrustumPlanes[5].w /= t;
}

bool Camera::CheckRectangleInFrustum(float xCenter, float yCenter, float zCenter, float xSize, float ySize, float zSize, float bias) {
    for(int i = 0; i < 6; i++) {
        if(m_FrustumPlanes[i].x * (xCenter - xSize) +
           m_FrustumPlanes[i].y * (yCenter - ySize) +
           m_FrustumPlanes[i].z * (zCenter - zSize) + m_FrustumPlanes[i].w >= bias)
        {
            continue;
        }

        if(m_FrustumPlanes[i].x * (xCenter + xSize) +
           m_FrustumPlanes[i].y * (yCenter - ySize) +
           m_FrustumPlanes[i].z * (zCenter - zSize) + m_FrustumPlanes[i].w >= bias)
        {
            continue;
        }

        if(m_FrustumPlanes[i].x * (xCenter - xSize) +
           m_FrustumPlanes[i].y * (yCenter + ySize) +
           m_FrustumPlanes[i].z * (zCenter - zSize) + m_FrustumPlanes[i].w >= bias)
        {
            continue;
        }

        if(m_FrustumPlanes[i].x * (xCenter - xSize) +
           m_FrustumPlanes[i].y * (yCenter - ySize) +
           m_FrustumPlanes[i].z * (zCenter + zSize) + m_FrustumPlanes[i].w >= bias)
        {
            continue;
        }

        if(m_FrustumPlanes[i].x * (xCenter + xSize) +
           m_FrustumPlanes[i].y * (yCenter + ySize) +
           m_FrustumPlanes[i].z * (zCenter - zSize) + m_FrustumPlanes[i].w >= bias)
        {
            continue;
        }

        if(m_FrustumPlanes[i].x * (xCenter + xSize) +
           m_FrustumPlanes[i].y * (yCenter - ySize) +
           m_FrustumPlanes[i].z * (zCenter + zSize) + m_FrustumPlanes[i].w >= bias)
        {
            continue;
        }

        if(m_FrustumPlanes[i].x * (xCenter - xSize) +
           m_FrustumPlanes[i].y * (yCenter + ySize) +
           m_FrustumPlanes[i].z * (zCenter + zSize) + m_FrustumPlanes[i].w >= bias)
        {
            continue;
        }

        if(m_FrustumPlanes[i].x * (xCenter + xSize) +
           m_FrustumPlanes[i].y * (yCenter + ySize) +
           m_FrustumPlanes[i].z * (zCenter + zSize) + m_FrustumPlanes[i].w >= bias)
        {
            continue;
        }

        return false;
    }

    return true;
}

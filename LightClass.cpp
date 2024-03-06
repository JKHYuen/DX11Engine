#include "LightClass.h"

LightClass::LightClass() {}
LightClass::LightClass(const LightClass& other) {}
LightClass::~LightClass() {}

void LightClass::SetDirectionalColor(float red, float green, float blue, float alpha) {
    m_DirectionalColor = XMFLOAT4(red, green, blue, alpha);
}

void LightClass::SetDirection(float x, float y, float z) {
    m_Direction = XMFLOAT3(x, y, z);
}

void LightClass::SetLookAt(float x, float y, float z) {
    m_LookAt.x = x;
    m_LookAt.y = y;
    m_LookAt.z = z;
}

void LightClass::SetPosition(float x, float y, float z) {
    m_Position.x = x;
    m_Position.y = y;
    m_Position.z = z;
}

XMFLOAT4 LightClass::GetDirectionalColor() {
    return m_DirectionalColor;
}

XMFLOAT3 LightClass::GetDirection() {
    return m_Direction;
}

XMFLOAT3 LightClass::GetPosition() {
    return m_Position;
}

void LightClass::GenerateOrthoMatrix(float width, float depthPlane, float nearPlane) {
    m_OrthoMatrix = XMMatrixOrthographicLH(width, width, nearPlane, depthPlane);
}

void LightClass::GetOrthoMatrix(XMMATRIX& orthoMatrix) {
    orthoMatrix = m_OrthoMatrix;
}

void LightClass::GenerateViewMatrix() {
	XMFLOAT3 up { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 lookAt { 0.0f, 0.0f, 0.0f };
    m_ViewMatrix = XMMatrixLookAtLH(XMLoadFloat3(&m_Position), XMLoadFloat3(&lookAt), XMLoadFloat3(&up));
}

void LightClass::GetViewMatrix(XMMATRIX& viewMatrix) {
    viewMatrix = m_ViewMatrix;
}

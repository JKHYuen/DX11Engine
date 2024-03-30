#include "DirectionalLight.h"
#include <cmath>

void DirectionalLight::SetColor(float red, float green, float blue, float alpha) {
    m_DirectionalColor = XMFLOAT4(red, green, blue, alpha);
}

void DirectionalLight::SetDirection(float rotX, float rotY, float rotZ) {
    rotX = std::fmod(rotX, 360.0f);
    rotY = std::fmod(rotY, 360.0f);
    rotZ = std::fmod(rotZ, 360.0f);

    SetQuaternionDirection(XMQuaternionRotationRollPitchYaw(rotX, rotY, rotZ));
}

void DirectionalLight::SetQuaternionDirection(XMVECTOR rotationQuaternion) {
    XMVECTOR dirVec = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    dirVec = XMVector3Rotate(dirVec, rotationQuaternion);

    XMStoreFloat3(&m_Direction, dirVec);

    m_Position.x = XMVectorGetX(dirVec) * m_LightDistance;
    m_Position.y = XMVectorGetY(dirVec) * m_LightDistance;
    m_Position.z = XMVectorGetZ(dirVec) * m_LightDistance;

    // Regenerate view matrix
    GenerateViewMatrix();
}

void DirectionalLight::GenerateOrthoMatrix(float width, float nearPlane, float depthPlane) {
    m_OrthoMatrix = XMMatrixOrthographicLH(width, width, nearPlane, depthPlane);
}

void DirectionalLight::GenerateViewMatrix() {
    static XMFLOAT3 up {0.0f, 1.0f, 0.0f};
    // Always look at origin, direction determined by m_Position
	static XMFLOAT3 lookAt { 0.0f, 0.0f, 0.0f };
    m_ViewMatrix = XMMatrixLookAtLH(XMLoadFloat3(&m_Position), XMLoadFloat3(&lookAt), XMLoadFloat3(&up));
}

// Note: a bit hacky
void DirectionalLight::GetEulerAngles(float& rotX, float& rotY) const {
    XMVECTOR normDirVec = XMLoadFloat3(&m_Position);
    float x = -XMVectorGetX(normDirVec);
    float z = -XMVectorGetY(normDirVec);
    float y = XMVectorGetZ(normDirVec);

    float r = XMVectorGetX(XMVector3Length(normDirVec));
    float t = std::atan2(y, x);
    float p = std::acos(z / r);

    rotX = XMConvertToDegrees(p) - 90;
    rotY = XMConvertToDegrees(t) + 90;
}

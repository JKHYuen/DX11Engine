#include "DirectionalLight.h"
#include <iostream>

void DirectionalLight::SetColor(float red, float green, float blue, float alpha) {
    m_DirectionalColor = XMFLOAT4(red, green, blue, alpha);
}

void DirectionalLight::SetDirection(float rotX, float rotY, float rotZ) {
    rotX = std::fmod(rotX, 360.0f);
    rotY = std::fmod(rotY, 360.0f);
    rotZ = std::fmod(rotZ, 360.0f);

    // default direction
    XMVECTOR dirVec = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    dirVec = XMVector3TransformCoord(dirVec, XMMatrixRotationRollPitchYaw(rotX, rotY, rotZ));

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

#pragma once

#include <directxmath.h>
using namespace DirectX;

class DirectionalLight {
public:
    DirectionalLight() {};
    DirectionalLight(const DirectionalLight&) {};
    ~DirectionalLight() {};

    void SetColor(float, float, float, float);

    // Sets direction of light, by rotating default direction (0.0, 0.0, 1.0) by rotation parameters (radians)
    // Updates m_Position and calls GenerateViewMatrix
    void SetQuaternionDirection(XMVECTOR rotationQuaternion);

    void SetLightDistance(float newDistance) { m_LightDistance = newDistance; }

    void GenerateOrthoMatrix(float width, float nearPlane, float depthPlane);
    void GenerateViewMatrix();

    XMFLOAT4 GetDirectionalColor() const { return m_DirectionalColor; }
    XMFLOAT3 GetPosition() const { return m_Position; }
    
    XMFLOAT3 GetDirection() const { return m_Direction; }
    void SetDirection(float rotX, float rotY, float rotZ);

    float GetShadowBias() const { return m_ShadowBias; }
    void SetShadowBias(float newValue) { m_ShadowBias = newValue; }

    void GetEulerAngles(float& rotX, float& rotY) const;

    void GetOrthoMatrix(XMMATRIX& orthoMatrix) const { orthoMatrix = m_OrthoMatrix; }
    void GetViewMatrix(XMMATRIX& viewMatrix) const { viewMatrix = m_ViewMatrix; }

private:
    float m_LightDistance = -15.0f;
    XMFLOAT4 m_DirectionalColor {};

    // normalized 3d free vector representing direction of light
    XMFLOAT3 m_Direction {};

    float m_ShadowBias {};

    // For shadow mapping
    XMFLOAT3 m_Position {};
    XMFLOAT3 m_LookAt {};
    XMMATRIX m_OrthoMatrix {};
    XMMATRIX m_ViewMatrix {};
};


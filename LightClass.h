#pragma once

#include <directxmath.h>
using namespace DirectX;

class LightClass {
public:
    LightClass();
    LightClass(const LightClass&);
    ~LightClass();

    void SetDirectionalColor(float, float, float, float);
    void SetDirection(float, float, float);
    void SetLookAt(float, float, float);
    void SetPosition(float, float, float);

    void GenerateOrthoMatrix(float width, float nearPlane, float depthPlane);
    void GetOrthoMatrix(XMMATRIX&);

    void GenerateViewMatrix();
    void GetViewMatrix(XMMATRIX&);

    XMFLOAT4 GetDirectionalColor();
    XMFLOAT3 GetDirection();

    XMFLOAT3 GetPosition();

private:
    XMFLOAT4 m_DirectionalColor {};
    XMFLOAT3 m_Direction {};
    XMFLOAT3 m_Position {};

    XMFLOAT3 m_LookAt {};
    XMMATRIX m_OrthoMatrix {};
    XMMATRIX m_ViewMatrix {};
};


#pragma once
#include <directxmath.h>
using namespace DirectX;

class CameraClass {
public:
	CameraClass();
	CameraClass(const CameraClass&);
	~CameraClass();

	void SetPosition(float, float, float);
	void SetRotation(float, float, float);

	XMFLOAT3 GetPosition();
	float GetPositionX() { return m_positionX; };
	float GetPositionY() { return m_positionY; };
	float GetPositionZ() { return m_positionZ; };

	XMFLOAT3 GetRotation();
	float GetRotationX() { return m_rotationX; };
	float GetRotationY() { return m_rotationY; };
	float GetRotationZ() { return m_rotationZ; };

	XMFLOAT3 GetLookAtDir() { return m_lookAtDir; };
	XMFLOAT3 GetRightDir() { return m_rightDir; };

	void Render();
	void GetViewMatrix(XMMATRIX&);

private:
	float m_positionX {}, m_positionY {}, m_positionZ {};
	float m_rotationX {}, m_rotationY {}, m_rotationZ {};

	XMFLOAT3 m_lookAtDir {};
	XMFLOAT3 m_rightDir {};
	XMMATRIX m_viewMatrix {};
};

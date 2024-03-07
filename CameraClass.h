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
	float GetPositionX() { return m_PositionX; };
	float GetPositionY() { return m_PositionY; };
	float GetPositionZ() { return m_PositionZ; };

	XMFLOAT3 GetRotation();
	float GetRotationX() { return m_RotationX; };
	float GetRotationY() { return m_RotationY; };
	float GetRotationZ() { return m_RotationZ; };

	XMFLOAT3 GetLookAtDir() { return m_LookAtDir; };
	XMFLOAT3 GetRightDir() { return m_RightDir; };

	void Render();
	void GetViewMatrix(XMMATRIX&);

private:
	float m_PositionX {}, m_PositionY {}, m_PositionZ {};
	float m_RotationX {}, m_RotationY {}, m_RotationZ {};

	XMFLOAT3 m_LookAtDir {};
	XMFLOAT3 m_RightDir {};
	XMMATRIX m_ViewMatrix {};
};

#pragma once
#include <array>

#include <DirectXMath.h>
using namespace DirectX;

class Camera {
public:
	Camera() {};
	Camera(const Camera&) {};
	~Camera() {};

	void SetPosition(float, float, float);
	void SetRotation(float, float, float);

	XMFLOAT3 GetPosition();
	float GetPositionX() const { return m_PositionX; }
	float GetPositionY() const { return m_PositionY; }
	float GetPositionZ() const { return m_PositionZ; }

	XMFLOAT3 GetRotation();
	float GetRotationX() const { return m_RotationX; }
	float GetRotationY() const { return m_RotationY; }
	float GetRotationZ() const { return m_RotationZ; }

	XMFLOAT3 GetLookAtDir() const { return m_LookAtDir; }
	XMFLOAT3 GetRightDir() const  { return m_RightDir; }

	void GetViewMatrix(XMMATRIX& viewMatrix) const { viewMatrix = m_ViewMatrix; }

	void Update();

	void UpdateFrustrum(XMMATRIX projectionMatrix, float screenDepth);
	bool CheckRectangleInFrustrum(float xCenter, float yCenter, float zCenter, float xSize, float ySize, float zSize);

private:
	float m_PositionX {}, m_PositionY {}, m_PositionZ {};
	float m_RotationX {}, m_RotationY {}, m_RotationZ {};

	XMFLOAT3 m_LookAtDir {};
	XMFLOAT3 m_RightDir {};
	XMMATRIX m_ViewMatrix {};

	std::array<XMFLOAT4, 6> m_Planes {};
};

#pragma once

#include <windows.h>
#include <directxmath.h>
#include <string>
#include <iostream>

class LightClass;
class ModelClass;
class LightShaderClass;
class DepthShaderClass;
class RenderTextureClass;
struct ID3D11Device;
struct ID3D11DeviceContext;

using namespace DirectX;

class GameObject {
public:
	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND windowHandle, const std::string& modelName, const std::string& textureName);

	bool Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, RenderTextureClass* shadowMap, LightClass* light, XMFLOAT3 cameraPos, float time);

	bool RenderToDepth(ID3D11DeviceContext* deviceContext, LightClass* light, float time);

	void Shutdown();

	void SetUVScale(float newScale) {
		m_UVScale = newScale;
	}

	void SetHeightMapScale(float newScale) {
		m_HeightMapScale = newScale;
	}

	void SetPosition(DirectX::XMFLOAT3 vec) {
		m_Position = vec;
	}

	void SetScale(DirectX::XMFLOAT3 newScale) {
		m_Scale = newScale;
	}

	void SetYRotationSpeed(float newSpeed) {
		m_RotationYSpeed = newSpeed;
	}

private:
	float m_RotationYSpeed {};
	float m_UVScale {};
	float m_HeightMapScale {};

	DirectX::XMFLOAT3 m_Position {};
	DirectX::XMFLOAT3 m_Scale { 1.0f, 1.0f, 1.0f };

	ModelClass* m_Model {};
	LightShaderClass* m_LightShader {};
	DepthShaderClass* m_DepthShader {};
};
#pragma once
#include <windows.h>
#include <directxmath.h>
#include <string>
#include <iostream>
#include <vector>

class DirectionalLight;
class Model;
class PBRShader;
class DepthShader;
class RenderTexture;
class Texture;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;

using namespace DirectX;

class GameObject {
public:
	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND windowHandle, PBRShader* pbrShaderInstance, DepthShader* depthShaderInstance, const std::vector<Texture*>& textures, Model* model);

	bool Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* shadowMap, ID3D11ShaderResourceView* irradianceMap, ID3D11ShaderResourceView* prefilteredMap, ID3D11ShaderResourceView* BRDFLut, DirectionalLight* light, XMFLOAT3 cameraPos, float time);

	bool RenderToDepth(ID3D11DeviceContext* deviceContext, DirectionalLight* light, float time);

	void Shutdown();

	void SetUVScale(float newScale) {
		m_UVScale = newScale;
	}

	void SetDisplacementMapHeightScale(float newScale) {
		m_DisplacementHeightScale = newScale;
	}

	void SetParallaxMapHeightScale(float newScale) {
		m_ParallaxHeightScale = newScale;
	}

	float GetParallaxMapHeightScale() const {
		return m_ParallaxHeightScale;
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

	void SetMaterialTextures(const std::vector<Texture*>& textures) {
		m_MaterialTextures = textures;
	}

private:
	float m_RotationYSpeed {};
	float m_UVScale {};
	float m_DisplacementHeightScale {};
	float m_ParallaxHeightScale {};

	DirectX::XMFLOAT3 m_Position {};
	DirectX::XMFLOAT3 m_Scale { 1.0f, 1.0f, 1.0f };

	Model* m_Model {};
	PBRShader* m_PBRShader {};
	DepthShader* m_DepthShader {};

	std::vector<Texture*> m_MaterialTextures {};
};
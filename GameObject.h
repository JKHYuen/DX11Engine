#pragma once
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
	struct GameObjectData {
		std::string modelName {};
		std::string materialName {};
		XMFLOAT3 position {};
		XMFLOAT3 scale {1.0f, 1.0f, 1.0f};
		float yRotSpeed {};
		float uvScale = 1.0f;
		float vertexDisplacementMapScale = 0.1f;
		float parallaxMapHeightScale = 0.0f;
	};

public:
	bool Initialize(PBRShader* pbrShaderInstance, DepthShader* depthShaderInstance, const std::vector<Texture*>& textures, Model* model, const GameObjectData& initialGameObjectData);

	bool Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* shadowMap, ID3D11ShaderResourceView* irradianceMap, ID3D11ShaderResourceView* prefilteredMap, ID3D11ShaderResourceView* BRDFLut, DirectionalLight* light, XMFLOAT3 cameraPos, float time);

	bool RenderToDepth(ID3D11DeviceContext* deviceContext, DirectionalLight* light, float time);

	void SetEnabled(bool state) { mb_IsEnabled = state; }
	bool GetEnabled() const { return mb_IsEnabled; }

	void SetUVScale(float newScale) { m_UVScale = newScale; }
	float GetUVScale() const { return m_UVScale; }

	void SetDisplacementMapHeightScale(float newScale) { m_DisplacementHeightScale = newScale; }
	float GetDisplacementMapHeightScale() const { return m_DisplacementHeightScale; }
	void SetParallaxMapHeightScale(float newScale) { m_ParallaxHeightScale = newScale; }
	float GetParallaxMapHeightScale() const { return m_ParallaxHeightScale; }
	void SetYRotationSpeed(float newSpeed) { m_RotationYSpeed = newSpeed; }
	float GetYRotationSpeed() const { return m_RotationYSpeed; }

	// Implemented this way (i.e. not using XMFLOAT3) for convenience in IMGUI
	void SetPosition(float x, float y, float z) { m_Position.x = x;	m_Position.y = y; m_Position.z = z; }
	void GetPosition(float& x, float& y, float& z) const { x = m_Position.x; y = m_Position.y; z = m_Position.z; }
	void SetScale(float x, float y, float z) { m_Scale.x = x; m_Scale.y = y; m_Scale.z = z; }
	void GetScale(float& x, float& y, float& z) const { x = m_Scale.x; y = m_Scale.y; z = m_Scale.z; }

	// m_PBRMaterialName needed for IMGUI
	void SetPBRMaterialTextures(std::string_view name, const std::vector<Texture*>& newTextures) { 
		m_PBRMaterialName = name;
		m_MaterialTextures = newTextures; 
	}
	std::string_view GetPBRMaterialName() const { return m_PBRMaterialName; }

	void SetModel(std::string_view name, Model* newModel) {
		m_ModelName = name;
		m_ModelInstance = newModel;
	}
	std::string_view GetModelName() const { return m_ModelName; }

private:
	bool mb_IsEnabled = true;
	std::string m_PBRMaterialName {};
	std::string m_ModelName {};

	float m_RotationYSpeed {};
	float m_UVScale {};
	float m_DisplacementHeightScale {};
	float m_ParallaxHeightScale {};

	DirectX::XMFLOAT3 m_Position {};
	DirectX::XMFLOAT3 m_Scale { 1.0f, 1.0f, 1.0f };

	Model* m_ModelInstance {};
	PBRShader* m_PBRShaderInstance {};
	DepthShader* m_DepthShaderInstance {};

	std::vector<Texture*> m_MaterialTextures {};
};
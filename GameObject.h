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
class Camera;
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
		float minRoughness = 0;
		bool useParallaxShadow = true;
		int minParallaxLayers = 8;
		int maxParallaxLayers = 32;
		float tessellationFactor = 1;
	};

public:
	bool Initialize(PBRShader* pbrShaderInstance, DepthShader* depthShaderInstance, const std::vector<Texture*>& textures, Model* model, const GameObjectData& initialGameObjectData);

	bool Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView* shadowMap, ID3D11ShaderResourceView* irradianceMap, ID3D11ShaderResourceView* prefilteredMap, ID3D11ShaderResourceView* BRDFLut, DirectionalLight* light, Camera* camera, float time);

	bool RenderToDepth(ID3D11DeviceContext* deviceContext, DirectionalLight* light, float time);

	void SetEnabled(bool state) { mb_IsEnabled = state; }
	bool GetEnabled() const { return mb_IsEnabled; }

	void SetUVScale(float newScale) { m_GameObjectData.uvScale = newScale; }
	float GetUVScale() const { return m_GameObjectData.uvScale; }
	void SetDisplacementMapHeightScale(float newScale) { m_GameObjectData.vertexDisplacementMapScale = newScale; }
	float GetDisplacementMapHeightScale() const { return m_GameObjectData.vertexDisplacementMapScale; }
	void SetParallaxMapHeightScale(float newScale) { m_GameObjectData.parallaxMapHeightScale = newScale; }
	float GetParallaxMapHeightScale() const { return m_GameObjectData.parallaxMapHeightScale; }
	void SetYRotationSpeed(float newSpeed) { m_GameObjectData.yRotSpeed = newSpeed; }
	float GetYRotationSpeed() const { return m_GameObjectData.yRotSpeed; }
	void SetMinRoughness(float newValue) { m_GameObjectData.minRoughness = newValue; }
	float GetMinRoughness() const { return m_GameObjectData.minRoughness; }

	void SetUseParallaxShadow(bool newValue) { m_GameObjectData.useParallaxShadow = newValue; }
	bool GetUseParallaxShadow() const { return m_GameObjectData.useParallaxShadow; }
	void SetMinParallaxLayers(int newValue) { m_GameObjectData.minParallaxLayers = newValue; }
	int GetMinParallaxLayers() const { return m_GameObjectData.minParallaxLayers; }
	void SetMaxParallaxLayers(int newValue) { m_GameObjectData.maxParallaxLayers = newValue; }
	int GetMaxParallaxLayers() const { return m_GameObjectData.maxParallaxLayers; }

	void SetTessellationFactor(float newValue) { m_GameObjectData.tessellationFactor = newValue; }
	float GetTesellationFactor() const { return m_GameObjectData.tessellationFactor; }

	// Implemented this way (i.e. not using XMFLOAT3) for convenience in IMGUI
	void SetPosition(float x, float y, float z) { m_GameObjectData.position.x = x;	m_GameObjectData.position.y = y; m_GameObjectData.position.z = z; }
	void GetPosition(float& x, float& y, float& z) const { x = m_GameObjectData.position.x; y = m_GameObjectData.position.y; z = m_GameObjectData.position.z; }
	void SetScale(float x, float y, float z) { m_GameObjectData.scale.x = x; m_GameObjectData.scale.y = y; m_GameObjectData.scale.z = z; }
	void GetScale(float& x, float& y, float& z) const { x = m_GameObjectData.scale.x; y = m_GameObjectData.scale.y; z = m_GameObjectData.scale.z; }

	// m_PBRMaterialName needed for IMGUI
	void SetPBRMaterialTextures(std::string_view name, const std::vector<Texture*>& newTextures) { 
		m_GameObjectData.materialName = name;
		m_MaterialTextures = newTextures; 
	}
	std::string_view GetPBRMaterialName() const { return m_GameObjectData.materialName; }

	void SetModel(std::string_view name, Model* newModel) {
		m_GameObjectData.modelName = name;
		m_ModelInstance = newModel;
	}
	std::string_view GetModelName() const { return m_GameObjectData.modelName; }

private:
	// Argument could be made that this should be a public variable
	GameObjectData m_GameObjectData {};
	bool mb_IsEnabled = true;

	Model* m_ModelInstance {};
	PBRShader* m_PBRShaderInstance {};
	DepthShader* m_DepthShaderInstance {};

	// Order of materialTextures array: albedoMap, normalMap, metallicMap, roughnessMap, aoMap, heightMap
	std::vector<Texture*> m_MaterialTextures {};
};
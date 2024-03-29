#include <windows.h>

#include <unordered_map>
#include <string>
#include <vector>
#include <DirectXMath.h>

class Texture;
class Model;
class GameObject;
class D3DInstance;
class PBRShader;
class DepthShader;
class CubeMapObject;
class RenderTexture;
class DirectionalLight;
class QuadModel;

class Scene {
public:
	Scene() {}
	Scene(const Texture& other) {}
	~Scene() {}

	bool InitializeDemoScene(D3DInstance* d3dInstance, HWND hwnd, DirectX::XMMATRIX screenCameraViewMatrix, QuadModel* quadModel, int shadowMapWidth, float shadowMapNearZ, float shadowMapFarZ);
	
	bool RenderScene(DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix, DirectX::XMFLOAT3 cameraPosition, float time);
	bool RenderDirectionalLightSceneDepth(float time);
	
	void UpdateMainImGuiWindow(int currentFPS, bool& isWireFrameRender, bool& showImGuiMenu);

	RenderTexture* GetDirectionalShadowMapRenderTexture() const { return m_DirectionalShadowMapRenderTexture; }
	
	void Shutdown();

private:
	D3DInstance* m_D3DInstance {};

	PBRShader* m_PBRShaderInstance {};
	DepthShader* m_DepthShaderInstance {};

	CubeMapObject* m_CubeMapObject {};

	DirectionalLight* m_DirectionalLight {};
	RenderTexture* m_DirectionalShadowMapRenderTexture {};

	// Point lights
	//LightClass* m_lights {};
	//int m_numLights {};

	std::vector<GameObject*> m_GameObjects {};

	std::unordered_map<std::string, std::vector<Texture*>> m_LoadedTextureResources {};
	std::unordered_map<std::string, Model*> m_LoadedModelResources {};

	bool LoadPBRTextureResource(const std::string& textureFileName);
	bool LoadModelResource(const std::string& modelFileName);

};
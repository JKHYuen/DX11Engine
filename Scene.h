#include <windows.h>

#include <unordered_map>
#include <string>
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

class Texture;
class Model;
class QuadModel;
class GameObject;
class D3DInstance;
class PBRShader;
class DepthShader;
class CubeMapObject;
class RenderTexture;
class DirectionalLight;
class Camera;
class Input;

class Scene {
public:
	Scene() {}
	Scene(const Texture& other) {}
	~Scene() {}

	bool InitializeDemoScene(D3DInstance* d3dInstance, HWND hwnd, XMMATRIX screenCameraViewMatrix, QuadModel* quadModel, int shadowMapWidth, float shadowMapNearZ, float shadowMapFarZ);
	
	bool RenderScene(XMMATRIX projectionMatrix, float time);
	bool RenderDirectionalLightSceneDepth(float time);
	
	void UpdateMainImGuiWindow(float currentFPS, bool& b_IsWireFrameRender, bool& b_ShowImGuiMenu, bool& b_ShowScreenFPS);

	void ProcessInput(Input* input, float deltaTime);

	RenderTexture* GetDirectionalShadowMapRenderTexture() const { return m_DirectionalShadowMapRenderTexture; }
	
	void Shutdown();

private:
	bool LoadPBRTextureResource(const std::string& textureFileName);
	bool LoadModelResource(const std::string& modelFileName);

private:
	D3DInstance* m_D3DInstance {};
	Camera* m_Camera {};

	PBRShader* m_PBRShaderInstance {};
	DepthShader* m_DepthShaderInstance {};

	CubeMapObject* m_CubeMapObject {};
	DirectionalLight* m_DirectionalLight {};
	RenderTexture* m_DirectionalShadowMapRenderTexture {};

	// Point lights
	//LightClass* m_lights {};
	//int m_numLights {};

	bool mb_AnimateDirectionalLight {};

	std::vector<GameObject*> m_GameObjects {};
	std::unordered_map<std::string, std::vector<Texture*>> m_LoadedTextureResources {};
	std::unordered_map<std::string, Model*> m_LoadedModelResources {};
};
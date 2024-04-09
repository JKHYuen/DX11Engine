#include <windows.h>

#include <unordered_map>
#include <string>
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

struct ID3D11ShaderResourceView;
struct ID3D11Device;
class Texture;
class Model;
class QuadModel;
class GameObject;
class D3DInstance;
class PBRShader;
class DepthShader;
class CubeMapObject;
class RenderTexture;
class TextureShader;
class DirectionalLight;
class Camera;
class Bloom;
class Input;

class Scene {
public:
	Scene() {}
	Scene(const Texture& other) {}
	~Scene() {}

	bool InitializeDemoScene(D3DInstance* d3dInstance, HWND hwnd, XMMATRIX screenCameraViewMatrix, QuadModel* quadModel, int shadowMapWidth, float shadowMapNearZ, float shadowMapFarZ, RenderTexture* screenRenderTexture, TextureShader* passThroughShaderInstance);
	void Shutdown();
	
	// Render objects to scene quad
	bool RenderScene(XMMATRIX projectionMatrix, float time);
	// Render final output with post processing
	bool RenderPostProcess(int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX orthoMatrix, ID3D11ShaderResourceView* textureSRV);

	bool RenderDirectionalLightSceneDepth(float time);
	void ProcessInput(Input* input, float deltaTime);
	
	// Note: Implemented here for convenience, probably doesn't belong in this class
	void UpdateMainImGuiWindow(float currentFPS, bool& b_IsWireFrameRender, bool& b_ShowImGuiMenu, bool& b_ShowScreenFPS, bool& b_QuitApp);

	RenderTexture* GetDirectionalShadowMapRenderTexture() const { return m_DirectionalShadowMapRenderTexture; }
	RenderTexture* GetDebugBloomOutput() const;

private:
	bool LoadPBRTextureResource(const std::string& textureFileName);
	bool LoadModelResource(const std::string& modelFileName);
	bool LoadPBRShader(ID3D11Device* device, HWND hwnd);

private:
	D3DInstance* m_D3DInstance {};
	Camera* m_Camera {};

	PBRShader* m_PBRShaderInstance {};
	DepthShader* m_DepthShaderInstance {};

	CubeMapObject* m_CubeMapObject {};
	DirectionalLight* m_DirectionalLight {};
	RenderTexture* m_DirectionalShadowMapRenderTexture {};

	// TODO: combine these
	TextureShader* m_PostProcessShader {};
	Bloom* m_BloomEffect {};

	// Point lights
	//LightClass* m_lights {};
	//int m_numLights {};

	bool mb_AnimateDirectionalLight {};

	std::vector<GameObject*> m_GameObjects {};
	std::unordered_map<std::string, std::vector<Texture*>> m_LoadedTextureResources {};
	std::unordered_map<std::string, Model*> m_LoadedModelResources {};
};
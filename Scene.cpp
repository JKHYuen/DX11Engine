#include "Scene.h"

#include "Application.h"
#include "Texture.h"
#include "Model.h"
#include "D3DInstance.h"
#include "PBRShader.h"
#include "DepthShader.h"
#include "GameObject.h"
#include "SkyBox.h"
#include "RenderTexture.h"
#include "TextureShader.h"
#include "DirectionalLight.h"
#include "QuadModel.h"
#include "Camera.h"
#include "Bloom.h"
#include "Input.h"

#include "imgui_impl_dx11.h"

#include <iostream>
#include <cmath>
#include <future>
#include <string_view>

// EXPERIMENTAL:
// Currently only multithreads functions that don't use device context
#define USE_MULTITHREAD_INITIALIZE 1

namespace {
	std::mutex s_DeviceContextMutex {};

	// Resource names (included in demo build) - used for IMGUI, could be built programmatically from files
	const std::vector<std::string> s_PBRMaterialFileNames {"bog", "brick", "dented", "dirt", "marble", "metal_grid", "rust", "stonewall", "waterworn", "windswept", "oak", "mud", "asphalt", "blocks"};
	const std::vector<std::string> s_ModelFileNames {"cube", "plane", "sphere"};
	const std::vector<std::string> s_HDRSkyboxFileNames {"rural_landscape_4k", "industrial_sunset_puresky_4k", "kloppenheim_03_4k", "schachen_forest_4k", "abandoned_tiled_room_4k"};

	constexpr int s_DefaultSkyboxIndex         = 0;
	constexpr int s_CubeFaceResolution         = 2048;
	constexpr int s_CubeMapMipLevels           = 9;
	constexpr int s_IrradianceMapResolution    = 32;
	constexpr int s_FullPrefilterMapResolution = 512;
	constexpr int s_PrecomputedBRDFResolution  = 512;

	/// Demo Scene starting values
	constexpr float s_StartingDirectionalLightDirX = 50.0f;
	constexpr float s_StartingDirectionalLightDirY = 230.0f;
	constexpr float s_StartingShadowBias = 0.001f;
	// sunlight color: 9.0f, 5.0f, 2.0f 
	//                 29.0f, 18.0f, 11.0f
	constexpr XMFLOAT3 s_StartingDirectionalLightColor = XMFLOAT3 {9.0f, 8.0f, 7.0f};
}

bool Scene::InitializeDemoScene(Application* appInstance, int shadowMapResolution, float shadowMapNearZ, float shadowMapFarZ
, float shadowDistance) {
	bool result;

	m_AppInstance = appInstance;
	m_D3DInstance = appInstance->GetD3DInstance();
	HWND hwnd = appInstance->GetHWND();

	/// Compile shaders
	m_DepthShaderInstance = new DepthShader();
	result = m_DepthShaderInstance->Initialize(m_D3DInstance->GetDevice(), hwnd);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the Depth shader object.", L"Error", MB_OK);
		return false;
	}

#if USE_MULTITHREAD_INITIALIZE == 0
	m_PBRShaderInstance = new PBRShader();
	result = m_PBRShaderInstance->Initialize(m_D3DInstance->GetDevice(), hwnd);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the PBR shader object.", L"Error", MB_OK);
		return false;
	}
#else
	auto fs1 = std::async(std::launch::async, &Scene::LoadPBRShader, this, m_D3DInstance->GetDevice(), hwnd);
#endif

	m_PostProcessShader = new TextureShader();
	result = m_PostProcessShader->Initialize(m_D3DInstance->GetDevice(), hwnd, true /*isPostProcessShader*/);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the texture shader object.", L"Error", MB_OK);
		return false;
	}

	// Bloom post process effect
	m_BloomEffect = new Bloom();
	result = m_BloomEffect->Initialize(m_D3DInstance->GetDevice(), m_D3DInstance->GetDeviceContext(), hwnd, appInstance->GetScreenRenderTexture(), m_PostProcessShader);
	if(!result) {
		MessageBox(hwnd, L"Could initialize bloom post processing.", L"Error", MB_OK);
		return false;
	}

	/// Create the 3D world camera
	m_WorldCamera = new Camera();
	m_WorldCamera->SetPosition(0.0f, 4.0f, -10.0f);

	/// Preload 3D resources
	/// Note: this should be programmatic in a real scene system
#if USE_MULTITHREAD_INITIALIZE == 0
	LoadModelResource("sphere");
	LoadModelResource("plane");
	LoadModelResource("cube");
#else
	// NOTE: very primitive way to do multithreading, need to multithread Texture::Initialize() for real performance gain (Majority of LoadPBRTextureResource() is locked as a critical section currently)
	auto fm1 = std::async(std::launch::async, &Scene::LoadModelResource, this, "sphere");
	auto fm2 = std::async(std::launch::async, &Scene::LoadModelResource, this, "plane");
	auto fm3 = std::async(std::launch::async, &Scene::LoadModelResource, this, "cube");
#endif

	result = LoadPBRTextureResource("rust");
	if(!result) { MessageBox(hwnd, L"Could initialize texture resource.", L"Error", MB_OK); return false; };
	result = LoadPBRTextureResource("stonewall");
	if(!result) { MessageBox(hwnd, L"Could initialize texture resource.", L"Error", MB_OK); return false; };
	result = LoadPBRTextureResource("metal_grid");
	if(!result) { MessageBox(hwnd, L"Could initialize texture resource.", L"Error", MB_OK); return false; };
	result = LoadPBRTextureResource("marble");
	if(!result) { MessageBox(hwnd, L"Could initialize texture resource.", L"Error", MB_OK); return false; };
	result = LoadPBRTextureResource("dirt");
	if(!result) { MessageBox(hwnd, L"Could initialize texture resource.", L"Error", MB_OK); return false; };
	result = LoadPBRTextureResource("bog");
	if(!result) { MessageBox(hwnd, L"Could initialize texture resource.", L"Error", MB_OK); return false; };

	/// Load default skybox cubemap
	m_CurrentCubemapIndex = s_DefaultSkyboxIndex;
	LoadCubemapResource(s_HDRSkyboxFileNames[m_CurrentCubemapIndex]);

	//struct GameObjectData {
	//	std::string modelName {};
	//	std::string materialName {};
	//	XMFLOAT3 position {};
	//	XMFLOAT3 scale {1.0f, 1.0f, 1.0f};
	//	float yRotSpeed {};
	//	float uvScale = 1.0f;
	//	float vertexDisplacementMapScale = 0.1f;
	//	float parallaxMapHeightScale = 0.0f;
	//	float minRoughness = 0;
	//	bool useParallaxShadow = true;
	//	int minParallaxLayers = 8;
	//	int maxParallaxLayers = 32;
	//	int tesselationFactor = 1;
	//};

	/// Load scene
	const std::vector<GameObject::GameObjectData> sceneObjects = {
		// Objects
		{"sphere", "rust",       {-3.0f, 4.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.0f, 0.0f},
		{"sphere", "stonewall",  {0.0f, 4.0f, 0.0f},  {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f, 0.0f},
		{"sphere", "metal_grid", {3.0f, 4.0f, 0.0f},  {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.0f, 0.0f},
		{"sphere", "marble",     {0.0f, 7.0f, 0.0f},  {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.0f, 0.0f},
		//{"cube",   "marble",     {0.0f, 11.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f, 0.0f},
		// Ground: should be last item, for IMGUI display			    					 
		{"plane",  "dirt",  {0.0f, 0.0f, 0.0f},  {5.0f, 1.0f, 5.0f}, 0.0f, 9.0f, 0.0f, 0.025f},
	};

	m_GameObjects.reserve(sceneObjects.size());
	for(size_t i = 0; i < sceneObjects.size(); i++) {
		m_GameObjects.push_back(new GameObject());
		m_GameObjects[i]->Initialize(m_PBRShaderInstance, m_DepthShaderInstance, m_LoadedTextureResources[sceneObjects[i].materialName], m_LoadedModelResources[sceneObjects[i].modelName], sceneObjects[i]);
	}

	/// Lighting
	// Create and initialize the shadow map texture
	m_DirectionalShadowMapRenderTexture = new RenderTexture();
	result = m_DirectionalShadowMapRenderTexture->Initialize(m_D3DInstance->GetDevice(), m_D3DInstance->GetDeviceContext(), shadowMapResolution, shadowMapResolution, shadowMapNearZ, shadowMapFarZ, DXGI_FORMAT_R32G32B32A32_FLOAT);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize shadow map texture.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the light object.
	m_DirectionalLight = new DirectionalLight();

	// Initialize Directional light
	m_DirectionalLight->SetColor(s_StartingDirectionalLightColor.x, s_StartingDirectionalLightColor.y, s_StartingDirectionalLightColor.z, 1.0f);
	m_DirectionalLight->GenerateOrthoMatrix(shadowDistance, shadowMapNearZ, shadowMapFarZ);
	m_DirectionalLight->SetDirection(XMConvertToRadians(s_StartingDirectionalLightDirX), XMConvertToRadians(s_StartingDirectionalLightDirY), 0.0f);
	m_DirectionalLight->SetShadowBias(s_StartingShadowBias);

	//// Set the number of lights we will use.
	//m_numLights = 4;

	//// Create and initialize the light objects array.
	//m_Lights = new LightClass[m_numLights];

	//// Manually set the color and position of each light.
	//m_Lights[0].SetDiffuseColor(1.0f, 0.0f, 0.0f, 1.0f);  // Red
	//m_Lights[0].SetPosition(-3.0f, 1.0f, 3.0f);

	//m_Lights[1].SetDiffuseColor(0.0f, 1.0f, 0.0f, 1.0f);  // Green
	//m_Lights[1].SetPosition(3.0f, 1.0f, 3.0f);

	//m_Lights[2].SetDiffuseColor(0.0f, 0.0f, 1.0f, 1.0f);  // Blue
	//m_Lights[2].SetPosition(-3.0f, 1.0f, -3.0f);

	//m_Lights[3].SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);  // White
	//m_Lights[3].SetPosition(3.0f, 1.0f, -3.0f);

#if USE_MULTITHREAD_INITIALIZE == 1
	if(!fs1.get()) {
		MessageBox(hwnd, L"Could not initialize the PBR shader object.", L"Error", MB_OK);
		return false;
	}
	if(!fm1.get() || !fm2.get() || !fm3.get()) {
		MessageBox(hwnd, L"Could not initialize 3D model.", L"Error", MB_OK);
		return false;
	}
#endif

	return true;
}

bool Scene::LoadPBRShader(ID3D11Device* device, HWND hwnd) {
	m_PBRShaderInstance = new PBRShader();
	return m_PBRShaderInstance->Initialize(m_D3DInstance->GetDevice(), hwnd);
}

bool Scene::RenderScene(XMMATRIX projectionMatrix, float time) {
	if(mb_AnimateDirectionalLight) {
		float animatedDir = std::sin(time * 0.5f) * 0.5f + 0.5f;
		static XMVECTOR quat1 = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(170.0f), XMConvertToRadians(30.0f), 0.0f);
		static XMVECTOR quat2 = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(8.0f), XMConvertToRadians(30.0f), 0.0f);
		m_DirectionalLight->SetQuaternionDirection(XMQuaternionSlerp(quat1, quat2, animatedDir));
	}

	m_WorldCamera->Update();
	m_WorldCamera->UpdateFrustum(projectionMatrix, m_AppInstance->GetScreenFar());

	Skybox* currentCubemap = m_LoadedCubemapResources[s_HDRSkyboxFileNames[m_CurrentCubemapIndex]];
	for(size_t i = 0; i < m_GameObjects.size(); i++) {
		if(!m_GameObjects[i]->Render(m_D3DInstance->GetDeviceContext(), projectionMatrix, m_DirectionalShadowMapRenderTexture->GetTextureSRV(), currentCubemap, m_DirectionalLight, m_WorldCamera, m_WorldCamera, time)) {
			return false;
		}
	}

	static ID3D11ShaderResourceView* nullSRV[10] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
	m_D3DInstance->GetDeviceContext()->PSSetShaderResources(0, 10, nullSRV);

	// Render skybox
	XMMATRIX viewMatrix {};
	m_WorldCamera->GetViewMatrix(viewMatrix);

	m_D3DInstance->SetToFrontCullRasterState();
	currentCubemap->Render(m_D3DInstance->GetDeviceContext(), viewMatrix, projectionMatrix, Skybox::kSkyBoxRender);
	m_D3DInstance->SetToBackCullRasterState();

	//m_ScreenRenderTexture->TurnZBufferOff();
	//m_ScreenRenderTexture->EnableAlphaBlending();

	// TODO: Transparent Objects

	//m_ScreenRenderTexture->TurnZBufferOn();
	//m_ScreenRenderTexture->DisableAlphaBlending();

	return true;
}

bool Scene::RenderSceneWithCullDebugCamera(XMMATRIX projectionMatrix, Camera* camera, float time) {
	Skybox* currentCubemap = m_LoadedCubemapResources[s_HDRSkyboxFileNames[m_CurrentCubemapIndex]];
	for(size_t i = 0; i < m_GameObjects.size(); i++) {
		if(!m_GameObjects[i]->Render(m_D3DInstance->GetDeviceContext(), projectionMatrix, m_DirectionalShadowMapRenderTexture->GetTextureSRV(), currentCubemap, m_DirectionalLight, camera, m_WorldCamera, time)) {
			return false;
		}
	}

	static ID3D11ShaderResourceView* nullSRV[10] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
	m_D3DInstance->GetDeviceContext()->PSSetShaderResources(0, 10, nullSRV);

	// Render skybox
	XMMATRIX viewMatrix {};
	camera->GetViewMatrix(viewMatrix);
	m_D3DInstance->SetToFrontCullRasterState();
	currentCubemap->Render(m_D3DInstance->GetDeviceContext(), viewMatrix, projectionMatrix, Skybox::kSkyBoxRender);
	m_D3DInstance->SetToBackCullRasterState();

	//m_ScreenRenderTexture->TurnZBufferOff();
	//m_ScreenRenderTexture->EnableAlphaBlending();

	// TODO: Transparent Objects

	//m_ScreenRenderTexture->TurnZBufferOn();
	//m_ScreenRenderTexture->DisableAlphaBlending();

	return true;
}



bool Scene::RenderPostProcess(int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX orthoMatrix, ID3D11ShaderResourceView* textureSRV) {
	// Note: bloom and post process shader (tonemapping) are separated to keep shaders more readable in this demo
	if(!m_BloomEffect->RenderEffect(m_D3DInstance, indexCount, worldMatrix, viewMatrix, orthoMatrix, textureSRV)) {
		return false;
	}

	m_D3DInstance->SetToBackBufferRenderTargetAndViewPort();
	m_D3DInstance->ClearBackBuffer(0.0f, 0.0f, 0.0f, 1.0f);

	if(!m_PostProcessShader->Render(m_D3DInstance->GetDeviceContext(), indexCount, worldMatrix, viewMatrix, orthoMatrix, m_BloomEffect->GetBloomOutput()->GetTextureSRV())) {
		return false;
	}

	// DEBUG: Bypass Bloom
	//if(!m_PostProcessShader->Render(m_D3DInstance->GetDeviceContext(), indexCount, worldMatrix, viewMatrix, orthoMatrix, textureSRV)) {
	//	return false;
	//}

	static ID3D11ShaderResourceView* nullSRV[2] = {nullptr, nullptr};
	m_D3DInstance->GetDeviceContext()->PSSetShaderResources(0, 2, nullSRV);

	return true;
}

RenderTexture* Scene::GetDebugBloomOutput() const { return m_BloomEffect->GetDebugBloomTexture(); }

bool Scene::RenderDirectionalLightSceneDepth(float time) {
	// Set the render target to be the render texture and clear it.
	m_DirectionalShadowMapRenderTexture->SetRenderTargetAndViewPort();
	m_DirectionalShadowMapRenderTexture->ClearRenderTarget(1.0f, 1.0f, 1.0f, 1.0f);

	m_D3DInstance->SetToFrontCullRasterState();

	for(size_t i = 0; i < m_GameObjects.size(); i++) {
		if(!m_GameObjects[i]->RenderToDepth(m_D3DInstance->GetDeviceContext(), m_DirectionalLight, time)) {
			return false;
		}
	}

	m_D3DInstance->SetToBackCullRasterState();

	return true;
}

bool Scene::LoadPBRTextureResource(const std::string& textureFileName) {
	if(m_LoadedTextureResources.find(textureFileName) == m_LoadedTextureResources.end()) {
		// Load models and materials to be used in scene
		std::string filePathPrefix {"./data/" + textureFileName + "/" + textureFileName};
		const std::vector<std::string> textureFileNames {
			filePathPrefix + "_albedo.tga",
			filePathPrefix + "_normal.tga",
			filePathPrefix + "_metallic.tga",
			filePathPrefix + "_roughness.tga",
			filePathPrefix + "_ao.tga",
			filePathPrefix + "_height.tga"
		};

		std::vector<Texture*> textureResources;
		textureResources.reserve(textureFileNames.size());
		for(size_t i = 0; i < textureFileNames.size(); i++) {
#if USE_MULTITHREAD_INITIALIZE == 1
			std::lock_guard<std::mutex> lock {s_DeviceContextMutex};
#endif
			textureResources.push_back(new Texture());
			if(!textureResources[i]->Initialize(
				m_D3DInstance->GetDevice(), m_D3DInstance->GetDeviceContext(), textureFileNames[i], DXGI_FORMAT_R8G8B8A8_UNORM)
				) {
				return false;
			}
		}

		m_LoadedTextureResources.emplace(textureFileName, textureResources);
	}

	return true;
}

bool Scene::LoadModelResource(const std::string& modelFileName) {
	if(m_LoadedModelResources.find(modelFileName) == m_LoadedModelResources.end()) {
		Model* pModel = new Model();
		if(!pModel->Initialize(
			m_D3DInstance->GetDevice(), "./data/" + modelFileName + ".txt")
			) {
			return false;
		}

		m_LoadedModelResources.emplace(modelFileName, pModel);
	}

	return true;
}

bool Scene::LoadCubemapResource(const std::string& hdrFileName) {
	if(m_LoadedCubemapResources.find(hdrFileName) == m_LoadedCubemapResources.end()) {
		XMMATRIX screenOrthoMatrix {}, screenCamViewMatrix {};
		m_D3DInstance->GetOrthoMatrix(screenOrthoMatrix);
		m_AppInstance->GetScreenDisplayCamera()->GetViewMatrix(screenCamViewMatrix);

		Skybox* pCubemap = new Skybox();
		bool result = pCubemap->Initialize(m_D3DInstance, m_AppInstance->GetHWND(), hdrFileName, s_CubeFaceResolution, s_CubeMapMipLevels, s_IrradianceMapResolution, s_FullPrefilterMapResolution, s_PrecomputedBRDFResolution, screenCamViewMatrix, screenOrthoMatrix, m_AppInstance->GetScreenDisplayQuadInstance());
		if(!result) {
			MessageBox(m_AppInstance->GetHWND(), L"Could not initialize cubemap.", L"Error", MB_OK);
			return false;
		}
		m_LoadedCubemapResources.emplace(hdrFileName, pCubemap);
	}

	return true;
}

void Scene::UpdateMainImGuiWindow(float currentFPS, bool& b_IsWireFrameRender, bool& b_ShowImGuiMenu, bool& b_ShowScreenFPS, bool& b_QuitAppFlag, bool& b_ShowDebugQuad1, bool& b_ShowDebugQuad2, bool& b_ShowDebugQuad3, bool& b_ToggleFullScreenFlag) {
	static auto ImGuiHelpMarker = [](const char* desc, bool b_IsSameLine = true, bool b_IsWarning = false) {
		if(b_IsSameLine) ImGui::SameLine();
		if(b_IsWarning)  ImGui::TextDisabled("(!)"); else ImGui::TextDisabled("(?)");
		if(ImGui::BeginItemTooltip()) {
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	};
	// Note: a little flimsy, assumes matName exists in s_PBRMaterialNames
	static auto FindPBRMaterialIndex = [](std::string_view matName) {
		for(int i = 0; i < s_PBRMaterialFileNames.size(); i++) {
			if(s_PBRMaterialFileNames[i] == matName) {
				return i;
			}
		}
		return -1;
	};
	static auto FindModelIndex = [](std::string_view modelName) {
		for(int i = 0; i < s_ModelFileNames.size(); i++) {
			if(s_ModelFileNames[i] == modelName) {
				return i;
			}
		}
		return -1;
	};

	static ImGuiSliderFlags kSliderFlags = ImGuiSliderFlags_AlwaysClamp;
	static ImGuiTableFlags kTableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders;

	bool b_isMenuActive = true;
	ImGui::Begin("Scene Menu", &b_isMenuActive, ImGuiWindowFlags_None);

	/// Exit button
	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
	if(ImGui::Button("EXIT APP")) {
		b_QuitAppFlag = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::TextDisabled("Controls");
	ImGuiHelpMarker("Click and drag to change numeric values.\nDouble click to type in values.\n\n   Esc: Quit App\n   Tab: Toggle this menu\n  WASD: movement\nLShift: move faster\n(Movement is disabled while this menu is open)");

	/// Misc
	static char gpuName[128] {};
	static int gpuMemory {-1};
	if(gpuMemory == -1) m_D3DInstance->GetVideoCardInfo(gpuName, gpuMemory);
	ImGui::TextDisabled("%s %d MB", gpuName, gpuMemory);
	ImGui::Text("FPS: %d", (int)currentFPS);
	ImGui::Spacing();

	if(ImGui::CollapsingHeader("Display")) {
		ImGui::Spacing();
		if(ImGui::Button("Toggle Fullscreen/Windowed")) {
			b_ToggleFullScreenFlag = true;
		}
		ImGuiHelpMarker("Keybind: F");
		ImGui::Spacing();
		ImGui::Checkbox("Show FPS on screen", &b_ShowScreenFPS); ImGuiHelpMarker("Keybind: F1");
		ImGui::SameLine(200);
		ImGui::Checkbox("Wireframe Render", &b_IsWireFrameRender); ImGuiHelpMarker("Keybind: F2");
		ImGui::Checkbox("Shadow Map View", &b_ShowDebugQuad1); ImGuiHelpMarker("Directional light shadow map.\nKeybind: Z");
		ImGui::SameLine(200);
		ImGui::Checkbox("Bloom Filter View", &b_ShowDebugQuad2); ImGuiHelpMarker("Bloom intensity not included.\nKeybind: X");
		ImGui::Checkbox("Culling Debug Camera", &b_ShowDebugQuad3); ImGuiHelpMarker("To debug object/triangle frustum culling on the main camera. Quite slow (full res) and does not include post processing.\nKeybing: C");
		ImGui::Spacing();
	}
	
	bool b_ShowSkyboxHeader = ImGui::CollapsingHeader("Skybox");
	ImGuiHelpMarker("Note: Environment maps for IBL are generated in run time, might be slow on first selection of skybox. Results are cached.", true, true);
	if(b_ShowSkyboxHeader) {
		if(ImGui::BeginTable("##skybox", 3, kTableFlags)) {
			for(int i = 0; i < s_HDRSkyboxFileNames.size(); i++) {
				ImGui::TableNextColumn();
				if(ImGui::Selectable(s_HDRSkyboxFileNames[i].c_str(), m_CurrentCubemapIndex == i)) {
					LoadCubemapResource(s_HDRSkyboxFileNames[i]);
					m_CurrentCubemapIndex = i;
				}
			}
			ImGui::EndTable();
		}
	}

	/// Directional Light
	if(ImGui::CollapsingHeader("Directional Light")) {
		ImGui::Spacing();
		ImGui::Checkbox("Enable animation", &mb_AnimateDirectionalLight);

		static float userLightDir[2] {s_StartingDirectionalLightDirX, s_StartingDirectionalLightDirY};
		if(mb_AnimateDirectionalLight) {
			ImGui::BeginDisabled();
			m_DirectionalLight->GetEulerAngles(userLightDir[0], userLightDir[1]);
		}
		ImGui::Spacing();

		ImGui::Text("Light Angle");
		if(ImGui::DragFloat2("[x, y]", userLightDir, 0.5f, -1000.0f, 1000.0f, "%.2f", kSliderFlags)) {
			userLightDir[0] = std::fmod(userLightDir[0], 360.0f);
			userLightDir[1] = std::fmod(userLightDir[1], 360.0f);
			m_DirectionalLight->SetDirection(XMConvertToRadians(userLightDir[0]), XMConvertToRadians(userLightDir[1]), 0.0f);
		}
		if(mb_AnimateDirectionalLight) {
			ImGui::EndDisabled();
		}

		static float userDirLightCol[3] = {s_StartingDirectionalLightColor.x, s_StartingDirectionalLightColor.y, s_StartingDirectionalLightColor.z};
		ImGui::Text("HDR Light Color");
		if(ImGui::DragFloat3("[r, g, b]", userDirLightCol, 0.1f, 0.0f, 1000.0f, "%.2f", kSliderFlags)) {
			m_DirectionalLight->SetColor(userDirLightCol[0], userDirLightCol[1], userDirLightCol[2], 1.0f);
		}
		ImGui::Spacing();

		static float userShadowBias = s_StartingShadowBias;
		if(ImGui::DragFloat("Shadow Bias", &userShadowBias, 0.001f, -1.0f, 1.0f, "%.3f", kSliderFlags)) {
			m_DirectionalLight->SetShadowBias(userShadowBias);
		}
	}

	/// Bloom Params
	bool b_ShowBloomHeader = ImGui::CollapsingHeader("Bloom");
	ImGuiHelpMarker("WARNING: Flickering may be caused by very bright pixels with near zero PBR material roughness. Use \"Minimum Roughness\" slider in Scene Objects header to alleviate the issue.", true, true);
	if(b_ShowBloomHeader) {
		ImGui::Spacing();
		static float userBloomIntensity = m_BloomEffect->GetIntensity();
		if(ImGui::DragFloat("Intensity", &userBloomIntensity, 0.01f, 0.0f, 1000.0f, "%.2f", kSliderFlags)) {
			m_BloomEffect->SetIntensity(userBloomIntensity);
		}

		static float userBloomThreshold = m_BloomEffect->GetThreshold();
		if(ImGui::DragFloat("Threshold", &userBloomThreshold, 0.01f, 0.0f, 10000.0f, "%.2f", kSliderFlags)) {
			m_BloomEffect->SetThreshold(userBloomThreshold);
		}

		static float userBloomSoftThreshold = m_BloomEffect->GetSoftThreshold();
		if(ImGui::DragFloat("Soft Threshold", &userBloomSoftThreshold, 0.001f, 0.0f, 1.0f, "%.2f", kSliderFlags)) {
			m_BloomEffect->SetSoftThreshold(userBloomSoftThreshold);
		}
		ImGui::Spacing();
	}

	/// Object Material Edit
	/// NOTE: implementation could be simplified with use of GameObject::GameObjectData struct
	bool b_ShowSceneObjectHeader = ImGui::CollapsingHeader("Scene Objects");
	ImGuiHelpMarker("Select a scene object to edit its object and material parameters.");
	if(b_ShowSceneObjectHeader) {
		// Note: Ground object is selected by default
		static int userSelectedGameObjectIndex = (int)m_GameObjects.size() - 1;
		// Note: Start true to intialize variables below
		static bool b_NewSceneObjectSelectedFlag = true;
		static bool b_UserObjectIsPlane {};

		static bool b_UserObjectEnabled = true;
		static int userSelectedMaterialIndex {};
		static int userSelectedModelIndex {};
		static float userPosition[3] {};
		static float userScale[3] {};
		static float userUVScale {};
		static float userDisplacementHeight {};
		static float userParallaxHeight {};
		static float userMinRoughness {};

		static bool b_UserParallaxShadowEnabled {};
		static int userMinParallaxLayers {};
		static int userMaxParallaxLayers {};
		static int userSelectedTessellationMode = 0;
		static float userUniformTessellationFactor {};
		static float userEdgeTessellationLength {};

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if(ImGui::TreeNode("Scene Object Select")) {
			if(ImGui::BeginTable("##GameObjects", 3, kTableFlags)) {
				for(int i = 0; i < m_GameObjects.size(); i++) {
					ImGui::TableNextColumn();

					char label[32];
					if(i == m_GameObjects.size() - 1) {
						sprintf_s(label, "Ground");
					}
					else {
						sprintf_s(label, "Object %d", i);
					}

					if(ImGui::Selectable(label, userSelectedGameObjectIndex == i)) {
						userSelectedGameObjectIndex = i;
						b_NewSceneObjectSelectedFlag = true;
					}
				}
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}

		ImGui::Spacing();
		ImGui::SeparatorText("Edit Parameters");
		ImGui::Spacing();

		GameObject* pSelectedGO = m_GameObjects[userSelectedGameObjectIndex];
		if(ImGui::Checkbox("Enable", &b_UserObjectEnabled)) {
			pSelectedGO->SetEnabled(b_UserObjectEnabled);
		}

		// Update parameters for new selected object if needed
		if(b_NewSceneObjectSelectedFlag) {
			b_UserObjectEnabled = pSelectedGO->GetEnabled();
			userSelectedMaterialIndex = FindPBRMaterialIndex(pSelectedGO->GetPBRMaterialName());
			userSelectedModelIndex = FindModelIndex(pSelectedGO->GetModelName());
			b_UserObjectIsPlane = pSelectedGO->GetModelName() == "plane";
			pSelectedGO->GetPosition(userPosition[0], userPosition[1], userPosition[2]);
			pSelectedGO->GetScale(userScale[0], userScale[1], userScale[2]);
			userUVScale = pSelectedGO->GetUVScale();
			userDisplacementHeight = pSelectedGO->GetDisplacementMapHeightScale();
			userParallaxHeight = pSelectedGO->GetParallaxMapHeightScale();
			userMinRoughness = pSelectedGO->GetMinRoughness();

			b_UserParallaxShadowEnabled = pSelectedGO->GetUseParallaxShadow();
			userMinParallaxLayers = pSelectedGO->GetMinParallaxLayers();
			userMaxParallaxLayers = pSelectedGO->GetMaxParallaxLayers();

			userSelectedTessellationMode = pSelectedGO->GetTessellationMode();
			userUniformTessellationFactor = pSelectedGO->GetUniformTesellationFactor();
			userEdgeTessellationLength = pSelectedGO->GetEdgeTessellationLength();

			b_NewSceneObjectSelectedFlag = false;
		}

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if(ImGui::TreeNode("Material Select")) {
			if(ImGui::BeginTable("##materials", 3, kTableFlags)) {
				for(int i = 0; i < s_PBRMaterialFileNames.size(); i++) {
					ImGui::TableNextColumn();
					if(ImGui::Selectable(s_PBRMaterialFileNames[i].c_str(), userSelectedMaterialIndex == i)) {
						userSelectedMaterialIndex = i;
						std::string matName = s_PBRMaterialFileNames[i % s_PBRMaterialFileNames.size()];
						LoadPBRTextureResource(matName);
						m_GameObjects[userSelectedGameObjectIndex]->SetPBRMaterialTextures(matName, m_LoadedTextureResources[matName]);
					}
				}
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}
		ImGui::Spacing();

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if(ImGui::TreeNode("Model Select")) {
			if(ImGui::BeginTable("##models", 3, kTableFlags)) {
				for(int i = 0; i < s_ModelFileNames.size(); i++) {
					ImGui::TableNextColumn();
					if(ImGui::Selectable(s_ModelFileNames[i].c_str(), userSelectedModelIndex == i)) {
						userSelectedModelIndex = i;
						std::string modelName = s_ModelFileNames[i % s_ModelFileNames.size()];
						LoadModelResource(modelName);
						m_GameObjects[userSelectedGameObjectIndex]->SetModel(modelName, m_LoadedModelResources[modelName]);
					}
				}
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}
		ImGui::Spacing();

		if(ImGui::DragFloat3("Position", userPosition, 0.01f, -1000.0f, 1000.0f, "%.2f", kSliderFlags)) {
			pSelectedGO->SetPosition(userPosition[0], userPosition[1], userPosition[2]);
		}

		if(ImGui::DragFloat3("Scale", userScale, 0.01f, -1000.0f, 1000.0f, "%.2f", kSliderFlags)) {
			pSelectedGO->SetScale(userScale[0], userScale[1], userScale[2]);
		}

		if(ImGui::DragFloat("UV Scale", &userUVScale, 0.01f, 1.0f, 1000.0f, "%.2f", kSliderFlags)) {
			pSelectedGO->SetUVScale(userUVScale);
		}
		ImGuiHelpMarker("Tip: Some materials look better with higher uv scales (e.g. asphalt, oak, and blocks).");

		if(ImGui::DragFloat("Minimum Roughness", &userMinRoughness, 0.001f, 0.0f, 1.0f, "%.3f", kSliderFlags)) {
			pSelectedGO->SetMinRoughness(userMinRoughness);
		}
		ImGuiHelpMarker("Minimum roughness allowed on PBR shader, this is a bandaid fix used to lower flickering from bloom.", true, true);

		ImGui::Spacing();
		ImGui::Text("Parallax Occlusion - READ ME:");
		ImGuiHelpMarker("Only supported if using \"plane\" model! Other models should set \"Parallax Height Scale\" to zero.\n\nHigher min / max parallax layers will quickly increase GPU load.\n\nShould not be used with vertex displacement.", true, true);
		if(ImGui::DragFloat("Parallax Height Scale", &userParallaxHeight, 0.001f, -100.0f, 100.0f, "%.3f", kSliderFlags)) {
			pSelectedGO->SetParallaxMapHeightScale(userParallaxHeight);
		}

		if(ImGui::DragInt("Parallax Min Layers", &userMinParallaxLayers, 1, 1, 256, "%d", kSliderFlags)) {
			pSelectedGO->SetMinParallaxLayers(userMinParallaxLayers);
		}

		if(ImGui::DragInt("Parallax Max Layers", &userMaxParallaxLayers, 1, 1, 256, "%d", kSliderFlags)) {
			pSelectedGO->SetMaxParallaxLayers(userMaxParallaxLayers);
		}

		if(ImGui::Checkbox("Enable Parallax Self Shadowing", &b_UserParallaxShadowEnabled)) {
			pSelectedGO->SetUseParallaxShadow(b_UserParallaxShadowEnabled);
		}

		ImGui::Spacing();
		ImGui::Text("Tessellation - READ ME:");
		ImGuiHelpMarker("Press \"F2\" to toggle wireframe view.\n\nVertex displacement should not be used with parallax occlusion, set \"Parallax Height Scale\" to zero to disable.\n\nTessellation enabled in shadow mapping by default (not optimized), may have flickering due to the simple PCF algorithm.\n\nStart by selecting a tessellation mode below:\nUNIFORM MODE - Set tessellation factor, this value will be used for all patch points.\n\nEDGE MODE - Set target edge length (in units based on screen resolution), edge lengths longer than this value will be tessellated. This mode is also distance based (farther objects will be less tessellated). The dynamic nature of this mode may make vertices displace noticeably as the camera is moved around. This is partially due to not having minimum and maximum values for distance scaling.", true, true);
		if(ImGui::BeginTable("##Tessellation Mode", 3, kTableFlags)) {
			for(int i = 0; i < PBRShader::TessellationMode::Num_TessellationModes; i++) {
				ImGui::TableNextColumn();
				if(ImGui::Selectable(PBRShader::s_TessellationModeNames[i].c_str(), userSelectedTessellationMode == i)) {
					userSelectedTessellationMode = i;
					pSelectedGO->SetTessellationMode(userSelectedTessellationMode);
					pSelectedGO->SetUniformTessellationFactor(userUniformTessellationFactor);
					pSelectedGO->SetEdgeTessellationLength(userEdgeTessellationLength);
				}
			}
			ImGui::EndTable();
		}
		ImGui::Spacing();

		// Uniform Tess
		if(userSelectedTessellationMode == 1) {
			if(ImGui::DragFloat("Tessellation Factor", &userUniformTessellationFactor, 0.1f, 1, 64, "%.1f", kSliderFlags)) {
				pSelectedGO->SetUniformTessellationFactor(userUniformTessellationFactor);
			}
		}
		// Edge Tess
		else if(userSelectedTessellationMode == 2) {
			if(ImGui::DragFloat("Edge Length", &userEdgeTessellationLength, 0.1f, 5, 1000, "%.1f", kSliderFlags)) {
				pSelectedGO->SetEdgeTessellationLength(userEdgeTessellationLength);
			}
		}

		if(ImGui::DragFloat("Displacement Height", &userDisplacementHeight, 0.01f, -100.0f, 100.0f, "%.3f", kSliderFlags)) {
			pSelectedGO->SetDisplacementMapHeightScale(userDisplacementHeight);
		}
		ImGuiHelpMarker("Vertex Displacement Scale:\nNeeds sufficient amount of vertices, use tessellation to create more triangles.", true, true);
	}

	ImGui::End();

	if(!b_isMenuActive) {
		b_ShowImGuiMenu = false;
		ShowCursor(b_ShowImGuiMenu);
	}
}

void Scene::ProcessInput(Input* input, float deltaTime) {
	static bool b_EnableFastMove = false;
	if(input->IsKeyUp(DIK_LSHIFT)) {
		b_EnableFastMove = false;
	}
	else if(input->IsKeyDown(DIK_LSHIFT)) {
		b_EnableFastMove = true;
	}
	
	// Mouse input
	static int currentMouseX;
	static int currentMouseY;
	input->GetMouseLocation(currentMouseX, currentMouseY);

	// Camera Rotation
	float mouseSensitivity = 10.0f * deltaTime;
	m_WorldCamera->SetRotation(m_WorldCamera->GetRotationX() + input->GetMouseAxisHorizontal() * mouseSensitivity, m_WorldCamera->GetRotationY() + input->GetMouseAxisVertical() * mouseSensitivity, m_WorldCamera->GetRotationZ());

	// Camera Translation
	XMFLOAT3 currLookAtDir = m_WorldCamera->GetLookAtDir();
	XMFLOAT3 currRightDir = m_WorldCamera->GetRightDir();
	XMVECTOR camMoveVector = XMVector3Normalize(XMVectorAdd(XMLoadFloat3(&currLookAtDir) * input->GetMoveAxisVertical(), XMLoadFloat3(&currRightDir) * input->GetMoveAxisHorizontal())) * deltaTime * (b_EnableFastMove ? 15.0f : 5.0f);

	m_WorldCamera->SetPosition(m_WorldCamera->GetPositionX() + XMVectorGetX(camMoveVector), m_WorldCamera->GetPositionY() + XMVectorGetY(camMoveVector), m_WorldCamera->GetPositionZ() + XMVectorGetZ(camMoveVector));
}

bool Scene::ResizeWindow(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int screenWidth, int screenHeight, float nearZ, float farZ) {
	bool result = m_BloomEffect->GenerateRenderTextures(device, deviceContext, screenWidth, screenHeight, nearZ, farZ);
	if(!result) return false;

	return true;
}

void Scene::Shutdown() {
	if(m_PBRShaderInstance) {
		m_PBRShaderInstance->Shutdown();
		delete m_PBRShaderInstance;
		m_PBRShaderInstance = nullptr;
	}

	if(m_DepthShaderInstance) {
		m_DepthShaderInstance->Shutdown();
		delete m_DepthShaderInstance;
		m_DepthShaderInstance = nullptr;
	}

	if(m_PostProcessShader) {
		m_PostProcessShader->Shutdown();
		delete m_PostProcessShader;
		m_PostProcessShader = nullptr;
	}

	if(m_BloomEffect) {
		m_BloomEffect->Shutdown();
		delete m_BloomEffect;
		m_BloomEffect = nullptr;
	}

	if(m_DirectionalShadowMapRenderTexture) {
		m_DirectionalShadowMapRenderTexture->Shutdown();
		delete m_DirectionalShadowMapRenderTexture;
		m_DirectionalShadowMapRenderTexture = nullptr;
	}

	if(m_DirectionalLight) {
		delete m_DirectionalLight;
		m_DirectionalLight = nullptr;
	}

	//if(m_lights) {
	//	delete[] m_lights;
	//	m_lights = nullptr;
	//}

	for(std::pair kvp : m_LoadedCubemapResources) {
		kvp.second->Shutdown();
		delete kvp.second;
		kvp.second = nullptr;
	}

	for(std::pair kvp : m_LoadedTextureResources) {
		for(size_t i = 0; i < kvp.second.size(); i++) {
			kvp.second[i]->Shutdown();
			delete kvp.second[i];
			kvp.second[i] = nullptr;
		}
	}

	for(std::pair kvp : m_LoadedModelResources) {
		kvp.second->Shutdown();
		delete kvp.second;
		kvp.second = nullptr;
	}

	for(size_t i = 0; i < m_GameObjects.size(); i++) {
		delete m_GameObjects[i];
		m_GameObjects[i] = nullptr;
	}

	if(m_WorldCamera) {
		delete m_WorldCamera;
		m_WorldCamera = nullptr;
	}
}



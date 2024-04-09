#include "Scene.h"

#include "Texture.h"
#include "Model.h"
#include "D3DInstance.h"
#include "PBRShader.h"
#include "DepthShader.h"
#include "GameObject.h"
#include "CubeMapObject.h"
#include "RenderTexture.h"
#include "TextureShader.h"
#include "DirectionalLight.h"
#include "QuadModel.h"
#include "Camera.h"
#include "Bloom.h"
#include "Input.h"

#include "imgui_impl_dx11.h"

#include <iostream>
#include <stdio.h>
#include <cmath>
#include <future>

// EXPERIMENTAL
#define USE_MULTITHREAD_INITIALIZE 1

/// Demo Scene starting values
static const float kStartingDirectionalLightDirX = 50.0f;
static const float kStartingDirectionalLightDirY = 230.0f;
// sunlight color: 9.0f, 5.0f, 2.0f 
//                 29.0f, 18.0f, 11.0f
static const XMFLOAT3 kStartingDirectionalLightColor = XMFLOAT3 {9.0f, 8.0f, 7.0f};

// PBR Texture names (files included in demo build)
static const std::vector<std::string> kPBRMaterialNames {"bog", "brick", "dented", "dirt", "marble", "metal_grid", "rust", "stonewall"};

static std::mutex s_DeviceContextMutex {};

bool Scene::LoadPBRShader(ID3D11Device* device, HWND hwnd) {
	m_PBRShaderInstance = new PBRShader();
	return m_PBRShaderInstance->Initialize(m_D3DInstance->GetDevice(), hwnd);
}

bool Scene::InitializeDemoScene(D3DInstance* d3dInstance, HWND hwnd, XMMATRIX screenCameraViewMatrix, QuadModel* quadModel, int shadowMapWidth, float shadowMapNearZ, float shadowMapFarZ, RenderTexture* screenRenderTexture, TextureShader* passThroughShaderInstance) {
	bool result;

	m_D3DInstance = d3dInstance;

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
	result = m_BloomEffect->Initialize(m_D3DInstance->GetDevice(), m_D3DInstance->GetDeviceContext(), hwnd, screenRenderTexture, m_PostProcessShader, passThroughShaderInstance);
	if(!result) {
		MessageBox(hwnd, L"Could initialize bloom post processing.", L"Error", MB_OK);
		return false;
	}

	/// Create the 3D world camera
	m_Camera = new Camera();
	m_Camera->SetPosition(0.0f, 4.0f, -10.0f);

	/// Load 3D Objects
	// TODO: Check for failure
	// TODO: Remove for release?
#if USE_MULTITHREAD_INITIALIZE == 0
	LoadModelResource("sphere");
	LoadModelResource("plane");
	LoadModelResource("cube");
#else
	auto f1 = std::async(std::launch::async, &Scene::LoadModelResource, this, "sphere");
	auto f2 = std::async(std::launch::async, &Scene::LoadModelResource, this, "plane");
	auto f3 = std::async(std::launch::async, &Scene::LoadModelResource, this, "cube");

	// NOTE: very primitive way to do multithreading, need to multithread Texture::Initialize() for real performance gain (Majority of LoadPBRTextureResource() is locked as a critical section currently)
	//auto f3 = std::async(std::launch::async, &Scene::LoadPBRTextureResource, this, "rust");
	//auto f4 = std::async(std::launch::async, &Scene::LoadPBRTextureResource, this, "stonewall");
	//auto f5 = std::async(std::launch::async, &Scene::LoadPBRTextureResource, this, "metal_grid");
	//auto f6 = std::async(std::launch::async, &Scene::LoadPBRTextureResource, this, "marble");
	//auto f7 = std::async(std::launch::async, &Scene::LoadPBRTextureResource, this, "dirt");
	//auto f8 = std::async(std::launch::async, &Scene::LoadPBRTextureResource, this, "bog");
#endif

	LoadPBRTextureResource("rust");
	LoadPBRTextureResource("stonewall");
	LoadPBRTextureResource("metal_grid");
	LoadPBRTextureResource("marble");
	LoadPBRTextureResource("dirt");
	LoadPBRTextureResource("bog");

	/// Load skybox cubemap
	XMMATRIX screenOrthoMatrix;
	d3dInstance->GetOrthoMatrix(screenOrthoMatrix);

	m_CubeMapObject = new CubeMapObject();
	// rural_landscape_4k | industrial_sunset_puresky_4k | kloppenheim_03_4k | schachen_forest_4k | abandoned_tiled_room_4k
	result = m_CubeMapObject->Initialize(m_D3DInstance, hwnd, "industrial_sunset_puresky_4k", 2048, 9, 32, 512, 512, screenCameraViewMatrix, screenOrthoMatrix, quadModel);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize cubemap.", L"Error", MB_OK);
		return false;
	}

	// Temp scene system
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

	const std::vector<GameObjectData> sceneObjects = {
		// Objects
		{"sphere", "rust",       {-3.0f, 4.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f, 0.0f},
		{"sphere", "stonewall",  {0.0f, 4.0f, 0.0f},  {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f, 0.0f},
		{"sphere", "metal_grid", {3.0f, 4.0f, 0.0f},  {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f, 0.0f},
		{"sphere", "marble",     {0.0f, 7.0f, 0.0f},  {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f, 0.0f},
		//{"cube",   "marble",     {0.0f, 11.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f, 0.0f},
		// Floor			    					 
		{"plane",  "dirt",  {0.0f, 0.0f, 0.0f},  {5.0f, 1.0f, 5.0f}, 0.0f, 9.0f, 0.0f, 0.025f},
	};

	m_GameObjects.reserve(sceneObjects.size());
	for(size_t i = 0; i < sceneObjects.size(); i++) {
		m_GameObjects.push_back(new GameObject());
		if(!m_GameObjects[i]->Initialize(m_PBRShaderInstance, m_DepthShaderInstance, m_LoadedTextureResources[sceneObjects[i].materialName], m_LoadedModelResources[sceneObjects[i].modelName])) {
			MessageBox(hwnd, L"Could not initialize PBR object.", L"Error", MB_OK);
			return false;
		}

		m_GameObjects[i]->SetPosition(sceneObjects[i].position);
		m_GameObjects[i]->SetScale(sceneObjects[i].scale);
		m_GameObjects[i]->SetYRotationSpeed(sceneObjects[i].yRotSpeed);
		m_GameObjects[i]->SetUVScale(sceneObjects[i].uvScale);
		m_GameObjects[i]->SetDisplacementMapHeightScale(sceneObjects[i].vertexDisplacementMapScale);
		m_GameObjects[i]->SetParallaxMapHeightScale(sceneObjects[i].parallaxMapHeightScale);
	}

	/// Lighting
	// Create and initialize the shadow map texture
	m_DirectionalShadowMapRenderTexture = new RenderTexture();
	result = m_DirectionalShadowMapRenderTexture->Initialize(m_D3DInstance->GetDevice(), m_D3DInstance->GetDeviceContext(), shadowMapWidth, shadowMapWidth, shadowMapNearZ, shadowMapFarZ, DXGI_FORMAT_R32G32B32A32_FLOAT);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize shadow map texture.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the light object.
	m_DirectionalLight = new DirectionalLight();

	// Initialize Directional light
	m_DirectionalLight->SetColor(kStartingDirectionalLightColor.x, kStartingDirectionalLightColor.y, kStartingDirectionalLightColor.z, 1.0f);
	m_DirectionalLight->GenerateOrthoMatrix(40.0f, shadowMapNearZ, shadowMapFarZ);
	m_DirectionalLight->SetDirection(XMConvertToRadians(kStartingDirectionalLightDirX), XMConvertToRadians(kStartingDirectionalLightDirY), 0.0f);

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
#endif

	return true;
}

bool Scene::RenderScene(XMMATRIX projectionMatrix, float time) {
	m_Camera->Update();

	XMMATRIX viewMatrix {}, orthoMatrix {};
	m_Camera->GetViewMatrix(viewMatrix);

	if(mb_AnimateDirectionalLight) {
		float animatedDir = std::sin(time * 0.5f) * 0.5f + 0.5f;
		static XMVECTOR quat1 = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(170.0f), XMConvertToRadians(30.0f), 0.0f);
		static XMVECTOR quat2 = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(8.0f), XMConvertToRadians(30.0f), 0.0f);
		m_DirectionalLight->SetQuaternionDirection(XMQuaternionSlerp(quat1, quat2, animatedDir));

	}

	for(size_t i = 0; i < m_GameObjects.size(); i++) {
		if(!m_GameObjects[i]->Render(m_D3DInstance->GetDeviceContext(), viewMatrix, projectionMatrix, m_DirectionalShadowMapRenderTexture->GetTextureSRV(), m_CubeMapObject->GetIrradianceMapSRV(), m_CubeMapObject->GetPrefilteredMapSRV(), m_CubeMapObject->GetPrecomputedBRDFSRV(), m_DirectionalLight, m_Camera->GetPosition(), time)) {
			return false;
		}
	}

	static ID3D11ShaderResourceView* nullSRV[10] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
	m_D3DInstance->GetDeviceContext()->PSSetShaderResources(0, 10, nullSRV);

	// Render skybox
	m_D3DInstance->SetToFrontCullRasterState();
	m_CubeMapObject->Render(m_D3DInstance->GetDeviceContext(), viewMatrix, projectionMatrix, CubeMapObject::kSkyBoxRender);
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
		std::string filePathPrefix {"../DX11Engine/data/" + textureFileName + "/" + textureFileName};
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
		Model* pTempModel = new Model();
		if(!pTempModel->Initialize(
			m_D3DInstance->GetDevice(), "../DX11Engine/data/" + modelFileName + ".txt")
			) {
			return false;
		}

		m_LoadedModelResources.emplace(modelFileName, pTempModel);
	}

	return true;
}

void Scene::UpdateMainImGuiWindow(float currentFPS, bool& b_IsWireFrameRender, bool& b_ShowImGuiMenu, bool& b_ShowScreenFPS, bool& b_QuitApp) {
	auto ImGuiHelpMarker = [](const char* desc) {
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if(ImGui::BeginItemTooltip()) {
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	};

	static ImGuiSliderFlags sliderFlags = ImGuiSliderFlags_AlwaysClamp;

	bool b_isMenuActive = true;
	ImGui::Begin("Scene Menu", &b_isMenuActive, ImGuiWindowFlags_None);

	/// Exit button
	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
	if(ImGui::Button("EXIT APP")) {
		b_QuitApp = true;
	}
	ImGui::PopStyleColor(3);
	ImGuiHelpMarker("Press this button or \"Esc\" to quit app.");
	ImGui::Spacing();

	/// Misc
	static char gpuName[128] {};
	static int gpuMemory {-1};
	if(gpuMemory == -1) {
		m_D3DInstance->GetVideoCardInfo(gpuName, gpuMemory);
	}
	ImGui::Text("%s %d MB", gpuName, gpuMemory);
	ImGui::Spacing();

	ImGui::Text("FPS: %d", (int)currentFPS);
	ImGui::Checkbox("Show FPS on screen", &b_ShowScreenFPS);
	ImGui::Spacing();

	ImGui::Checkbox("Enable Wireframe Render", &b_IsWireFrameRender);
	ImGui::Spacing();

	/// Directional Light
	ImGui::SeparatorText("Directional Light");
	ImGui::Checkbox("Enable animation", &mb_AnimateDirectionalLight);

	static float userLightDir[2] {kStartingDirectionalLightDirX, kStartingDirectionalLightDirY};
	if(mb_AnimateDirectionalLight) {
		ImGui::BeginDisabled();
		m_DirectionalLight->GetEulerAngles(userLightDir[0], userLightDir[1]);
	}
	ImGui::Text("Light Angle");
	if(ImGui::DragFloat2("[x, y]", userLightDir, 0.5f, -1000.0f, 1000.0f, "%.2f", sliderFlags)) {
		userLightDir[0] = std::fmod(userLightDir[0], 360.0f);
		userLightDir[1] = std::fmod(userLightDir[1], 360.0f);
		m_DirectionalLight->SetDirection(XMConvertToRadians(userLightDir[0]), XMConvertToRadians(userLightDir[1]), 0.0f);
	}
	if(mb_AnimateDirectionalLight) {
		ImGui::EndDisabled();
	}

	static float userDirLightCol[3] = {kStartingDirectionalLightColor.x, kStartingDirectionalLightColor.y, kStartingDirectionalLightColor.z};
	ImGui::Text("HDR Light Color");
	if(ImGui::DragFloat3("[r, g, b]", userDirLightCol, 0.1f, 0.0f, 1000.0f, "%.2f", sliderFlags)) {
		m_DirectionalLight->SetColor(userDirLightCol[0], userDirLightCol[1], userDirLightCol[2], 1.0f);
	}
	ImGui::Spacing();

	/// Bloom Params
	ImGui::SeparatorText("Bloom");
	ImGui::Text("Intensity");
	static float userBloomIntensity = m_BloomEffect->GetIntensity();
	if(ImGui::DragFloat("##Intensity", &userBloomIntensity, 0.01f, 0.0f, 1000.0f, "%.2f", sliderFlags)) {
		m_BloomEffect->SetIntensity(userBloomIntensity);
	}

	ImGui::Text("Threshold");
	static float userBloomThreshold = m_BloomEffect->GetThreshold();
	if(ImGui::DragFloat("##Thresh", &userBloomThreshold, 0.01f, 0.0f, 10000.0f, "%.2f", sliderFlags)) {
		m_BloomEffect->SetThreshold(userBloomThreshold);
	}

	ImGui::Text("Soft Threshold");
	static float userBloomSoftThreshold = m_BloomEffect->GetSoftThreshold();
	if(ImGui::DragFloat("##SoftThresh", &userBloomSoftThreshold, 0.001f, 0.0f, 1.0f, "%.2f", sliderFlags)) {
		m_BloomEffect->SetSoftThreshold(userBloomSoftThreshold);
	}
	ImGui::Spacing();

	/// Material Edit
	ImGui::SeparatorText("Ground");

	ImGui::Text("Material Select");
	static int selected = 3;
	if(ImGui::BeginTable("##materials", 3, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
		for(int i = 0; i < kPBRMaterialNames.size(); i++) {
			ImGui::TableNextColumn();
			if(ImGui::Selectable(kPBRMaterialNames[i].c_str(), selected == i)) {
				LoadPBRTextureResource(kPBRMaterialNames[i % kPBRMaterialNames.size()]);
				m_GameObjects[m_GameObjects.size() - 1]->SetMaterialTextures(m_LoadedTextureResources[kPBRMaterialNames[i % kPBRMaterialNames.size()]]);
			}
		}
		ImGui::EndTable();
	}

	static float userParallaxHeight = m_GameObjects[m_GameObjects.size() - 1]->GetParallaxMapHeightScale();
	if(ImGui::DragFloat("[x, y]", &userParallaxHeight, 0.001f, -1000.0f, 1000.0f, "%.3f", sliderFlags)) {
		m_GameObjects[m_GameObjects.size() - 1]->SetParallaxMapHeightScale(userParallaxHeight);
	}
	ImGui::Spacing();


	//static bool b1 {};
	//if(ImGui::Checkbox("Switch Ground Material", &b1)) {
	//	if(b1) {
	//		LoadPBRTextureResource("dented");
	//		m_GameObjects[m_GameObjects.size() - 1]->SetMaterialTextures(m_LoadedTextureResources["dented"]);
	//	}
	//	else {
	//		LoadPBRTextureResource("dirt");
	//		m_GameObjects[m_GameObjects.size() - 1]->SetMaterialTextures(m_LoadedTextureResources["dirt"]);
	//	}
	//}

	ImGui::End();

	if(!b_isMenuActive) {
		b_ShowImGuiMenu = false;
		ShowCursor(b_ShowImGuiMenu);
	}
}


void Scene::ProcessInput(Input* input, float deltaTime) {
	static bool b_EnableFastMove = false;
	if(input->IsLeftShiftKeyUp()) {
		b_EnableFastMove = false;
	}
	else if(input->IsLeftShiftKeyDown()) {
		b_EnableFastMove = true;
	}
	
	// Mouse input
	static int currentMouseX;
	static int currentMouseY;
	input->GetMouseLocation(currentMouseX, currentMouseY);

	// Camera Rotation
	float mouseSensitivity = 10.0f * deltaTime;
	m_Camera->SetRotation(m_Camera->GetRotationX() + input->GetMouseAxisHorizontal() * mouseSensitivity, m_Camera->GetRotationY() + input->GetMouseAxisVertical() * mouseSensitivity, m_Camera->GetRotationZ());

	// Camera Translation
	XMFLOAT3 currLookAtDir = m_Camera->GetLookAtDir();
	XMFLOAT3 currRightDir = m_Camera->GetRightDir();
	XMVECTOR camMoveVector = XMVector3Normalize(XMVectorAdd(XMLoadFloat3(&currLookAtDir) * input->GetMoveAxisVertical(), XMLoadFloat3(&currRightDir) * input->GetMoveAxisHorizontal())) * deltaTime * (b_EnableFastMove ? 15.0f : 5.0f);

	m_Camera->SetPosition(m_Camera->GetPositionX() + XMVectorGetX(camMoveVector), m_Camera->GetPositionY() + XMVectorGetY(camMoveVector), m_Camera->GetPositionZ() + XMVectorGetZ(camMoveVector));
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

	if(m_CubeMapObject) {
		m_CubeMapObject->Shutdown();
		delete m_CubeMapObject;
		m_CubeMapObject = nullptr;
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

	if(m_Camera) {
		delete m_Camera;
		m_Camera = nullptr;
	}
}



#include "Scene.h"

#include "Texture.h"
#include "Model.h"
#include "D3DInstance.h"
#include "PBRShader.h"
#include "DepthShader.h"
#include "GameObject.h"
#include "CubeMapObject.h"
#include "RenderTexture.h"
#include "DirectionalLight.h"
#include "QuadModel.h"
#include "Camera.h"
#include "Input.h"

#include "imgui_impl_dx11.h"

#include <iostream>
#include <cmath>
#include <future>

/// Demo Scene starting values
static const float kStartingDirectionalLightDirX = 50.0f;
static const float kStartingDirectionalLightDirY = 30.0f;
// sunlight color: 9.0f, 5.0f, 2.0f 
//                 29.0f, 18.0f, 11.0f
static const XMFLOAT3 kStartingDirectionalLightColor = XMFLOAT3 {9.0f, 8.0f, 7.0f};

static std::mutex s_DeviceContextMutex {};

bool Scene::InitializeDemoScene(D3DInstance* d3dInstance, HWND hwnd, XMMATRIX screenCameraViewMatrix, QuadModel* quadModel, int shadowMapWidth, float shadowMapNearZ, float shadowMapFarZ) {
	bool result;

	m_D3DInstance = d3dInstance;

	/// Compile shaders
	m_DepthShaderInstance = new DepthShader();
	result = m_DepthShaderInstance->Initialize(m_D3DInstance->GetDevice(), hwnd);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the Depth shader object.", L"Error", MB_OK);
		return false;
	}

	m_PBRShaderInstance = new PBRShader();
	result = m_PBRShaderInstance->Initialize(m_D3DInstance->GetDevice(), hwnd);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the PBR shader object.", L"Error", MB_OK);
		return false;
	}

	/// Create the 3D world camera
	m_Camera = new Camera();
	m_Camera->SetPosition(0.0f, 4.0f, -10.0f);

	/// Load 3D Objects
	// TODO: Check for failure
	// TODO: Remove for release?
#define USE_MULTITHREAD 1
#if USE_MULTITHREAD == 0
	LoadModelResource("sphere");
	LoadModelResource("plane");

	LoadPBRTextureResource("rust");
	LoadPBRTextureResource("stonewall");
	LoadPBRTextureResource("metal_grid");
	LoadPBRTextureResource("marble");
	LoadPBRTextureResource("dirt");
	LoadPBRTextureResource("bog");
#else
	auto f7 = std::async(std::launch::async, &Scene::LoadModelResource, this, "sphere");
	auto f8 = std::async(std::launch::async, &Scene::LoadModelResource, this, "plane");

	auto f1 = std::async(std::launch::async, &Scene::LoadPBRTextureResource, this, "rust");
	auto f2 = std::async(std::launch::async, &Scene::LoadPBRTextureResource, this, "stonewall");
	auto f3 = std::async(std::launch::async, &Scene::LoadPBRTextureResource, this, "metal_grid");
	auto f4 = std::async(std::launch::async, &Scene::LoadPBRTextureResource, this, "marble");
	auto f5 = std::async(std::launch::async, &Scene::LoadPBRTextureResource, this, "dirt");
	auto f6 = std::async(std::launch::async, &Scene::LoadPBRTextureResource, this, "bog");
#endif

	/// Load skybox cubemap
	XMMATRIX screenOrthoMatrix;
	d3dInstance->GetOrthoMatrix(screenOrthoMatrix);

	m_CubeMapObject = new CubeMapObject();
	// rural_landscape_4k | industrial_sunset_puresky_4k | kloppenheim_03_4k | schachen_forest_4k | abandoned_tiled_room_4k
	result = m_CubeMapObject->Initialize(m_D3DInstance, hwnd, "rural_landscape_4k", 2048, 9, 32, 512, 512, screenCameraViewMatrix, screenOrthoMatrix, quadModel);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize cubemap.", L"Error", MB_OK);
		return false;
	}

#if USE_MULTITHREAD == 1
	f1.wait();
	f2.wait();
	f3.wait();
	f4.wait();
	f5.wait();
	f6.wait();
	f7.wait();
	f8.wait();
#endif


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
		m_GameObjects.emplace_back(new GameObject());
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

	return true;
}

bool Scene::RenderScene(XMMATRIX projectionMatrix, float time) {
	m_Camera->Update();

	XMMATRIX viewMatrix {};
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

	static ID3D11ShaderResourceView* nullSRV[6] = {nullptr, nullptr,nullptr, nullptr, nullptr, nullptr};
	m_D3DInstance->GetDeviceContext()->PSSetShaderResources(0, 6, nullSRV);

	// TODO: figure out more elegant solution for raster states
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

bool Scene::RenderDirectionalLightSceneDepth(float time) {
	// Set the render target to be the render texture and clear it.
	m_DirectionalShadowMapRenderTexture->SetRenderTarget();
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

		static std::vector<std::future<bool>> s_F(textureFileNames.size());

		std::vector<Texture*> textureResources;
		textureResources.reserve(textureFileNames.size());
		for(size_t i = 0; i < textureFileNames.size(); i++) {
#if USE_MULTITHREAD == 1
			//std::lock_guard<std::mutex> lock {s_DeviceContextMutex};
#endif
			textureResources.emplace_back(new Texture());
			if(!textureResources[i]->Initialize(
				m_D3DInstance->GetDevice(), m_D3DInstance->GetDeviceContext(), textureFileNames[i], DXGI_FORMAT_R8G8B8A8_UNORM)
				) {
				return false;
			}
		}

		m_LoadedTextureResources.emplace(textureFileName, textureResources);
	}
	else {
		std::cout << "Warning: Attempting to load existing PBR material textures: \"" << textureFileName << "\"." << std::endl;
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
	else {
		std::cout << "Warning: Attempting to load existing model: \"" << modelFileName << "\"." << std::endl;
	}

	return true;
}

void Scene::UpdateMainImGuiWindow(float currentFPS, bool& b_IsWireFrameRender, bool& b_ShowImGuiMenu, bool& b_ShowScreenFPS) {
	static ImGuiSliderFlags sliderFlags = ImGuiSliderFlags_AlwaysClamp;

	bool b_isMenuActive = true;
	ImGui::Begin("Scene Menu", &b_isMenuActive, ImGuiWindowFlags_None);

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

	/// Material Edit
	ImGui::SeparatorText("Ground Material");
	static float userParallaxHeight = m_GameObjects[m_GameObjects.size() - 1]->GetParallaxMapHeightScale();
	if(ImGui::DragFloat("[x, y]", &userParallaxHeight, 0.001f, -1000.0f, 1000.0f, "%.3f", sliderFlags)) {
		m_GameObjects[m_GameObjects.size() - 1]->SetParallaxMapHeightScale(userParallaxHeight);
	}

	static bool b1 {};
	if(ImGui::Checkbox("Switch Ground Material", &b1)) {
		m_GameObjects[m_GameObjects.size() - 1]->SetMaterialTextures(b1 ? m_LoadedTextureResources["bog"] : m_LoadedTextureResources["dirt"]);
	}

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



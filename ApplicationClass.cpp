﻿#include "ApplicationClass.h"

#include "D3DClass.h"
#include "InputClass.h"
#include "CameraClass.h"
#include "TextureShaderClass.h"
#include "FontShaderClass.h"

#include "RenderTextureClass.h"
#include "QuadModel.h"

#include "FontClass.h"
#include "TextClass.h"
#include "DirectionalLightClass.h"
#include "SpriteClass.h"
#include "TimerClass.h"
#include "FpsClass.h"

#include "GameObject.h"
#include "PBRShaderClass.h"
#include "DepthShaderClass.h"
#include "CubeMapObject.h"

#include "imgui_impl_dx11.h"

#include <iostream>

ApplicationClass::ApplicationClass() {}
ApplicationClass::ApplicationClass(const ApplicationClass& other) {}
ApplicationClass::~ApplicationClass() {}

static float startingDirectionalLightDirX = 50.0f;
static float startingDirectionalLightDirY = 30.0f;

bool ApplicationClass::Initialize(bool isFullScreen, int screenWidth, int screenHeight, HWND hwnd) {
	bool result;
	char fpsString[32];
	//char mouseString1[32], mouseString2[32], mouseString3[32];

	// Note: currently only used for debug quad position calc
	m_ScreenWidth = screenWidth;
	m_ScreenHeight = screenHeight;

	m_StartTime = std::chrono::steady_clock::now();

	// Create and initialize the Direct3D object
	m_Direct3D = new D3DClass();
	result = m_Direct3D->Initialize(screenWidth, screenHeight, gVsyncEnabled, hwnd, isFullScreen, gScreenDepth, gScreenNear);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	// Create the 3D world camera
	m_Camera = new CameraClass();
	m_Camera->SetPosition(0.0f, 4.0f, -10.0f);

	// Create camera to view screen render texture
	m_ScreenDisplayCamera = new CameraClass();
	// Display Camera is orthographic
	m_ScreenDisplayCamera->SetPosition(0.0f, 0.0f, -1.0f);
	// Initialize stationary camera to view full screen quads
	m_ScreenDisplayCamera->Render();
	
	/// Initialize shared shader instances
	m_DepthShaderInstance = new DepthShaderClass();
	result = m_DepthShaderInstance->Initialize(m_Direct3D->GetDevice(), hwnd);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the Depth shader object.", L"Error", MB_OK);
		return false;
	}

	m_PBRShaderInstance = new PBRShaderClass();
	result = m_PBRShaderInstance->Initialize(m_Direct3D->GetDevice(), hwnd);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the PBR shader object.", L"Error", MB_OK);
		return false;
	}

	///////////////////////////////
	// Screen Render Texture //////
	///////////////////////////////
	// Create and initialize the screen render texture
	m_ScreenRenderTexture = new RenderTextureClass();
	result = m_ScreenRenderTexture->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, gScreenNear, gScreenDepth, DXGI_FORMAT_R32G32B32A32_FLOAT);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize screen render texture.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the screen display quad
	m_ScreenDisplayPlane = new QuadModel();
	result = m_ScreenDisplayPlane->Initialize(m_Direct3D->GetDevice(), screenWidth / 2.0f, screenHeight / 2.0f);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize screen quad.", L"Error", MB_OK);
		return false;
	}

	///////////////////////////////
	// 2D Rendering //
	///////////////////////////////
	m_PostProcessShader = new TextureShaderClass();
	result = m_PostProcessShader->Initialize(m_Direct3D->GetDevice(), hwnd, true);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the texture shader object.", L"Error", MB_OK);
		return false;
	}

	// Shader for depth debug quad
	m_DebugDepthShader = new TextureShaderClass();
	result = m_DebugDepthShader->Initialize(m_Direct3D->GetDevice(), hwnd, false);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the depth shader object.", L"Error", MB_OK);
		return false;
	}

	// Set the sprite info file we will be using.
	//strcpy_s(spriteFilename, "../DX11Engine/data/sprite_data_01.txt");

	// Create and initialize the sprite object.
	//m_sprite = new SpriteClass();

	//result = m_sprite->Initialize(m_direct3D->GetDevice(), m_direct3D->GetDeviceContext(), screenWidth, screenHeight, spriteFilename, 0, 0);
	//if(!result) {
	//	return false;
	//}

	///////////////////////////////
	// 3D Objects //
	///////////////////////////////

	// Load cubemap
	XMMATRIX screenCameraViewMatrix{};
	m_ScreenDisplayCamera->GetViewMatrix(screenCameraViewMatrix);
	XMMATRIX screenOrthoMatrix {};
	m_Direct3D->GetOrthoMatrix(screenOrthoMatrix);

	m_CubeMapObject = new CubeMapObject();
	// rural_landscape_4k | industrial_sunset_puresky_4k | kloppenheim_03_4k | schachen_forest_4k | abandoned_tiled_room_4k
	result = m_CubeMapObject->Initialize(m_Direct3D, hwnd, "rural_landscape_4k", 2048, 9, 32, 512, 512, screenCameraViewMatrix, screenOrthoMatrix, m_ScreenDisplayPlane);
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

	const std::vector<GameObjectData> sampleSceneObjects = {
		// Objects
		{"sphere", "rust",       {-3.0f, 4.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f, 0.0f},
		{"sphere", "stonewall",  {0.0f, 4.0f, 0.0f},  {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f, 0.0f},
		{"sphere", "metal_grid", {3.0f, 4.0f, 0.0f},  {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f, 0.0f},
		{"sphere", "marble",     {0.0f, 7.0f, 0.0f},  {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f, 0.0f},
		//{"cube",   "marble",     {0.0f, 11.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f, 0.0f},
		// Floor			    					 
		{"plane",  "dirt",  {0.0f, 0.0f, 0.0f},  {5.0f, 1.0f, 5.0f}, 0.0f, 9.0f, 0.0f, 0.025f},
	};

	m_GameObjects.reserve(sampleSceneObjects.size());
	for(size_t i = 0; i < sampleSceneObjects.size(); i++) {
		m_GameObjects.emplace_back();
		if(!m_GameObjects[i].Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), hwnd, sampleSceneObjects[i].modelName, sampleSceneObjects[i].materialName, m_PBRShaderInstance, m_DepthShaderInstance)) {
			MessageBox(hwnd, L"Could not initialize PBR object.", L"Error", MB_OK);
			return false;
		}

		m_GameObjects[i].SetPosition(sampleSceneObjects[i].position);
		m_GameObjects[i].SetScale(sampleSceneObjects[i].scale);
		m_GameObjects[i].SetYRotationSpeed(sampleSceneObjects[i].yRotSpeed);
		m_GameObjects[i].SetUVScale(sampleSceneObjects[i].uvScale);
		m_GameObjects[i].SetDisplacementMapHeightScale(sampleSceneObjects[i].vertexDisplacementMapScale);
		m_GameObjects[i].SetParallaxMapHeightScale(sampleSceneObjects[i].parallaxMapHeightScale);
	}

	/// Lighting
	// Create and initialize the shadow map texture
	m_ShadowMapRenderTexture = new RenderTextureClass();
	result = m_ShadowMapRenderTexture->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), gShadowmapWidth, gShadowmapHeight, gShadowMapNear, gShadowMapDepth, DXGI_FORMAT_R32G32B32A32_FLOAT);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize shadow map texture.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the debug display of shadow map
	m_DepthDebugDisplayPlane = new QuadModel();
	result = m_DepthDebugDisplayPlane->Initialize(m_Direct3D->GetDevice(), screenHeight / 6.0f, screenHeight / 6.0f);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize debug quad.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the light object.
	m_Light = new DirectionalLightClass();

	// Directional light
	// m_Light->SetDirectionalColor(9.0f, 5.0f, 2.0f, 1.0f); // sunlight color
	m_Light->SetDirectionalColor(9.0f, 8.0f, 7.0f, 1.0f);
	m_Light->GenerateOrthoMatrix(40.0f, gShadowMapNear, gShadowMapDepth);

	m_Light->SetDirection(XMConvertToRadians(startingDirectionalLightDirX), XMConvertToRadians(startingDirectionalLightDirY), 0.0f);

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

	///////////////////////////////
	// Timer //
	///////////////////////////////
	// Create and initialize the timer object.
	m_Timer = new TimerClass();
	result = m_Timer->Initialize();
	if(!result) {
		return false;
	}

	///////////////////////////////
	// Text //
	///////////////////////////////
	
	// Create and initialize the font shader object.
	m_FontShader = new FontShaderClass();
	result = m_FontShader->Initialize(m_Direct3D->GetDevice(), hwnd);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the font shader object.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the font object.
	m_Font = new FontClass();
	result = m_Font->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), 0);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the font object.", L"Error", MB_OK);
		return false;
	}

	///////////////////////////////
	// Mouse Button Display //
	///////////////////////////////

	// Set the initial mouse strings.
	//strcpy_s(mouseString1, "Mouse X: 0");
	//strcpy_s(mouseString2, "Mouse Y: 0");
	//strcpy_s(mouseString3, "Mouse Button: No");

	//// Create and initialize the text objects for the mouse strings.
	//m_MouseTexts = new TextClass[3];

	//result = m_MouseTexts[0].Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, mouseString1, 10, 35, 1.0f, 1.0f, 1.0f);
	//if(!result) {
	//	return false;
	//}

	//result = m_MouseTexts[1].Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, mouseString1, 10, 60, 1.0f, 1.0f, 1.0f);
	//if(!result) {
	//	return false;
	//}

	//result = m_MouseTexts[2].Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, 32, m_Font, mouseString1, 10, 85, 1.0f, 1.0f, 1.0f);
	//if(!result) {
	//	return false;
	//}

	///////////////////////////////
	// FPS Counter //
	///////////////////////////////
	// Create and initialize the fps object.
	m_Fps = new FpsClass();
	m_Fps->Initialize(1.0f);

	// Set the initial fps and fps string.
	strcpy_s(fpsString, "0");

	// Create and initialize the text object for the fps string.
	m_FpsString = new TextClass();

	result = m_FpsString->Initialize(
		m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(),
		screenWidth, screenHeight, 32,
		m_Font, fpsString, 10, 10,
		0.0f, 1.0f, 0.0f);
	if(!result) {
		return false;
	}

	return true;
}

bool ApplicationClass::Frame(InputClass* input) {
	int mouseX, mouseY;
	bool mouseDown;
	float frameTime {};

	// Calculate delta time
	ChronoTimePoint currentFrameTimePoint = std::chrono::steady_clock::now();
	m_DeltaTime = (currentFrameTimePoint - m_LastFrameTimePoint).count();
	m_LastFrameTimePoint = currentFrameTimePoint;

	// TEMP
	//std::cout << m_DeltaTime * 1000.0f << " ms" << std::endl;

	m_Time = (currentFrameTimePoint - m_StartTime).count();

	// Update the system stats.
	m_Timer->Frame();

	// Get the current frame time.
	frameTime = m_Timer->GetTime();

	// Update the sprite object (animation) using the frame time.
	//m_sprite->Update(frameTime);

    // Update the frames per second each frame.
    if(!UpdateFps()) {
        return false;
    }

	// Render 3D scene to render texture
	if(!RenderSceneToScreenTexture()) {
		return false;
	}

	// Render 3D scene depth to shadow map
	if(!RenderSceneDepthTexture()) {
		return false;
	}

	if(mb_ShowMainMenu) {
		UpdateMainImGuiWindow();
	}

	// Render scene texture with post processing to main back buffer
	if(!RenderToBackBuffer()) {
		return false;
	}

	/// USER INPUT
	// Exit app if escape is pressed
	if(input->IsEscapeKeyDown()) {
		return false;
	}

	if(input->IsTabKeyDown()) {
		mb_ShowMainMenu = !mb_ShowMainMenu;
		ShowCursor(mb_ShowMainMenu);
	}

	if(input->IsF1KeyDown()) {
		mb_RenderDebugQuad = !mb_RenderDebugQuad;
	}

	if(!mb_ShowMainMenu) {
		if(input->IsLeftShiftKeyUp()) {
			mb_FastMove = false;
		}
		else if(input->IsLeftShiftKeyDown()) {
			mb_FastMove = true;
		}

		// Get the location of the mouse from the input object,
		input->GetMouseLocation(mouseX, mouseY);

		// Check if the mouse has been pressed.
		mouseDown = input->IsMousePressed();

		// Camera Rotation
		float mouseSensitivity = 10.0f * m_DeltaTime;
		m_Camera->SetRotation(m_Camera->GetRotationX() + input->GetMouseAxisHorizontal() * mouseSensitivity, m_Camera->GetRotationY() + input->GetMouseAxisVertical() * mouseSensitivity, m_Camera->GetRotationZ());

		// Camera Translation
		XMFLOAT3 currLookAtDir = m_Camera->GetLookAtDir();
		XMFLOAT3 currRightDir = m_Camera->GetRightDir();
		XMVECTOR camMoveVector = XMVector3Normalize(XMVectorAdd(XMLoadFloat3(&currLookAtDir) * input->GetMoveAxisVertical(), XMLoadFloat3(&currRightDir) * input->GetMoveAxisHorizontal())) * m_DeltaTime * (mb_FastMove ? 15.0f : 5.0f);

		m_Camera->SetPosition(m_Camera->GetPositionX() + XMVectorGetX(camMoveVector), m_Camera->GetPositionY() + XMVectorGetY(camMoveVector), m_Camera->GetPositionZ() + XMVectorGetZ(camMoveVector));
	}

	// Update the mouse strings each frame.
	//result = UpdateMouseStrings(mouseX, mouseY, mouseDown);
	//if(!result) {
	//	return false;
	//}
	///

	return true;
}

bool ApplicationClass::UpdateFps() {
	char tempString[16], finalString[16];

	// Update the fps each frame.
	int fps = (int)m_Fps->Frame(m_DeltaTime);

	// fps from the previous frame was the same
	if(fps == -1) {
		return true;
	}

	// Truncate the fps to below 100,000.
	if(fps > 99999) {
		fps = 99999;
	}

	// Convert the fps integer to string format.
	sprintf_s(tempString, "%d", fps);

	// Setup the fps string.
	strcpy_s(finalString, tempString);
	//strcat_s(finalString, tempString);

	float red, green, blue;
	if(fps >= 60) {
		red = 1.0f;
		green = 1.0f;
		blue = 1.0f;
	}

	if(fps < 60) {
		red = 1.0f;
		green = 1.0f;
		blue = 0.0f;
	}

	if(fps < 30) {
		red = 1.0f;
		green = 0.0f;
		blue = 0.0f;
	}

	// Update the sentence vertex buffer with the new string information.
	bool result = m_FpsString->UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, 10, 10, red, green, blue);
	if(!result) {
		return false;
	}

	return true;
}

bool ApplicationClass::UpdateMouseStrings(int mouseX, int mouseY, bool mouseDown) {
	char tempString[16], finalString[32];
	bool result;

	// Convert the mouse X integer to string format.
	sprintf_s(tempString, "%d", mouseX);

	// Setup the mouse X string.
	strcpy_s(finalString, "Mouse X: ");
	strcat_s(finalString, tempString);

	// Update the sentence vertex buffer with the new string information.
	result = m_MouseTexts[0].UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, 10, 35, 1.0f, 1.0f, 1.0f);
	if(!result) {
		return false;
	}

	// Convert the mouse Y integer to string format.
	sprintf_s(tempString, "%d", mouseY);

	// Setup the mouse Y string.
	strcpy_s(finalString, "Mouse Y: ");
	strcat_s(finalString, tempString);

	// Update the sentence vertex buffer with the new string information.
	result = m_MouseTexts[1].UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, 10, 60, 1.0f, 1.0f, 1.0f);
	if(!result) {
		return false;
	}

	// Setup the mouse button string.
	if(mouseDown) {
		strcpy_s(finalString, "Mouse Button: Yes");
	}
	else {
		strcpy_s(finalString, "Mouse Button: No");
	}

	// Update the sentence vertex buffer with the new string information.
	result = m_MouseTexts[2].UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, 10, 85, 1.0f, 1.0f, 1.0f);
	if(!result) {
		return false;
	}

	return true;
}

bool ApplicationClass::RenderSceneDepthTexture() {
	// Set the render target to be the render texture and clear it.
	m_ShadowMapRenderTexture->SetRenderTarget();
	m_ShadowMapRenderTexture->ClearRenderTarget(1.0f, 1.0f, 1.0f, 1.0f);

	m_Direct3D->SetToFrontCullRasterState();

	for(GameObject go : m_GameObjects) {
		if(!go.RenderToDepth(m_Direct3D->GetDeviceContext(), m_Light, m_Time)) {
			return false;
		}
	}

	m_Direct3D->SetToBackCullRasterState();

	return true;
}

void ApplicationClass::UpdateMainImGuiWindow() {

	static ImGuiSliderFlags sliderFlags = ImGuiSliderFlags_AlwaysClamp;

	bool b_isMenuActive;
	//ImGui::ShowDemoWindow(&my_tool_active);

	ImGui::Begin("Scene Menu", &b_isMenuActive, ImGuiWindowFlags_None);

	//ImGui::Text("fps: %d", m_Fps->GetCurrentFPS());

	// Generate samples and plot them
	//float samples[100] {};
	//for(int n = 0; n < 100; n++)
	//	samples[n] = std::sinf(n * 0.2f + (float)ImGui::GetTime() * 1.5f);
	//ImGui::PlotLines("Samples", samples, 100);

	static float x = startingDirectionalLightDirX;
	static float y = startingDirectionalLightDirY;
	ImGui::Text("Direction Light Angle");
	ImGui::DragFloat("X", &x, 0.5f, -1000.0f, 1000.0f, "%.3f", sliderFlags);
	ImGui::DragFloat("Y", &y, 0.5f, -1000.0f, 1000.0f, "%.3f", sliderFlags);
	x = std::fmod(x, 360.0f);
	y = std::fmod(y, 360.0f);

	m_Light->SetDirection(XMConvertToRadians(x), XMConvertToRadians(y), 0.0f);
	
	ImGui::End();

	if(!b_isMenuActive) {
		mb_ShowMainMenu = false;
		ShowCursor(mb_ShowMainMenu);
	}
}

bool ApplicationClass::RenderSceneToScreenTexture() {
	// Set the render target to be the render texture and clear it.
	m_ScreenRenderTexture->SetRenderTarget();
	m_ScreenRenderTexture->ClearRenderTarget(0.01f, 0.01f, 0.015f, 1.0f);

	m_Camera->Render();

	// Get the matrices.
	XMMATRIX worldMatrix {}, viewMatrix {}, projectionMatrix {}, orthoMatrix {};
	m_Direct3D->GetWorldMatrix(worldMatrix);
	m_Camera->GetViewMatrix(viewMatrix);
	m_ScreenRenderTexture->GetProjectionMatrix(projectionMatrix);
	//m_screenRenderTexture->GetOrthoMatrix(orthoMatrix);

	///////////////////////////////
	// 3D Rendering //
	///////////////////////////////
	// Animate directional light
	//float animatedDir = std::sin(m_Time * 0.5f);
	//m_LightDirX = animatedDir;
	//m_LightDirY = -(animatedDir * 0.5f + 0.8f);
	//m_LightDirZ = 0.5f;

	// Opaque 3D PBR Objects
	for(GameObject go : m_GameObjects) {
		if(!go.Render(m_Direct3D->GetDeviceContext(), viewMatrix, projectionMatrix, m_ShadowMapRenderTexture->GetTextureSRV(), m_CubeMapObject->GetIrradianceMapSRV(), m_CubeMapObject->GetPrefilteredMapSRV(), m_CubeMapObject->GetPrecomputedBRDFSRV(), m_Light, m_Camera->GetPosition(), m_Time)) {
			return false;
		}
	}

	ID3D11ShaderResourceView* nullSRV[6] = {nullptr, nullptr,nullptr, nullptr, nullptr, nullptr};
	m_Direct3D->GetDeviceContext()->PSSetShaderResources(0, 6, nullSRV);

	// TODO: figure out elegant solution for raster states
	m_Direct3D->SetToFrontCullRasterState();
	m_CubeMapObject->Render(m_Direct3D->GetDeviceContext(), viewMatrix, projectionMatrix, CubeMapObject::kSkyBoxRender);
	m_Direct3D->SetToBackCullRasterState();


	//m_ScreenRenderTexture->TurnZBufferOff();
	//m_ScreenRenderTexture->EnableAlphaBlending();

	//// TODO: Transparent Objects

	//m_ScreenRenderTexture->TurnZBufferOn();
	//m_ScreenRenderTexture->DisableAlphaBlending();


	return true;
}

bool ApplicationClass::RenderToBackBuffer() {
	// Reset to the original back buffer and viewport
	m_Direct3D->SetToBackBufferRenderTarget();
	m_Direct3D->ResetViewport();

	// Clear the buffers to begin the scene.
	m_Direct3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);

	// Get the world, view, and projection matrices from the camera and d3d objects.
	XMMATRIX worldMatrix {}, viewMatrix {}, projectionMatrix {}, orthoMatrix {};
	m_Direct3D->GetWorldMatrix(worldMatrix);
	m_ScreenDisplayCamera->GetViewMatrix(viewMatrix);
	//m_direct3D->GetProjectionMatrix(projectionMatrix);
	m_Direct3D->GetOrthoMatrix(orthoMatrix);


	///////////////////////////////
	// 2D Rendering //
	///////////////////////////////
	// Turn off the Z buffer to begin all 2D rendering.
	m_Direct3D->TurnZBufferOff();
	m_Direct3D->EnableAlphaBlending();

	// Render the display plane using the texture shader and the render texture resource.
	m_ScreenDisplayPlane->Render(m_Direct3D->GetDeviceContext());
	if(!m_PostProcessShader->Render(m_Direct3D->GetDeviceContext(), m_ScreenDisplayPlane->GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix, m_ScreenRenderTexture->GetTextureSRV())) {
		return false;
	}

	///////////////////////////////
	// DEBUG textures (UI) //
	///////////////////////////////

	// Depth Debug Quad
	if(mb_RenderDebugQuad) {
		m_DepthDebugDisplayPlane->Render(m_Direct3D->GetDeviceContext());
		float depthQuadPosX = -m_ScreenWidth / 2.0f + m_ScreenHeight / 6.0f;
		float depthQuadPosY = -m_ScreenHeight / 2.0f + m_ScreenHeight / 6.0f;
		if(!m_DebugDepthShader->Render(m_Direct3D->GetDeviceContext(), m_DepthDebugDisplayPlane->GetIndexCount(), XMMatrixTranslation(depthQuadPosX, depthQuadPosY, 0), viewMatrix, orthoMatrix, m_ShadowMapRenderTexture->GetTextureSRV())) {
			return false;
		}
	}

	///////////////////////////////
	// DEBUG Text (UI) //
	///////////////////////////////
	// Render the fps text string using the font shader.
	m_FpsString->Render(m_Direct3D->GetDeviceContext());

	if(!m_FontShader->Render(m_Direct3D->GetDeviceContext(), m_FpsString->GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix,
		m_Font->GetTexture(), m_FpsString->GetPixelColor())) {
		return false;
	}

	// Render the mouse text strings using the font shader.
	//for(size_t i = 0; i < 3; i++) {
	//	m_MouseTexts[i].Render(m_Direct3D->GetDeviceContext());
	//	if(!m_FontShader->Render(m_Direct3D->GetDeviceContext(), m_MouseTexts[i].GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix,
	//		m_Font->GetTexture(), m_MouseTexts[i].GetPixelColor())) {
	//		return false;
	//	}
	//}

	// Turn the Z buffer back on now that all 2D rendering has completed.
	m_Direct3D->TurnZBufferOn();
	m_Direct3D->DisableAlphaBlending();

	/// Render IMGUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present the rendered scene to the screen.
	m_Direct3D->EndScene();
	return true;
}

void ApplicationClass::Shutdown() {
	// Release the text objects for the mouse strings.
	if(m_MouseTexts) {
		m_MouseTexts[0].Shutdown();
		m_MouseTexts[1].Shutdown();
		m_MouseTexts[2].Shutdown();

		delete[] m_MouseTexts;
		m_MouseTexts = nullptr;
	}

	if(m_CubeMapObject) {
		m_CubeMapObject->Shutdown();
		delete m_CubeMapObject;
		m_CubeMapObject = nullptr;
	}

	// Release the display plane object.
	if(m_ScreenDisplayPlane) {
		m_ScreenDisplayPlane->Shutdown();
		delete m_ScreenDisplayPlane;
		m_ScreenDisplayPlane = nullptr;
	}

	if(m_DepthDebugDisplayPlane) {
		m_DepthDebugDisplayPlane->Shutdown();
		delete m_DepthDebugDisplayPlane;
		m_DepthDebugDisplayPlane = nullptr;
	}

	// Release the render to texture object.
	if(m_ScreenRenderTexture) {
		m_ScreenRenderTexture->Shutdown();
		delete m_ScreenRenderTexture;
		m_ScreenRenderTexture = nullptr;
	}

	if(m_ShadowMapRenderTexture) {
		m_ShadowMapRenderTexture->Shutdown();
		delete m_ShadowMapRenderTexture;
		m_ShadowMapRenderTexture = nullptr;
	}

	// Release the text object for the fps string.
	if(m_FpsString) {
		m_FpsString->Shutdown();
		delete m_FpsString;
		m_FpsString = nullptr;
	}

	// Release the fps object.
	if(m_Fps) {
		delete m_Fps;
		m_Fps = nullptr;
	}

	// Release the font object.
	if(m_Font) {
		m_Font->Shutdown();
		delete m_Font;
		m_Font = nullptr;
	}

	// Release the font shader object.
	if(m_FontShader) {
		m_FontShader->Shutdown();
		delete m_FontShader;
		m_FontShader = nullptr;
	}

	// Release the timer object.
	if(m_Timer) {
		delete m_Timer;
		m_Timer = nullptr;
	}

	// Release the sprite object.
	//if(m_sprite) {
	//	m_sprite->Shutdown();
	//	delete m_sprite;
	//	m_sprite = nullptr;
	//}


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

	// Release the texture shader object.
	if(m_PostProcessShader) {
		m_PostProcessShader->Shutdown();
		delete m_PostProcessShader;
		m_PostProcessShader = nullptr;
	}

	if(m_DebugDepthShader) {
		m_DebugDepthShader->Shutdown();
		delete m_DebugDepthShader;
		m_DebugDepthShader = nullptr;
	}

	// Release the light objects.
	//if(m_lights) {
	//	delete[] m_lights;
	//	m_lights = nullptr;
	//}

	// Release the light object.
	if(m_Light) {
		delete m_Light;
		m_Light = nullptr;
	}

	for(GameObject go : m_GameObjects) {
		go.Shutdown();
	}

	// Release the camera objects.
	if(m_Camera) {
		delete m_Camera;
		m_Camera = nullptr;
	}

	if(m_ScreenDisplayCamera) {
		delete m_ScreenDisplayCamera;
		m_ScreenDisplayCamera = nullptr;
	}

	// Release the Direct3D object.
	if(m_Direct3D) {
		m_Direct3D->Shutdown();
		delete m_Direct3D;
		m_Direct3D = nullptr;
	}
}


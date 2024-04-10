#include "Application.h"

#include "D3DInstance.h"
#include "Input.h"
#include "Camera.h"
#include "TextureShader.h"
#include "FontShader.h"

#include "RenderTexture.h"
#include "QuadModel.h"
#include "Model.h"
#include "Texture.h"

#include "Font.h"
#include "Text.h"
#include "Sprite.h"
#include "Timer.h"
#include "FpsCounter.h"

#include "Scene.h"

#include "imgui_impl_dx11.h"

#include <iostream>
#include <algorithm>

bool Application::Initialize(bool isFullScreen, int screenWidth, int screenHeight, HWND hwnd) {
	bool result;
	char fpsString[32];

	// Note: member currently only used to calc debug quad screen position later, this could probably be simplified
	m_ScreenWidth = screenWidth;
	m_ScreenHeight = screenHeight;

	m_StartTime = std::chrono::steady_clock::now();

	// Create and initialize the Direct3D object
	m_D3DInstance = new D3DInstance();
	result = m_D3DInstance->Initialize(screenWidth, screenHeight, g_VsyncEnabled, hwnd, isFullScreen, g_ScreenDepth, g_ScreenNear);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	// Create camera to view screen render texture
	m_ScreenDisplayCamera = new Camera();
	// Display Camera is orthographic
	m_ScreenDisplayCamera->SetPosition(0.0f, 0.0f, -1.0f);
	// Initialize stationary camera to view full screen quads
	m_ScreenDisplayCamera->Update();

	/// Screen Render
	// Create and initialize the screen render texture
	m_ScreenRenderTexture = new RenderTexture();
	result = m_ScreenRenderTexture->Initialize(m_D3DInstance->GetDevice(), m_D3DInstance->GetDeviceContext(), screenWidth, screenHeight, g_ScreenNear, g_ScreenDepth, DXGI_FORMAT_R32G32B32A32_FLOAT);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize screen render texture.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the screen display quad
	// NOTE: not sure why we need to divide dimensions by 2.0f here
	m_ScreenDisplayQuad = new QuadModel();
	result = m_ScreenDisplayQuad->Initialize(m_D3DInstance->GetDevice(), screenWidth / 2.0f, screenHeight / 2.0f);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize screen quad.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the debug displays
	// NOTE: unit quad can be reused with scaling instead of multiple quads
	m_DebugDisplayQuad1 = new QuadModel();
	result = m_DebugDisplayQuad1->Initialize(m_D3DInstance->GetDevice(), screenHeight / 6.0f, screenHeight / 6.0f);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize debug quad.", L"Error", MB_OK);
		return false;
	}

	m_DebugDisplayQuad2 = new QuadModel();
	result = m_DebugDisplayQuad2->Initialize(m_D3DInstance->GetDevice(), screenWidth / 6.0f, screenHeight / 6.0f);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize debug quad.", L"Error", MB_OK);
		return false;
	}

	/// Screen shaders
	// Shader for depth debug quad
	m_PassThroughShader = new TextureShader();
	result = m_PassThroughShader->Initialize(m_D3DInstance->GetDevice(), hwnd, false /*isPostProcessShader*/);
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

	/// 3D Objects
	XMMATRIX screenCameraViewMatrix{};
	m_ScreenDisplayCamera->GetViewMatrix(screenCameraViewMatrix);
	XMMATRIX screenOrthoMatrix {};
	m_D3DInstance->GetOrthoMatrix(screenOrthoMatrix);

	m_DemoScene = new Scene();
	result = m_DemoScene->InitializeDemoScene(m_D3DInstance, hwnd, screenCameraViewMatrix, m_ScreenDisplayQuad, g_ShadowMapWidth, g_ShadowMapNear, g_ShadowMapDepth, m_ScreenRenderTexture, m_PassThroughShader);
	if(!result) {
		MessageBox(hwnd, L"Could not load scene.", L"Error", MB_OK);
		return false;
	}

	/// Timer
	// Create and initialize the timer object.
	m_Timer = new Timer();
	result = m_Timer->Initialize();
	if(!result) {
		MessageBox(hwnd, L"Could not initialize timer.", L"Error", MB_OK);
		return false;
	}

	/// Text
	// Create and initialize the font shader object.
	m_FontShader = new FontShader();
	result = m_FontShader->Initialize(m_D3DInstance->GetDevice(), hwnd);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the font shader object.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the font object.
	m_Font = new Font();
	result = m_Font->Initialize(m_D3DInstance->GetDevice(), m_D3DInstance->GetDeviceContext(), 0);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the font object.", L"Error", MB_OK);
		return false;
	}

	/// FPS Counter
	// Create and initialize the fps object.
	m_FpsCounter = new FpsCounter();
	m_FpsCounter->Initialize(1.0f);

	// Set the initial fps and fps string.
	strcpy_s(fpsString, "0");

	// Create and initialize the text object for the fps string.
	m_FpsString = new Text();
	result = m_FpsString->Initialize(
		m_D3DInstance->GetDevice(), m_D3DInstance->GetDeviceContext(),
		screenWidth, screenHeight, 32,
		m_Font, fpsString, 10, 10,
		0.0f, 1.0f, 0.0f);
	if(!result) {
		return false;
	}

	return true;
}

bool Application::Frame(Input* input) {
	// Calculate delta time
	ChronoTimePoint currentFrameTimePoint = std::chrono::steady_clock::now();
	m_DeltaTime = (currentFrameTimePoint - m_LastFrameTimePoint).count();
	m_LastFrameTimePoint = currentFrameTimePoint;

	//std::cout << m_DeltaTime * 1000.0f << " ms" << std::endl;

	m_Time = (currentFrameTimePoint - m_StartTime).count();

	// Update the system stats.
	m_Timer->Frame();

	// Get the current frame time.
	float frameTime = m_Timer->GetTime();

	// Update the sprite object (animation) using the frame time.
	//m_sprite->Update(frameTime);

    // Update the frames per second each frame.
    if(!UpdateFpsDisplay()) {
        return false;
    }

	// Render 3D scene to render texture
	if(!RenderSceneToScreenTexture()) {
		return false;
	}

	// Render 3D scene depth to shadow map
	if(!m_DemoScene->RenderDirectionalLightSceneDepth(m_Time)) {
		return false;
	}

	// Render scene texture with post processing to main back buffer
	if(!RenderToBackBuffer()) {
		return false;
	}

	/// USER INPUT
	// Exit app if escape key down or ImGui quit flag
	if(input->IsEscapeKeyDown() || mb_QuitApp) {
		return false;
	}

	if(input->IsTabKeyDown()) {
		mb_ShowImGuiMenu = !mb_ShowImGuiMenu;
		ShowCursor(mb_ShowImGuiMenu);
	}

	if(input->IsF1KeyDown()) {
		mb_RenderDebugQuad = !mb_RenderDebugQuad;
	}

	// Enable camera controls if IMGUI is hidden
	if(!mb_ShowImGuiMenu) {
		m_DemoScene->ProcessInput(input, m_DeltaTime);
	}

	return true;
}

bool Application::UpdateFpsDisplay() {
	char tempString[16], finalString[16];

	// Update the fps each frame.
	int fps = (int)m_FpsCounter->Frame(m_DeltaTime);

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

	float red, green, blue;
	if(fps >= 60) {
		red = 1.0f;
		green = 1.0f;
		blue = 1.0f;
	}
	else if(fps < 60) {
		red = 1.0f;
		green = 1.0f;
		blue = 0.0f;
	}
	else if(fps < 30) {
		red = 1.0f;
		green = 0.0f;
		blue = 0.0f;
	}

	// Update the sentence vertex buffer with the new string information.
	bool result = m_FpsString->UpdateText(m_D3DInstance->GetDeviceContext(), m_Font, finalString, 10, 10, red, green, blue);
	if(!result) {
		return false;
	}

	return true;
}

bool Application::RenderSceneToScreenTexture() {
	// Set the render target to be the render texture and clear it.
	m_ScreenRenderTexture->SetRenderTargetAndViewPort();
	m_ScreenRenderTexture->ClearRenderTarget(0.01f, 0.01f, 0.015f, 1.0f);

	XMMATRIX projectionMatrix {};
	m_ScreenRenderTexture->GetProjectionMatrix(projectionMatrix);

	if(mb_IsWireFrameRender) {
		m_D3DInstance->SetToWireBackCullRasterState();
	}

	m_DemoScene->RenderScene(projectionMatrix, m_Time);

	return true;
}

bool Application::RenderToBackBuffer() {
	XMMATRIX worldMatrix {}, viewMatrix {}, projectionMatrix {}, orthoMatrix {};
	m_D3DInstance->GetWorldMatrix(worldMatrix);
	m_ScreenDisplayCamera->GetViewMatrix(viewMatrix);
	m_D3DInstance->GetOrthoMatrix(orthoMatrix);

	// NOTE: Calls m_D3DInstance->SetToBackBufferRenderTargetAndViewPort();
	// Render the display plane using the post processing setup in scene
	m_ScreenDisplayQuad->Render(m_D3DInstance->GetDeviceContext());
	if(!m_DemoScene->RenderPostProcess(m_ScreenDisplayQuad->GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix, m_ScreenRenderTexture->GetTextureSRV())) {
		return false;
	}

	/// Alpha blended
	m_D3DInstance->TurnZBufferOff();
	m_D3DInstance->EnableAlphaBlending();

	/// DEBUG Text (UI)
	if(mb_ShowScreenFPS) {
		m_FpsString->Render(m_D3DInstance->GetDeviceContext());
		if(!m_FontShader->Render(m_D3DInstance->GetDeviceContext(), m_FpsString->GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix,
			m_Font->GetTexture(), m_FpsString->GetPixelColor())) {
			return false;
		}
	}

	m_D3DInstance->TurnZBufferOn();
	m_D3DInstance->DisableAlphaBlending();

	/// DEBUG textures (UI) //
	// Depth Debug Quad
	if(mb_RenderDebugQuad) {
		m_DebugDisplayQuad1->Render(m_D3DInstance->GetDeviceContext());
		// Bottom left
		static XMMATRIX translationMatrix1 = XMMatrixTranslation(
			-m_ScreenWidth / 2.0f + m_ScreenHeight / 6.0f,
			-m_ScreenHeight / 2.0f + m_ScreenHeight / 6.0f, 0
		);
		if(!m_PassThroughShader->Render(m_D3DInstance->GetDeviceContext(), m_DebugDisplayQuad1->GetIndexCount(), translationMatrix1, viewMatrix, orthoMatrix, m_DemoScene->GetDirectionalShadowMapRenderTexture()->GetTextureSRV())) {
			return false;
		}

		m_DebugDisplayQuad2->Render(m_D3DInstance->GetDeviceContext());
		// Bottom right
		static XMMATRIX translationMatrix2 = XMMatrixTranslation(
			m_ScreenWidth / 2.0f - m_ScreenWidth / 6.0f,
			-m_ScreenHeight / 2.0f + m_ScreenHeight / 6.0f, 0
		);

		if(!m_PassThroughShader->Render(m_D3DInstance->GetDeviceContext(), m_DebugDisplayQuad2->GetIndexCount(), translationMatrix2, viewMatrix, orthoMatrix, m_DemoScene->GetDebugBloomOutput()->GetTextureSRV())) {
			return false;
		}
	}

	/// Render IMGUI
	if(mb_ShowImGuiMenu) {
		m_DemoScene->UpdateMainImGuiWindow(m_FpsCounter->GetCurrentFPS(), mb_IsWireFrameRender, mb_ShowImGuiMenu, mb_ShowScreenFPS, mb_QuitApp);
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present the rendered scene to the screen.
	m_D3DInstance->SwapPresent();
	return true;
}

void Application::Shutdown() {
	if(m_DemoScene) {
		m_DemoScene->Shutdown();
		delete m_DemoScene;
		m_DemoScene = nullptr;
	}

	if(m_ScreenDisplayQuad) {
		m_ScreenDisplayQuad->Shutdown();
		delete m_ScreenDisplayQuad;
		m_ScreenDisplayQuad = nullptr;
	}

	if(m_DebugDisplayQuad1) {
		m_DebugDisplayQuad1->Shutdown();
		delete m_DebugDisplayQuad1;
		m_DebugDisplayQuad1 = nullptr;
	}

	if(m_DebugDisplayQuad2) {
		m_DebugDisplayQuad2->Shutdown();
		delete m_DebugDisplayQuad2;
		m_DebugDisplayQuad2 = nullptr;
	}

	// Release the render to texture object.
	if(m_ScreenRenderTexture) {
		m_ScreenRenderTexture->Shutdown();
		delete m_ScreenRenderTexture;
		m_ScreenRenderTexture = nullptr;
	}

	// Release the text object for the fps string.
	if(m_FpsString) {
		m_FpsString->Shutdown();
		delete m_FpsString;
		m_FpsString = nullptr;
	}

	// Release the fps object.
	if(m_FpsCounter) {
		delete m_FpsCounter;
		m_FpsCounter = nullptr;
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

	if(m_PassThroughShader) {
		m_PassThroughShader->Shutdown();
		delete m_PassThroughShader;
		m_PassThroughShader = nullptr;
	}

	if(m_ScreenDisplayCamera) {
		delete m_ScreenDisplayCamera;
		m_ScreenDisplayCamera = nullptr;
	}


	// Release the Direct3D object.
	if(m_D3DInstance) {
		m_D3DInstance->Shutdown();
		delete m_D3DInstance;
		m_D3DInstance = nullptr;
	}
}


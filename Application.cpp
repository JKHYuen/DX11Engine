﻿#include "Application.h"

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
	//char mouseString1[32], mouseString2[32], mouseString3[32];

	// Note: currently only used for debug quad position calc
	m_ScreenWidth = screenWidth;
	m_ScreenHeight = screenHeight;

	m_StartTime = std::chrono::steady_clock::now();

	// Create and initialize the Direct3D object
	m_Direct3D = new D3DInstance();
	result = m_Direct3D->Initialize(screenWidth, screenHeight, gVsyncEnabled, hwnd, isFullScreen, gScreenDepth, gScreenNear);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	// Create the 3D world camera
	m_Camera = new Camera();
	m_Camera->SetPosition(0.0f, 4.0f, -10.0f);

	// Create camera to view screen render texture
	m_ScreenDisplayCamera = new Camera();
	// Display Camera is orthographic
	m_ScreenDisplayCamera->SetPosition(0.0f, 0.0f, -1.0f);
	// Initialize stationary camera to view full screen quads
	m_ScreenDisplayCamera->Render();

	/// Screen Render Texture
	// Create and initialize the screen render texture
	m_ScreenRenderTexture = new RenderTexture();
	result = m_ScreenRenderTexture->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, gScreenNear, gScreenDepth, DXGI_FORMAT_R32G32B32A32_FLOAT);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize screen render texture.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the screen display quad
	m_ScreenDisplayQuad = new QuadModel();
	result = m_ScreenDisplayQuad->Initialize(m_Direct3D->GetDevice(), screenWidth / 2.0f, screenHeight / 2.0f);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize screen quad.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the debug display of shadow map
	m_DebugDisplayQuad = new QuadModel();
	result = m_DebugDisplayQuad->Initialize(m_Direct3D->GetDevice(), screenHeight / 6.0f, screenHeight / 6.0f);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize debug quad.", L"Error", MB_OK);
		return false;
	}

	/// 2D Rendering
	m_PostProcessShader = new TextureShader();
	result = m_PostProcessShader->Initialize(m_Direct3D->GetDevice(), hwnd, true /*isPostProcessShader*/);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the texture shader object.", L"Error", MB_OK);
		return false;
	}

	// Shader for depth debug quad
	m_DebugDepthShader = new TextureShader();
	result = m_DebugDepthShader->Initialize(m_Direct3D->GetDevice(), hwnd, false /*isPostProcessShader*/);
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
	m_Direct3D->GetOrthoMatrix(screenOrthoMatrix);

	m_DemoScene = new Scene();
	m_DemoScene->InitializeDemoScene(m_Direct3D, hwnd, screenCameraViewMatrix, m_ScreenDisplayQuad, gShadowMapWidth, gShadowMapNear, gShadowMapDepth);

	/// Timer
	// Create and initialize the timer object.
	m_Timer = new Timer();
	result = m_Timer->Initialize();
	if(!result) {
		return false;
	}

	/// Text
	// Create and initialize the font shader object.
	m_FontShader = new FontShader();
	result = m_FontShader->Initialize(m_Direct3D->GetDevice(), hwnd);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the font shader object.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the font object.
	m_Font = new Font();
	result = m_Font->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), 0);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize the font object.", L"Error", MB_OK);
		return false;
	}

	/// Mouse Button Display 
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

	/// FPS Counter
	// Create and initialize the fps object.
	m_FpsCounter = new FpsCounter();
	m_FpsCounter->Initialize(1.0f);

	// Set the initial fps and fps string.
	strcpy_s(fpsString, "0");

	// Create and initialize the text object for the fps string.
	m_FpsString = new Text();

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

bool Application::Frame(Input* input) {
	int mouseX, mouseY;
	bool mouseDown;
	float frameTime {};

	// Calculate delta time
	ChronoTimePoint currentFrameTimePoint = std::chrono::steady_clock::now();
	m_DeltaTime = (currentFrameTimePoint - m_LastFrameTimePoint).count();
	m_LastFrameTimePoint = currentFrameTimePoint;

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
	if(!m_DemoScene->RenderDirectionalLightSceneDepth(m_Time)) {
		return false;
	}

	if(mb_ShowImGuiMenu) {
		m_DemoScene->UpdateMainImGuiWindow(m_FpsCounter->GetCurrentFPS(), mb_IsWireFrameRender, mb_ShowImGuiMenu);
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
		mb_ShowImGuiMenu = !mb_ShowImGuiMenu;
		ShowCursor(mb_ShowImGuiMenu);
	}

	if(input->IsF1KeyDown()) {
		mb_RenderDebugQuad = !mb_RenderDebugQuad;
	}

	// Enable camera controls if IMGUI is hidden
	if(!mb_ShowImGuiMenu) {
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

	return true;
}

bool Application::UpdateFps() {
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
	//strcat_s(finalString, tempString);

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
	bool result = m_FpsString->UpdateText(m_Direct3D->GetDeviceContext(), m_Font, finalString, 10, 10, red, green, blue);
	if(!result) {
		return false;
	}

	return true;
}

bool Application::UpdateMouseStrings(int mouseX, int mouseY, bool mouseDown) {
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

bool Application::RenderSceneToScreenTexture() {
	// Set the render target to be the render texture and clear it.
	m_ScreenRenderTexture->SetRenderTarget();
	m_ScreenRenderTexture->ClearRenderTarget(0.01f, 0.01f, 0.015f, 1.0f);

	m_Camera->Render();

	XMMATRIX viewMatrix {}, projectionMatrix {};
	m_Camera->GetViewMatrix(viewMatrix);
	m_ScreenRenderTexture->GetProjectionMatrix(projectionMatrix);

	if(mb_IsWireFrameRender) {
		m_Direct3D->SetToWireBackCullRasterState();
	}

	m_DemoScene->RenderScene(viewMatrix, projectionMatrix, m_Camera->GetPosition(), m_Time);

	return true;
}

bool Application::RenderToBackBuffer() {
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


	/// 2D Rendering
	// Turn off the Z buffer to begin all 2D rendering.
	m_Direct3D->TurnZBufferOff();
	m_Direct3D->EnableAlphaBlending();

	// Render the display plane using the texture shader and the render texture resource.
	m_ScreenDisplayQuad->Render(m_Direct3D->GetDeviceContext());
	if(!m_PostProcessShader->Render(m_Direct3D->GetDeviceContext(), m_ScreenDisplayQuad->GetIndexCount(), worldMatrix, viewMatrix, orthoMatrix, m_ScreenRenderTexture->GetTextureSRV())) {
		return false;
	}

	/// DEBUG textures (UI) //
	// Depth Debug Quad
	if(mb_RenderDebugQuad) {
		m_DebugDisplayQuad->Render(m_Direct3D->GetDeviceContext());
		float depthQuadPosX = -m_ScreenWidth / 2.0f + m_ScreenHeight / 6.0f;
		float depthQuadPosY = -m_ScreenHeight / 2.0f + m_ScreenHeight / 6.0f;
		if(!m_DebugDepthShader->Render(m_Direct3D->GetDeviceContext(), m_DebugDisplayQuad->GetIndexCount(), XMMatrixTranslation(depthQuadPosX, depthQuadPosY, 0), viewMatrix, orthoMatrix, m_DemoScene->GetDirectionalShadowMapRenderTexture()->GetTextureSRV())) {
			return false;
		}
	}

	/// DEBUG Text (UI)
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

void Application::Shutdown() {
	if(m_DemoScene) {
		m_DemoScene->Shutdown();
		delete m_DemoScene;
		m_DemoScene = nullptr;
	}

	if(m_MouseTexts) {
		m_MouseTexts[0].Shutdown();
		m_MouseTexts[1].Shutdown();
		m_MouseTexts[2].Shutdown();

		delete[] m_MouseTexts;
		m_MouseTexts = nullptr;
	}

	if(m_ScreenDisplayQuad) {
		m_ScreenDisplayQuad->Shutdown();
		delete m_ScreenDisplayQuad;
		m_ScreenDisplayQuad = nullptr;
	}

	if(m_DebugDisplayQuad) {
		m_DebugDisplayQuad->Shutdown();
		delete m_DebugDisplayQuad;
		m_DebugDisplayQuad = nullptr;
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


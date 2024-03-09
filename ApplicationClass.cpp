#include "ApplicationClass.h"

ApplicationClass::ApplicationClass() {}
ApplicationClass::ApplicationClass(const ApplicationClass& other) {}
ApplicationClass::~ApplicationClass() {}

bool ApplicationClass::Initialize(int screenWidth, int screenHeight, HWND hwnd) {
	bool result;
	char fpsString[32];
	//char mouseString1[32], mouseString2[32], mouseString3[32];

	// Note: currently only used for debug quad position calc
	m_ScreenWidth = screenWidth;
	m_ScreenHeight = screenHeight;

	m_StartTime = std::chrono::steady_clock::now();

	// Create and initialize the Direct3D object
	m_Direct3D = new D3DClass();
	result = m_Direct3D->Initialize(screenWidth, screenHeight, gVsyncEnabled, hwnd, gFullScreen, gScreenDepth, gScreenNear);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	// Create the 3D world camera
	m_Camera = new CameraClass();
	m_Camera->SetPosition(0.0f, 4.0f, -10.0f);

	// Create camera to view screen render texture
	m_DisplayCamera = new CameraClass();
	// Display Camera is orthographic
	m_DisplayCamera->SetPosition(0.0f, 0.0f, -1.0f);
	//m_displayCamera->SetPosition(0.0f, 0.0f, -2.4f);

	///////////////////////////////
	// Screen Render Texture //
	///////////////////////////////
	// Create and initialize the screen render texture
	m_ScreenRenderTexture = new RenderTextureClass();
	result = m_ScreenRenderTexture->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, gScreenNear, gScreenDepth, DXGI_FORMAT_R32G32B32A32_FLOAT);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize screen render texture.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the screen display quad
	m_ScreenDisplayPlane = new DisplayPlaneClass();
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
	// TODO: move raster state change into CubeMapObject class
	m_Direct3D->SetToFrontCullRasterState();
	m_CubeMapObject = new CubeMapObject();
	// rural_landscape_4k | industrial_sunset_puresky_4k | kloppenheim_03_4k | schachen_forest_4k
	result = m_CubeMapObject->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), hwnd, "industrial_sunset_puresky_4k", 2048);
	m_Direct3D->SetToBackCullRasterState();

	if(!result) {
		MessageBox(hwnd, L"Could not initialize cubemap.", L"Error", MB_OK);
		return false;
	}

	// Temp scene object system
	struct GameObjectData {
		std::string modelName {};
		std::string materialName {};
		XMFLOAT3 position {};
		XMFLOAT3 scale {1.0f, 1.0f, 1.0f};
		float yRotSpeed {};
		float uvScale = 1.0f;
		float heightMapScale = 0.1f;
	};

	const std::vector<GameObjectData> sampleSceneObjects = {
		// Objects
		{"sphere", "rust",         {-3.0f, 4.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f},
		{"sphere", "stonewall",  {0.0f, 4.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f},
		{"cube", "metal_grid", {3.0f, 4.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 0.1f},
		// floor
		{"plane",  "dirt",    {0.0f, 0.0f, 0.0f}, {5.0f, 1.0f, 5.0f}, 0.0f, 10.0f, 0.5f},
	};

	m_GameObjects.reserve(sampleSceneObjects.size());
	for(size_t i = 0; i < sampleSceneObjects.size(); i++) {
		m_GameObjects.emplace_back();
		if(!m_GameObjects[i].Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), hwnd, sampleSceneObjects[i].modelName, sampleSceneObjects[i].materialName)) {
			MessageBox(hwnd, L"Could not initialize PBR object.", L"Error", MB_OK);
			return false;
		}

		m_GameObjects[i].SetPosition(sampleSceneObjects[i].position);
		m_GameObjects[i].SetScale(sampleSceneObjects[i].scale);
		m_GameObjects[i].SetYRotationSpeed(sampleSceneObjects[i].yRotSpeed);
		m_GameObjects[i].SetUVScale(sampleSceneObjects[i].uvScale);
		m_GameObjects[i].SetHeightMapScale(sampleSceneObjects[i].heightMapScale);
	}

	///////////////////////////////
	// Lighting //
	///////////////////////////////

	// Create and initialize the shadow map texture
	m_ShadowMapRenderTexture = new RenderTextureClass();
	result = m_ShadowMapRenderTexture->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), gShadowmapWidth, gShadowmapHeight, gShadowMapNear, gShadowMapDepth, DXGI_FORMAT_R32G32B32A32_FLOAT);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize shadow map texture.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the debug display of shadow map
	m_DepthDebugDisplayPlane = new DisplayPlaneClass();
	result = m_DepthDebugDisplayPlane->Initialize(m_Direct3D->GetDevice(), screenHeight / 6.0f, screenHeight / 6.0f);
	if(!result) {
		MessageBox(hwnd, L"Could not initialize debug quad.", L"Error", MB_OK);
		return false;
	}

	// Create and initialize the light object.
	m_Light = new LightClass();

	// Directional light
	//m_Light->SetDirectionalColor(9.0f, 5.0f, 2.0f, 1.0f); // sunlight color
	m_Light->SetDirectionalColor(9.0f, 8.0f, 7.0f, 1.0f);
	m_Light->SetDirection(0.0f, -1.0f, 0.0f);

	m_Light->GenerateOrthoMatrix(20.0f, gShadowMapNear, gShadowMapDepth);

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
	m_Fps->Initialize();

	// Set the initial fps and fps string.
	strcpy_s(fpsString, "Fps: 0");

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
	bool result, mouseDown;
	float frameTime {};

	// Calculate delta time
	ChronoTimePoint currentFrameTimePoint = std::chrono::steady_clock::now();
	m_DeltaTime = (currentFrameTimePoint - m_LastFrameTimePoint).count();

	// TEMP
	//std::cout << m_deltaTime * 1000.0f << " ms" << std::endl;

	m_LastFrameTimePoint = currentFrameTimePoint;
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

	// Render scene texture with post processing to main back buffer
	if(!RenderToBackBuffer()) {
		return false;
	}

	/// USER INPUT
// Check if the user pressed escape and wants to exit the application.
	if(input->IsEscapePressed()) {
		return false;
	}

	// Get the location of the mouse from the input object,
	input->GetMouseLocation(mouseX, mouseY);

	// Check if the mouse has been pressed.
	mouseDown = input->IsMousePressed();

	// Camera Rotation
	float mouseSensitivity = 10.0f * m_DeltaTime;
	//m_Camera->SetRotation(m_Camera->GetRotationX() + input->GetMouseAxisVertical() * mouseSensitivity, m_Camera->GetRotationY() + input->GetMouseAxisHorizontal() * mouseSensitivity, m_Camera->GetRotationZ());
	m_Camera->SetRotation(m_Camera->GetRotationX() + input->GetMouseAxisHorizontal() * mouseSensitivity, m_Camera->GetRotationY() + input->GetMouseAxisVertical() * mouseSensitivity, m_Camera->GetRotationZ());

	// Camera Translation
	XMFLOAT3 currLookAtDir = m_Camera->GetLookAtDir();
	XMFLOAT3 currRightDir = m_Camera->GetRightDir();
	XMVECTOR camMoveVector = XMVector3Normalize(XMVectorAdd(XMLoadFloat3(&currLookAtDir) * input->GetMoveAxisVertical(), XMLoadFloat3(&currRightDir) * input->GetMoveAxisHorizontal())) * m_DeltaTime * 5.0f;

	m_Camera->SetPosition(m_Camera->GetPositionX() + XMVectorGetX(camMoveVector), m_Camera->GetPositionY() + XMVectorGetY(camMoveVector), m_Camera->GetPositionZ() + XMVectorGetZ(camMoveVector));

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
	float red, green, blue;

	// Update the fps each frame.
	int fps = m_Fps->Frame();

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
	strcpy_s(finalString, "Fps: ");
	strcat_s(finalString, tempString);

	// If fps is 60 or above set the fps color to green.
	if(fps >= 60) {
		red = 0.0f;
		green = 1.0f;
		blue = 0.0f;
	}

	// If fps is below 60 set the fps color to yellow.
	if(fps < 60) {
		red = 1.0f;
		green = 1.0f;
		blue = 0.0f;
	}

	// If fps is below 30 set the fps color to red.
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
	m_ShadowMapRenderTexture->SetRenderTarget(m_Direct3D->GetDeviceContext());
	m_ShadowMapRenderTexture->ClearRenderTarget(m_Direct3D->GetDeviceContext(), 1.0f, 1.0f, 1.0f, 1.0f);

	m_Direct3D->SetToFrontCullRasterState();

	for(GameObject go : m_GameObjects) {
		if(!go.RenderToDepth(m_Direct3D->GetDeviceContext(), m_Light, m_Time)) {
			return false;
		}
	}

	m_Direct3D->SetToBackCullRasterState();

	return true;
}

bool ApplicationClass::RenderSceneToScreenTexture() {
	// Set the render target to be the render texture and clear it.
	m_ScreenRenderTexture->SetRenderTarget(m_Direct3D->GetDeviceContext());
	m_ScreenRenderTexture->ClearRenderTarget(m_Direct3D->GetDeviceContext(), 0.01f, 0.01f, 0.015f, 1.0f);

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
	float animatedDir = std::sin(m_Time * 0.5f);
	float lightDirX = animatedDir;
	float lightDirY = -(animatedDir * 0.5f + 0.8f);
	float lightDirZ = 0.5f;

	// Update the directional light and shadow depth map
	XMVECTOR dirVec = XMVectorSet(lightDirX, lightDirY, lightDirZ, 0.0f);
	dirVec = XMVector3Normalize(dirVec);
	m_Light->SetDirection(XMVectorGetX(dirVec), XMVectorGetY(dirVec), XMVectorGetX(dirVec));

	// Note: a bit hacky
	float direcLightRenderDist = 15.0f;
	m_Light->SetPosition(-lightDirX * direcLightRenderDist, -lightDirY * direcLightRenderDist, -lightDirZ * direcLightRenderDist);
	m_Light->GenerateViewMatrix();

	// Opaque 3D PBR Objects
	for(GameObject go : m_GameObjects) {
		if(!go.Render(m_Direct3D->GetDeviceContext(), viewMatrix, projectionMatrix, m_ShadowMapRenderTexture->GetTextureSRV(), m_CubeMapObject->GetIrradianceSRV(), m_Light, m_Camera->GetPosition(), m_Time)) {
			return false;
		}
	}

	// TODO: cubemap
	m_Direct3D->SetToFrontCullRasterState();
	m_CubeMapObject->Render(m_Direct3D->GetDeviceContext(), viewMatrix, projectionMatrix, CubeMapObject::kSkyBox);
	m_Direct3D->SetToBackCullRasterState();


	//m_screenRenderTexture->TurnZBufferOff();
	//m_screenRenderTexture->EnableAlphaBlending();

	// TODO: Transparent Objects

	//m_screenRenderTexture->TurnZBufferOn();
	//m_screenRenderTexture->DisableAlphaBlending();

	return true;
}

bool ApplicationClass::RenderToBackBuffer() {
	// Reset to the original back buffer and viewport
	m_Direct3D->SetToBackBufferRenderTarget();
	m_Direct3D->ResetViewport();

	// Clear the buffers to begin the scene.
	m_Direct3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);
	// Generate the view matrix based on the camera's position.
	m_DisplayCamera->Render();

	// Get the world, view, and projection matrices from the camera and d3d objects.
	XMMATRIX worldMatrix {}, viewMatrix {}, projectionMatrix {}, orthoMatrix {};
	m_Direct3D->GetWorldMatrix(worldMatrix);
	m_DisplayCamera->GetViewMatrix(viewMatrix);
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
	m_DepthDebugDisplayPlane->Render(m_Direct3D->GetDeviceContext());
	float depthQuadPosX = -m_ScreenWidth / 2.0f + m_ScreenHeight / 6.0f;
	float depthQuadPosY = -m_ScreenHeight / 2.0f + m_ScreenHeight / 6.0f;
	m_DebugDepthShader->Render(m_Direct3D->GetDeviceContext(), m_DepthDebugDisplayPlane->GetIndexCount(), XMMatrixTranslation(depthQuadPosX, depthQuadPosY, 0), viewMatrix, orthoMatrix, m_ShadowMapRenderTexture->GetTextureSRV());

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

	if(m_DisplayCamera) {
		delete m_DisplayCamera;
		m_DisplayCamera = nullptr;
	}

	// Release the Direct3D object.
	if(m_Direct3D) {
		m_Direct3D->Shutdown();
		delete m_Direct3D;
		m_Direct3D = nullptr;
	}
}


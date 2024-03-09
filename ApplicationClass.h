#pragma once
#include <windows.h>
#include <vector>
#include <chrono>
#include <iostream>

#include "d3dclass.h"
#include "InputClass.h"
#include "CameraClass.h"
#include "ModelClass.h"
#include "PBRShaderClass.h"
#include "TextureShaderClass.h"
#include "FontShaderClass.h"
#include "DepthShaderClass.h"

#include "RenderTextureClass.h"
#include "DisplayPlaneClass.h"

#include "TextClass.h"
#include "LightClass.h"
#include "SpriteClass.h"
#include "TimerClass.h"
#include "FpsClass.h"

#include "GameObject.h"
#include "CubeMapObject.h"

constexpr bool gFullScreen = false;
constexpr bool gVsyncEnabled = true;
constexpr float gScreenDepth = 1000.0f;
constexpr float gScreenNear = 0.1f;
constexpr int gShadowmapWidth = 2048;
constexpr int gShadowmapHeight = 2048;
constexpr float gShadowMapDepth = 50.0f;
constexpr float gShadowMapNear = 1.0f;

using ChronoTimePoint = std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<float>>;

class ApplicationClass {
public:
	ApplicationClass();
	ApplicationClass(const ApplicationClass&);
	~ApplicationClass();

	bool Initialize(int, int, HWND);
	void Shutdown();
	bool Frame(InputClass*);

private:
	bool RenderToBackBuffer();
	bool RenderSceneToScreenTexture();
	bool RenderSceneDepthTexture();

	bool UpdateMouseStrings(int, int, bool);
	bool UpdateFps();

	int m_ScreenWidth {};
	int m_ScreenHeight {};

	D3DClass* m_Direct3D {};
	TimerClass* m_Timer {};
	CameraClass* m_Camera {};
	CameraClass* m_DisplayCamera {};

	std::vector<GameObject> m_GameObjects {};

	// Directional light
	LightClass* m_Light {};

	// Point lights
	//LightClass* m_lights {};
	//int m_numLights {};

	// Screen Render Texture
	RenderTextureClass* m_ScreenRenderTexture {};
	RenderTextureClass* m_ShadowMapRenderTexture {};
	DisplayPlaneClass* m_ScreenDisplayPlane {};
	DisplayPlaneClass* m_DepthDebugDisplayPlane {};

	CubeMapObject* m_CubeMapObject {};

	TextureShaderClass* m_PostProcessShader {};
	TextureShaderClass* m_DebugDepthShader {};
	//SpriteClass* m_sprite {};

	FontShaderClass* m_FontShader {};
	FontClass* m_Font {};
	TextClass* m_TextString1 {};
	TextClass* m_TextString2 {};

	FpsClass* m_Fps {};
	TextClass* m_FpsString {};
	TextClass* m_MouseTexts {};

	// Custom Timer
	ChronoTimePoint m_StartTime {};
	ChronoTimePoint m_LastFrameTimePoint {};
	float m_DeltaTime {};
	// Time since App launch
	float m_Time {};
};
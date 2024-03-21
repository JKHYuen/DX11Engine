#pragma once
#include <windows.h>
#include <vector>
#include <chrono>

constexpr bool gFullScreen = false;
constexpr int gDefaultWindowedWidth = 1280;
constexpr int gDefaultWindowedHeight = 720;

constexpr bool gVsyncEnabled = true;
constexpr float gScreenDepth = 1000.0f;
constexpr float gScreenNear = 0.1f;
constexpr int gShadowmapWidth = 2048;
constexpr int gShadowmapHeight = 2048;
constexpr float gShadowMapDepth = 50.0f;
constexpr float gShadowMapNear = 1.0f;

using ChronoTimePoint = std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<float>>;

class D3DClass;
class InputClass;
class CameraClass;
class TextureShaderClass;
class FontShaderClass;
class SpriteClass;
class RenderTextureClass;
class QuadModel;

class FontClass;
class TextClass;
class LightClass;
class SpriteClass;
class TimerClass;
class FpsClass;

class GameObject;
class CubeMapObject;

class ApplicationClass {
public:
	ApplicationClass();
	ApplicationClass(const ApplicationClass&);
	~ApplicationClass();

	bool Initialize(int, int, HWND);
	void Shutdown();
	bool Frame(InputClass*);

	D3DClass* GetD3DClass() { return m_Direct3D; };

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
	CameraClass* m_ScreenDisplayCamera {};

	std::vector<GameObject> m_GameObjects {};

	// Directional light
	LightClass* m_Light {};

	// Point lights
	//LightClass* m_lights {};
	//int m_numLights {};

	// Screen Rendering
	RenderTextureClass* m_ScreenRenderTexture {};
	RenderTextureClass* m_ShadowMapRenderTexture {};
	QuadModel* m_ScreenDisplayPlane {};
	QuadModel* m_DepthDebugDisplayPlane {};

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

	bool m_bRenderDebugQuad {};
	bool m_bFastMove {};

	// Custom Timer
	ChronoTimePoint m_StartTime {};
	ChronoTimePoint m_LastFrameTimePoint {};
	float m_DeltaTime {};
	// Time since App launch
	float m_Time {};

};
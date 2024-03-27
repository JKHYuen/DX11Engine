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
constexpr float gShadowMapDepth = 100.0f;
constexpr float gShadowMapNear = 1.0f;

using ChronoTimePoint = std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<float>>;

class D3DInstance;
class Input;

class Camera;
class TextureShader;
class FontShader;
class Sprite;
class RenderTexture;
class QuadModel;

class Font;
class Text;
class DirectionalLight;
class Sprite;
class Timer;
class FpsCounter;

class GameObject;
class PBRShader;
class DepthShader;
class CubeMapObject;

class Application {
public:
	Application();
	Application(const Application&);
	~Application();

	bool Initialize(bool isFullScreen, int screenWidth, int screenHeight, HWND hwnd);
	void Shutdown();
	bool Frame(Input* input);
	void UpdateMainImGuiWindow();

	D3DInstance* GetD3DClass() { return m_Direct3D; };

private:
	bool RenderToBackBuffer();
	bool RenderSceneToScreenTexture();
	bool RenderSceneDepthTexture();

	bool UpdateMouseStrings(int, int, bool);
	bool UpdateFps();

	int m_ScreenWidth {};
	int m_ScreenHeight {};

	D3DInstance* m_Direct3D {};
	Timer* m_Timer {};
	Camera* m_Camera {};
	Camera* m_ScreenDisplayCamera {};

	std::vector<GameObject> m_GameObjects {};

	// Directional light
	DirectionalLight* m_Light {};

	// Point lights
	//LightClass* m_lights {};
	//int m_numLights {};

	// Screen Rendering
	RenderTexture* m_ScreenRenderTexture {};
	RenderTexture* m_ShadowMapRenderTexture {};
	QuadModel* m_ScreenDisplayPlane {};
	QuadModel* m_DepthDebugDisplayPlane {};

	CubeMapObject* m_CubeMapObject {};

	TextureShader* m_PostProcessShader {};
	TextureShader* m_DebugDepthShader {};

	//SpriteClass* m_sprite {};

	FontShader* m_FontShader {};
	Font* m_Font {};
	Text* m_TextString1 {};
	Text* m_TextString2 {};

	FpsCounter* m_Fps {};
	Text* m_FpsString {};
	Text* m_MouseTexts {};

	bool mb_RenderDebugQuad {};
	bool mb_FastMove {};
	bool mb_ShowMainMenu {};

	PBRShader* m_PBRShaderInstance {};
	DepthShader* m_DepthShaderInstance {};

	// Custom Timer
	ChronoTimePoint m_StartTime {};
	ChronoTimePoint m_LastFrameTimePoint {};
	float m_DeltaTime {};
	// Time since App launch
	float m_Time {};

};
#pragma once
#define NOMINMAX
#include <windows.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include "GameObject.h"

constexpr bool gFullScreen = false;
constexpr int gDefaultWindowedWidth = 1280;
constexpr int gDefaultWindowedHeight = 720;

constexpr bool gVsyncEnabled = true;
constexpr float gScreenDepth = 1000.0f;
constexpr float gScreenNear = 0.1f;
constexpr int gShadowMapWidth = 2048;
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
class Model;
class Texture;

class Font;
class Text;
class Sprite;
class Timer;
class FpsCounter;

struct ID3D11Device;
class Scene;

class Application {
public:
	Application() {}
	Application(const Application&) {}
	~Application() {}

	bool Initialize(bool isFullScreen, int screenWidth, int screenHeight, HWND hwnd);
	void Shutdown();
	bool Frame(Input* input);

	D3DInstance* GetD3DClass() { return m_Direct3D; };

private:
	bool RenderToBackBuffer();
	bool RenderSceneToScreenTexture();

	bool UpdateMouseStrings(int, int, bool);
	bool UpdateFps();

	int m_ScreenWidth {};
	int m_ScreenHeight {};

	D3DInstance* m_Direct3D {};
	Camera* m_Camera {};
	Camera* m_ScreenDisplayCamera {};
	
	// Scene
	Scene* m_DemoScene {};

	// Screen Rendering
	RenderTexture* m_ScreenRenderTexture {};

	QuadModel* m_ScreenDisplayQuad {};
	QuadModel* m_DebugDisplayQuad {};

	TextureShader* m_PostProcessShader {};
	TextureShader* m_DebugDepthShader {};

	//SpriteClass* m_sprite {};

	FontShader* m_FontShader {};
	Font* m_Font {};
	Text* m_TextString1 {};
	Text* m_TextString2 {};

	bool mb_FastMove {};

	// Debug
	bool mb_RenderDebugQuad {};
	bool mb_ShowImGuiMenu {};
	bool mb_IsWireFrameRender {};

	FpsCounter* m_FpsCounter {};
	Text* m_FpsString {};
	Text* m_MouseTexts {};

	// Timer
	ChronoTimePoint m_StartTime {};
	ChronoTimePoint m_LastFrameTimePoint {};
	float m_DeltaTime {};
	// Time since App launch
	float m_Time {};

	// LEGACY
	Timer* m_Timer {};

};
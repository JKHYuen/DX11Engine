#pragma once
#define NOMINMAX
#include <windows.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include "GameObject.h"

constexpr bool g_FullScreen = false;
constexpr int g_DefaultWindowedWidth = 1280;
constexpr int g_DefaultWindowedHeight = 720;

constexpr bool g_VsyncEnabled = true;
constexpr float g_ScreenDepth = 1000.0f;
constexpr float g_ScreenNear = 0.1f;
constexpr int g_ShadowMapWidth = 2048;
constexpr float g_ShadowMapDepth = 100.0f;
constexpr float g_ShadowMapNear = 1.0f;

using ChronoTimePoint = std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<float>>;

class D3DInstance;
class Input;

class Camera;
class TextureShader;
class FontShader;
class Sprite;
class RenderTexture;
class Bloom;
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

	D3DInstance* GetD3DClass() { return m_D3DInstance; };

private:
	bool RenderToBackBuffer();
	bool RenderSceneToScreenTexture();

	bool UpdateFpsDisplay();

private:
	int m_ScreenWidth {};
	int m_ScreenHeight {};

	D3DInstance* m_D3DInstance {};
	Camera* m_ScreenDisplayCamera {};
	
	// Scene
	Scene* m_DemoScene {};

	// Screen Rendering
	RenderTexture* m_ScreenRenderTexture {};
	//Bloom* m_BloomEffect {};

	QuadModel* m_ScreenDisplayQuad {};
	QuadModel* m_DebugDisplayQuad1 {};
	QuadModel* m_DebugDisplayQuad2 {};

	//TextureShader* m_PostProcessShader {};
	TextureShader* m_DebugDepthShader {};

	//SpriteClass* m_sprite {};

	FontShader* m_FontShader {};
	Font* m_Font {};

	// Debug
	bool mb_RenderDebugQuad {};
	bool mb_ShowImGuiMenu {};
	bool mb_IsWireFrameRender {};
	bool mb_ShowScreenFPS {};

	FpsCounter* m_FpsCounter {};
	Text* m_FpsString {};

	// Timer
	ChronoTimePoint m_StartTime {};
	ChronoTimePoint m_LastFrameTimePoint {};
	float m_DeltaTime {};
	// Time since App launch
	float m_Time {};

	// LEGACY
	Timer* m_Timer {};

};
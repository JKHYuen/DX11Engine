#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>

#include "GameObject.h"

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

	bool Initialize(bool b_IsFullScreen, bool b_IsVsyncEnabled, int screenWidth, int screenHeight, int defaultWindowedWidth, int defaultWindowedHeight, float nearZ, float farZ, int shadowMapResolution, float shadowMapNear, float shadowMapFar, HWND hwnd);
	void Shutdown();
	bool Frame(Input* input);

	D3DInstance* GetD3DInstance() const { return m_D3DInstance; }
	HWND GetHWND() const { return m_Hwnd; }

	QuadModel* GetScreenDisplayQuadInstance() const { return m_ScreenDisplayQuad; }
	RenderTexture* GetScreenRenderTexture() const { return m_ScreenRenderTexture; }
	Camera* GetScreenDisplayCamera() const { return m_ScreenDisplayCamera; }

private:
	bool RenderToBackBuffer();
	bool RenderSceneToScreenTexture();
	bool UpdateFpsDisplay();

	bool GenerateRenderQuads(int screenWidth, int screenHeight);
	bool ToggleFullscreen();

private:
	bool mb_IsFullScreen {};
	bool mb_QuitApp {};

	float m_ScreenNear {};
	float m_ScreenFar {};
	int m_DefaultWindowedWidth  {};
	int m_DefaultWindowedHeight {};

	D3DInstance* m_D3DInstance {};
	HWND m_Hwnd {};
	Camera* m_ScreenDisplayCamera {};

	Scene* m_DemoScene {};

	/// Screen Rendering
	RenderTexture* m_ScreenRenderTexture {};
	QuadModel* m_ScreenDisplayQuad {};
	// Shader that only renders texture with no effects
	TextureShader* m_PassThroughShader {};

	FontShader* m_FontShader {};
	Font* m_Font {};
	//SpriteClass* m_sprite {};

	/// Debug
	QuadModel* m_DebugDisplayQuad1 {};
	QuadModel* m_DebugDisplayQuad2 {};
	bool mb_RenderDebugQuad1 {};
	bool mb_RenderDebugQuad2 {};
	// for hardcoded debug quad placement
	XMMATRIX m_DebugQuadTranslationMatrix1 {};
	XMMATRIX m_DebugQuadTranslationMatrix2 {};

	bool mb_ShowImGuiMenu {};
	bool mb_IsWireFrameRender {};
	bool mb_ShowScreenFPS {};

	FpsCounter* m_FpsCounter {};
	Text* m_FpsString {};

	/// Timer
	ChronoTimePoint m_StartTime {};
	ChronoTimePoint m_LastFrameTimePoint {};
	float m_DeltaTime {};
	// Time since App launch
	float m_Time {};

	// LEGACY
	Timer* m_Timer {};

};
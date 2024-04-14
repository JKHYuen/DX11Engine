#pragma once
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

	bool Initialize(bool b_IsFullScreen, bool b_IsVsyncEnabled, int screenWidth, int screenHeight, float screenNear, float screenFar, int shadowMapResolution, float shadowMapNear, float shadowMapFar, HWND hwnd);
	void Shutdown();
	bool Frame(Input* input);

	D3DInstance* GetD3DInstance() const { return m_D3DInstance; }
	HWND GetHWND() const { return m_Hwnd; }

	QuadModel* GetScreenDisplayQuadInstance() const { return m_ScreenDisplayQuad; }
	RenderTexture* GetScreenRenderTexture() const { return m_ScreenRenderTexture; }
	//TextureShader* GetPassThroughShaderInstance() const { return m_PassThroughShader; }
	Camera* GetScreenDisplayCamera() const { return m_ScreenDisplayCamera; }

private:
	bool RenderToBackBuffer();
	bool RenderSceneToScreenTexture();
	bool UpdateFpsDisplay();

private:
	bool mb_QuitApp {};
	D3DInstance* m_D3DInstance {};
	HWND m_Hwnd {};
	Camera* m_ScreenDisplayCamera {};

	// Scene
	Scene* m_DemoScene {};

	// Screen Rendering
	RenderTexture* m_ScreenRenderTexture {};

	QuadModel* m_ScreenDisplayQuad {};
	QuadModel* m_DebugDisplayQuad1 {};
	QuadModel* m_DebugDisplayQuad2 {};

	// Shader that only renders texture with no effects
	TextureShader* m_PassThroughShader {};

	//SpriteClass* m_sprite {};

	FontShader* m_FontShader {};
	Font* m_Font {};

	// Debug
	bool mb_RenderDebugQuad1 {};
	bool mb_RenderDebugQuad2 {};
	XMMATRIX m_DebugQuadTranslationMatrix1 {};
	XMMATRIX m_DebugQuadTranslationMatrix2 {};
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
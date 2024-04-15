#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

class Input;
class Application;

class EngineSystem {
public:
	EngineSystem() {}
	EngineSystem(const EngineSystem&) {}
	~EngineSystem() {}

	bool Initialize();
	void Shutdown();
	void Run();

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

	int GetUserScreenWidth() const { return m_UserScreenWidth; }
	int GetUserScreenHeight() const { return m_UserScreenHeight; }

private:
	bool Frame();
	void InitializeWindows(int&, int&);

private:
	LPCWSTR m_ApplicationName {};
	HINSTANCE m_Hinstance {};
	HWND m_Hwnd {};

	Input* m_Input {};
	Application* m_Application {};

	int m_UserScreenWidth {};
	int m_UserScreenHeight {};
};

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static EngineSystem* g_ApplicationHandle = nullptr;


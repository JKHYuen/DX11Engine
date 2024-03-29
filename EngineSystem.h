#pragma once
#define WIN32_LEAN_AND_MEAN
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

private:
	bool Frame();
	void InitializeWindows(int&, int&);

private:
	LPCWSTR m_ApplicationName {};
	HINSTANCE m_Hinstance {};
	HWND m_Hwnd {};

	Input* m_Input {};
	Application* m_Application {};
};

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static EngineSystem* g_ApplicationHandle = nullptr;


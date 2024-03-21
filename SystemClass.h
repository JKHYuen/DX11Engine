#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class InputClass;
class ApplicationClass;

class SystemClass {
public:
	SystemClass();
	SystemClass(const SystemClass&);
	~SystemClass();

	bool Initialize();
	void Shutdown();
	void Run();

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

private:
	bool Frame();
	void InitializeWindows(int&, int&);

private:
	LPCWSTR m_applicationName {};
	HINSTANCE m_hinstance {};
	HWND m_hwnd {};

	InputClass* m_input {};
	ApplicationClass* m_application {};
};

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static SystemClass* g_ApplicationHandle = nullptr;


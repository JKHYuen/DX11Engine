#include "EngineSystem.h"

#include "D3DInstance.h"
#include "Input.h"
#include "Application.h"

#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <cmath>
#include <iostream>

// App params (hardcoded)
namespace {
	constexpr bool sb_StartFullScreenState = false;
	constexpr bool sb_IsVsyncEnabled = true;
	constexpr int s_DefaultWindowedWidth = 1280;
	constexpr int s_DefaultWindowedHeight = 720;

	// This is used for 3d world cam and cam to render screen quad
	constexpr float s_NearZ = 0.1f;
	constexpr float s_FarZ = 1000.0f;

	constexpr int s_ShadowMapResolution = 4096;
	constexpr float s_ShadowMapFar = 150.0f;
	constexpr float s_ShadowMapNear = 1.0f;
	constexpr float s_ShadowDistance = 100.0f;
}

bool EngineSystem::Initialize() {
	int screenWidth {};
	int screenHeight {};
	bool result;

	// Initialize the windows api
	InitializeWindows(screenWidth, screenHeight);

	// Create and initialize the input object
	m_Input = new Input();
	result = m_Input->Initialize(m_Hinstance, m_Hwnd, screenWidth, screenHeight);
	if(!result) {
		return false;
	}

	// Create and initialize the application class object
	m_Application = new Application();
	result = m_Application->Initialize(sb_StartFullScreenState, sb_IsVsyncEnabled, screenWidth, screenHeight, s_DefaultWindowedWidth, s_DefaultWindowedHeight, s_NearZ, s_FarZ, s_ShadowMapResolution, s_ShadowMapNear, s_ShadowMapFar, s_ShadowDistance, m_Hwnd);
	if (!result) {
		return false;
	}

	/// Initialize DearImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); 
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(m_Hwnd);
	ImGui_ImplDX11_Init(m_Application->GetD3DInstance()->GetDevice(), m_Application->GetD3DInstance()->GetDeviceContext());

	return true;
}

void EngineSystem::Run() {
	MSG msg {};
	bool result{};

	// Initialize the message structure.
	ZeroMemory(&msg, sizeof(MSG));

	// Loop until there is a quit message from the window or the user.
	while(1) {
		// Handle the windows messages.
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if(msg.message == WM_QUIT) {
			break;
		}
		else {
			// Otherwise do the frame processing.
			result = Frame();
			if(!result) {
				break;
			}
		}
	}
}

bool EngineSystem::Frame() {
	bool result;

	/// Input
	result = m_Input->Frame();
	if(!result) {
		return false;
	}

	/// IMGUI
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	/// App / Render
	result = m_Application->Frame(m_Input);
	if(!result) {
		return false;
	}

	return true;	
}

LRESULT CALLBACK EngineSystem::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
	return DefWindowProc(hwnd, umsg, wparam, lparam);
}

void EngineSystem::InitializeWindows(int& screenWidth, int& screenHeight) {
	// Get an external pointer to this object.	
	s_EngineSystemHandle = this;

	// Get the instance of this application.
	m_Hinstance = GetModuleHandle(NULL);

	// Give the application a name.
	m_ApplicationName = L"DX11Engine";

	// Setup the windows class with default settings.
	WNDCLASSEX wc {};
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_Hinstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = m_ApplicationName;
	wc.cbSize = sizeof(WNDCLASSEX);

	// Register the window class.
	RegisterClassEx(&wc);

	/// UNUSED - (for exclusive fullscreen) this app currently only supports borderelss windowed fullscreen
	//if(sb_IsFullScreen) {
	//	// If full screen set the screen to maximum size of the users desktop and 32bit.
	//  DEVMODE dmScreenSettings;
	//	memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
	//	dmScreenSettings.dmSize = sizeof(dmScreenSettings);
	//	dmScreenSettings.dmPelsWidth = (unsigned long)screenWidth;
	//	dmScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
	//	dmScreenSettings.dmBitsPerPel = 32;
	//	dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

	//	// Change the display settings to full screen.
	//	ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

	//	// Set the position of the window to the top left corner.
	//	posX = posY = 0;
	//}

	// Set starting windowed resolution.
	screenWidth = sb_StartFullScreenState ? GetSystemMetrics(SM_CXSCREEN) : s_DefaultWindowedWidth;
	screenHeight = sb_StartFullScreenState ? GetSystemMetrics(SM_CYSCREEN) : s_DefaultWindowedHeight;

	// Place the window in the middle of the screen.
	int posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
	int posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;

	DWORD windowStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP;

	// NOTE: Border causes stretching of viewport, remove this for now
	//if(!sb_IsFullScreen) windowStyle |= WS_CAPTION | WS_SYSMENU;

	// Create the window with the screen settings and get the handle to it.
	m_Hwnd = CreateWindowEx(WS_EX_APPWINDOW, m_ApplicationName, m_ApplicationName, windowStyle,
		posX, posY, screenWidth, screenHeight, NULL, NULL, m_Hinstance, NULL);

	// Bring the window up on the screen and set it as main focus.
	ShowWindow(m_Hwnd, SW_SHOW);
	SetForegroundWindow(m_Hwnd);
	SetFocus(m_Hwnd);

	// Hide the mouse cursor.
	ShowCursor(false);

	//RECT rect {};
	//GetWindowRect(m_Hwnd, &rect);
	//MapWindowPoints(m_Hwnd, nullptr, reinterpret_cast<POINT*>(&rect), 2);
	//ClipCursor(&rect);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EngineSystem::WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
	if(ImGui_ImplWin32_WndProcHandler(hwnd, umessage, wparam, lparam))
		return true;

	switch(umessage) {
		// Check if the window is being destroyed.
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		// Check if the window is being closed.
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;

		// Pass all other messages
		default:
			return s_EngineSystemHandle->MessageHandler(hwnd, umessage, wparam, lparam);
	}
}

void EngineSystem::Shutdown() {
	/// Release the application class object.
	if(m_Application) {
		m_Application->Shutdown();
		delete m_Application;
		m_Application = nullptr;
	}

	/// Release the input object.
	if(m_Input) {
		delete m_Input;
		m_Input = nullptr;
	}

	/// Shutdown the window.
	ShowCursor(true);

	// Fix the display settings if leaving full screen mode.
	if(sb_StartFullScreenState) {
		ChangeDisplaySettings(NULL, 0);
	}

	// Remove the window.
	DestroyWindow(m_Hwnd);
	m_Hwnd = NULL;

	// Remove the application instance.
	UnregisterClass(m_ApplicationName, m_Hinstance);
	m_Hinstance = NULL;

	// Release the pointer to this class.
	s_EngineSystemHandle = NULL;

	/// Shutdown ImGui
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}





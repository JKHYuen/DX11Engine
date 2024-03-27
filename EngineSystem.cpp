#include "EngineSystem.h"

#include "D3DInstance.h"
#include "Input.h"
#include "Application.h"

#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <cmath>
#include <iostream>

EngineSystem::EngineSystem() {}
EngineSystem::EngineSystem(const EngineSystem& other) {}
EngineSystem::~EngineSystem() {}

bool EngineSystem::Initialize() {
	// Initialize the width and height of the screen to zero before sending the variables into the function.
	int screenWidth {};
	int screenHeight {};
	bool result;

	// Initialize the windows api.
	InitializeWindows(screenWidth, screenHeight);

	// Create and initialize the input object.  This object will be used to handle reading the keyboard input from the user.
	m_Input = new Input();
	result = m_Input->Initialize(m_Hinstance, m_Hwnd, screenWidth, screenHeight);
	if(!result) {
		return false;
	}

	// Create and initialize the application class object.  This object will handle rendering all the graphics for this application.
	m_application = new Application();
	result = m_application->Initialize(gFullScreen, screenWidth, screenHeight, m_Hwnd);
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
	ImGui_ImplDX11_Init(m_application->GetD3DClass()->GetDevice(), m_application->GetD3DClass()->GetDeviceContext());

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
	result = m_application->Frame(m_Input);
	if(!result) {
		return false;
	}

	return true;	
}

LRESULT CALLBACK EngineSystem::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
	return DefWindowProc(hwnd, umsg, wparam, lparam);
}

void EngineSystem::InitializeWindows(int& screenWidth, int& screenHeight) {
	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;
	int posX, posY;

	// Get an external pointer to this object.	
	g_ApplicationHandle = this;

	// Get the instance of this application.
	m_Hinstance = GetModuleHandle(NULL);

	// Give the application a name.
	m_ApplicationName = L"DX11EngineTest";

	// Setup the windows class with default settings.
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

	// Determine the resolution of the clients desktop screen.
	screenWidth = GetSystemMetrics(SM_CXSCREEN);
	screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Setup the screen settings depending on whether it is running in full screen or in windowed mode.
	if(gFullScreen) {
		// If full screen set the screen to maximum size of the users desktop and 32bit.
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = (unsigned long)screenWidth;
		dmScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
		dmScreenSettings.dmBitsPerPel = 64;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Change the display settings to full screen.
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		// Set the position of the window to the top left corner.
		posX = posY = 0;
	}
	else {
		// If windowed then set it to 800x600 resolution.
		screenWidth = gDefaultWindowedWidth;
		screenHeight = gDefaultWindowedHeight;

		// Place the window in the middle of the screen.
		posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;
	}

	// Create the window with the screen settings and get the handle to it.
	m_Hwnd = CreateWindowEx(WS_EX_APPWINDOW, m_ApplicationName, m_ApplicationName,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP, 
		posX, posY, screenWidth, screenHeight, NULL, NULL, m_Hinstance, NULL);

	// Bring the window up on the screen and set it as main focus.
	ShowWindow(m_Hwnd, SW_SHOW);
	SetForegroundWindow(m_Hwnd);
	SetFocus(m_Hwnd);

	// Hide the mouse cursor.
	ShowCursor(false);

	//RECT rect {};
	//GetWindowRect(m_hwnd, &rect);
	//MapWindowPoints(m_hwnd, nullptr, reinterpret_cast<POINT*>(&rect), 2);
	//ClipCursor(&rect);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
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

		// All other messages pass to the message handler in the system class.
		default:
			return g_ApplicationHandle->MessageHandler(hwnd, umessage, wparam, lparam);
	}
}

void EngineSystem::Shutdown() {
	/// Release the application class object.
	if(m_application) {
		m_application->Shutdown();
		delete m_application;
		m_application = nullptr;
	}

	/// Release the input object.
	if(m_Input) {
		delete m_Input;
		m_Input = nullptr;
	}

	/// Shutdown ImGui
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	/// Shutdown the window.
	// Show the mouse cursor.
	ShowCursor(true);

	// Fix the display settings if leaving full screen mode.
	if(gFullScreen) {
		ChangeDisplaySettings(NULL, 0);
	}

	// Remove the window.
	DestroyWindow(m_Hwnd);
	m_Hwnd = NULL;

	// Remove the application instance.
	UnregisterClass(m_ApplicationName, m_Hinstance);
	m_Hinstance = NULL;

	// Release the pointer to this class.
	g_ApplicationHandle = NULL;

	return;
}





#include "inputclass.h"
#include <iostream>

InputClass::InputClass() {
	m_DirectInput = nullptr;
	m_Keyboard    = nullptr;
	m_Mouse       = nullptr;
}

InputClass::InputClass(const InputClass& other) {}
InputClass::~InputClass() {}

bool InputClass::Initialize(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight) {
    HRESULT result;

    // Store the screen size which will be used for positioning the mouse cursor.
    m_ScreenWidth = screenWidth;
    m_ScreenHeight = screenHeight;

    // Initialize the location of the mouse on the screen.
    m_MouseX = 0;
    m_MouseY = 0;

    // Initialize the main direct input interface.
    result = DirectInput8Create(hinstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_DirectInput, NULL);
    if(FAILED(result)) {
        return false;
    }

    // Initialize the direct input interface for the keyboard.
    result = m_DirectInput->CreateDevice(GUID_SysKeyboard, &m_Keyboard, NULL);
    if(FAILED(result)) {
        return false;
    }

    // Set the data format.  In this case since it is a keyboard we can use the predefined data format.
    result = m_Keyboard->SetDataFormat(&c_dfDIKeyboard);
    if(FAILED(result)) {
        return false;
    }

    // Set the cooperative level of the keyboard to not share with other programs.
    result = m_Keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
    if(FAILED(result)) {
        return false;
    }

    // Now acquire the keyboard.
    result = m_Keyboard->Acquire();
    if(FAILED(result)) {
        return false;
    }

    // Initialize the direct input interface for the mouse.
    result = m_DirectInput->CreateDevice(GUID_SysMouse, &m_Mouse, NULL);
    if(FAILED(result)) {
        return false;
    }

    // Set the data format for the mouse using the pre-defined mouse data format.
    result = m_Mouse->SetDataFormat(&c_dfDIMouse);
    if(FAILED(result)) {
        return false;
    }

    // Set the cooperative level of the mouse to share with other programs.
    result = m_Mouse->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    if(FAILED(result)) {
        return false;
    }

    // Acquire the mouse.
    result = m_Mouse->Acquire();
    if(FAILED(result)) {
        return false;
    }

    return true;
}

// https://www.gamedev.net/reference/articles/article842.asp
static USHORT ScanToChar(DWORD scanCode) {	//obtain keyboard information	
    static HKL layout = GetKeyboardLayout(0);
    static UCHAR keyboardState[256];

    if(!GetKeyboardState(keyboardState)) 
        return 0;

    UINT vk = MapVirtualKeyEx(scanCode, 1, layout);

    USHORT asciiValue;
    ToAsciiEx(vk, scanCode, keyboardState, &asciiValue, 0, layout);

    return asciiValue;
}

bool InputClass::Frame() {
    bool result;

    // Read the current state of the keyboard.
    result = ReadKeyboard();
    if(!result) {
        return false;
    }

    // Read the current state of the mouse.
    result = ReadMouse();
    if(!result) {
        return false;
    }

    // Print pressed key to console
    //for(int i = 0; i < sizeof(m_keyboardState) / sizeof(unsigned char) ; i++) {
    //    if(m_keyboardState[i] & 0x80) {
    //        std::cout << ScanToChar(i) << std::endl;
    //    }
    //}

    // Process the changes in the mouse and keyboard.
    ProcessInput();

    return true;
}

bool InputClass::ReadKeyboard() {
    m_PrevFrameKeyboardState = m_CurrentKeyboardState;

    // Read the keyboard device.
    HRESULT result = m_Keyboard->GetDeviceState(sizeof(m_CurrentKeyboardState), (LPVOID)&m_CurrentKeyboardState);
    if(FAILED(result)) {
        // If the keyboard lost focus or was not acquired then try to get control back.
        if((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED)) {
            m_Keyboard->Acquire();
        }
        else {
            return false;
        }
    }

    return true;
}

bool InputClass::ReadMouse() {
    HRESULT result;

    // Read the mouse device.
    result = m_Mouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&m_MouseState);
    if(FAILED(result)) {
        // If the mouse lost focus or was not acquired then try to get control back.
        if((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED)) {
            m_Mouse->Acquire();
        }
        else {
            return false;
        }
    }

    return true;
}

void InputClass::ProcessInput() {
    // Update the location of the mouse cursor based on the change of the mouse location during the frame.
    m_MouseX += m_MouseState.lX;
    m_MouseY += m_MouseState.lY;

    // Ensure the mouse location doesn't exceed the screen width or height.
    if(m_MouseX < 0) { m_MouseX = 0; }
    if(m_MouseY < 0) { m_MouseY = 0; }

    if(m_MouseX > m_ScreenWidth) { m_MouseX = m_ScreenWidth; }
    if(m_MouseY > m_ScreenHeight) { m_MouseY = m_ScreenHeight; }
}

float InputClass::GetMoveAxisHorizontal() {
    if(m_CurrentKeyboardState[DIK_A] & 0x80) {
        return -1.0f;
    }
    if(m_CurrentKeyboardState[DIK_D] & 0x80) {
        return 1.0f;
    }
    return 0.0f;
}

float InputClass::GetMoveAxisVertical() {
    if(m_CurrentKeyboardState[DIK_W] & 0x80) {
        return 1.0f;
    }
    if(m_CurrentKeyboardState[DIK_S] & 0x80) {
        return -1.0f;
    }
    return 0.0f;
}


void InputClass::GetMouseLocation(int& mouseX, int& mouseY) {
    mouseX = m_MouseX;
    mouseY = m_MouseY;
    return;
}

bool InputClass::IsMousePressed() {
    // Check the left mouse button state.
    return m_MouseState.rgbButtons[0] & 0x80;
}

void InputClass::Shutdown() {
    // Release the mouse.
    if(m_Mouse) {
        m_Mouse->Unacquire();
        m_Mouse->Release();
        m_Mouse = nullptr;
    }

    // Release the keyboard.
    if(m_Keyboard) {
        m_Keyboard->Unacquire();
        m_Keyboard->Release();
        m_Keyboard = nullptr;
    }

    // Release the main interface to direct input.
    if(m_DirectInput) {
        m_DirectInput->Release();
        m_DirectInput = nullptr;
    }
}




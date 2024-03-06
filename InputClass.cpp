#include "inputclass.h"
#include <iostream>

InputClass::InputClass() {
	m_directInput = nullptr;
	m_keyboard    = nullptr;
	m_mouse       = nullptr;
}

InputClass::InputClass(const InputClass& other) {}
InputClass::~InputClass() {}


bool InputClass::Initialize(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight) {
    HRESULT result;

    // Store the screen size which will be used for positioning the mouse cursor.
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // Initialize the location of the mouse on the screen.
    m_mouseX = 0;
    m_mouseY = 0;

    // Initialize the main direct input interface.
    result = DirectInput8Create(hinstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_directInput, NULL);
    if(FAILED(result)) {
        return false;
    }

    // Initialize the direct input interface for the keyboard.
    result = m_directInput->CreateDevice(GUID_SysKeyboard, &m_keyboard, NULL);
    if(FAILED(result)) {
        return false;
    }

    // Set the data format.  In this case since it is a keyboard we can use the predefined data format.
    result = m_keyboard->SetDataFormat(&c_dfDIKeyboard);
    if(FAILED(result)) {
        return false;
    }

    // Set the cooperative level of the keyboard to not share with other programs.
    result = m_keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
    if(FAILED(result)) {
        return false;
    }

    // Now acquire the keyboard.
    result = m_keyboard->Acquire();
    if(FAILED(result)) {
        return false;
    }

    // Initialize the direct input interface for the mouse.
    result = m_directInput->CreateDevice(GUID_SysMouse, &m_mouse, NULL);
    if(FAILED(result)) {
        return false;
    }

    // Set the data format for the mouse using the pre-defined mouse data format.
    result = m_mouse->SetDataFormat(&c_dfDIMouse);
    if(FAILED(result)) {
        return false;
    }

    // Set the cooperative level of the mouse to share with other programs.
    result = m_mouse->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    if(FAILED(result)) {
        return false;
    }

    // Acquire the mouse.
    result = m_mouse->Acquire();
    if(FAILED(result)) {
        return false;
    }

    return true;
}

void InputClass::Shutdown() {
    // Release the mouse.
    if(m_mouse) {
        m_mouse->Unacquire();
        m_mouse->Release();
        m_mouse = nullptr;
    }

    // Release the keyboard.
    if(m_keyboard) {
        m_keyboard->Unacquire();
        m_keyboard->Release();
        m_keyboard = nullptr;
    }

    // Release the main interface to direct input.
    if(m_directInput) {
        m_directInput->Release();
        m_directInput = nullptr;
    }

    return;
}

// https://www.gamedev.net/reference/articles/article842.asp
static char ScanToChar(DWORD scanCode) {	//obtain keyboard information	
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
    HRESULT result;

    // Read the keyboard device.
    result = m_keyboard->GetDeviceState(sizeof(m_keyboardState), (LPVOID)&m_keyboardState);
    if(FAILED(result)) {
        // If the keyboard lost focus or was not acquired then try to get control back.
        if((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED)) {
            m_keyboard->Acquire();
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
    result = m_mouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&m_mouseState);
    if(FAILED(result)) {
        // If the mouse lost focus or was not acquired then try to get control back.
        if((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED)) {
            m_mouse->Acquire();
        }
        else {
            return false;
        }
    }

    return true;
}

void InputClass::ProcessInput() {
    // Update the location of the mouse cursor based on the change of the mouse location during the frame.
    m_mouseX += m_mouseState.lX;
    m_mouseY += m_mouseState.lY;

    // Ensure the mouse location doesn't exceed the screen width or height.
    if(m_mouseX < 0) { m_mouseX = 0; }
    if(m_mouseY < 0) { m_mouseY = 0; }

    if(m_mouseX > m_screenWidth) { m_mouseX = m_screenWidth; }
    if(m_mouseY > m_screenHeight) { m_mouseY = m_screenHeight; }
}

float InputClass::GetMouseAxisHorizontal() {
    return m_mouseState.lX;
}

float InputClass::GetMouseAxisVertical() {
    return m_mouseState.lY;
}

bool InputClass::IsEscapePressed() {
    // Do a bitwise and on the keyboard state to check if the escape key is currently being pressed.
    if(m_keyboardState[DIK_ESCAPE] & 0x80) {
        return true;
    }

    return false;
}

float InputClass::GetMoveAxisHorizontal() {
    if(m_keyboardState[DIK_A] & 0x80) {
        return -1.0f;
    }
    if(m_keyboardState[DIK_D] & 0x80) {
        return 1.0f;
    }
    return 0.0f;
}

float InputClass::GetMoveAxisVertical() {
    if(m_keyboardState[DIK_W] & 0x80) {
        return 1.0f;
    }
    if(m_keyboardState[DIK_S] & 0x80) {
        return -1.0f;
    }
    return 0.0f;
}


void InputClass::GetMouseLocation(int& mouseX, int& mouseY) {
    mouseX = m_mouseX;
    mouseY = m_mouseY;
    return;
}

bool InputClass::IsMousePressed() {
    // Check the left mouse button state.
    return m_mouseState.rgbButtons[0] & 0x80;
}






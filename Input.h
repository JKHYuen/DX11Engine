#pragma once

#define DIRECTINPUT_VERSION 0x0800

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include <dinput.h>
#include <array>

class Input {
public:
    Input() {}
    Input(const Input&) {}
    ~Input() {}

    bool Initialize(HINSTANCE, HWND, int, int);
    void Shutdown();
    bool Frame();

    void GetMouseLocation(int&, int&);
    bool IsMousePressed();

    bool IsKeyDown(int keyCode) const { return (m_CurrentKeyboardState[keyCode] & 0x80) && !(m_PrevFrameKeyboardState[keyCode] & 0x80); }
    bool IsKeyUp(int keyCode) const { return !(m_CurrentKeyboardState[keyCode] & 0x80) && (m_PrevFrameKeyboardState[keyCode] & 0x80); }
    bool IsKeyHeld(int keyCode) const { return m_CurrentKeyboardState[keyCode] & 0x80; }

    LONG GetMouseAxisHorizontal() const { return m_MouseState.lX; };
    LONG GetMouseAxisVertical() const { return m_MouseState.lY; };

    float GetMoveAxisHorizontal();
    float GetMoveAxisVertical();

private:
    bool ReadKeyboard();
    bool ReadMouse();
    void ProcessInput();

private:
    IDirectInput8* m_DirectInput {};
    IDirectInputDevice8* m_Keyboard {};
    IDirectInputDevice8* m_Mouse {};

    std::array<unsigned char, 256> m_CurrentKeyboardState {};
    std::array<unsigned char, 256> m_PrevFrameKeyboardState {};
    DIMOUSESTATE m_MouseState {};

    int m_ScreenWidth {}, m_ScreenHeight {};
    int m_MouseX {}, m_MouseY {};
};

#pragma once
#pragma comment(lib, "winmm.lib")

#include <windows.h>
#include <mmsystem.h>
#include <iostream>

class FpsClass {
public:
    FpsClass();
    FpsClass(const FpsClass&);
    ~FpsClass();

    void Initialize();
    int  Frame();

private:
    int m_fps   {};
    int m_count {};
    unsigned long m_startTime {};
    int m_previousFps {};
};
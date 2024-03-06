#include "fpsclass.h"

FpsClass::FpsClass() {}
FpsClass::FpsClass(const FpsClass& other) {}
FpsClass::~FpsClass() {}

void FpsClass::Initialize() {
    m_fps   = 0;
    m_count = 0;
    m_previousFps = -1;

    m_startTime = timeGetTime();
}

int FpsClass::Frame() {
    m_count++;

    if(timeGetTime() >= (m_startTime + 1000)) {
        m_fps = m_count;
        m_count = 0;

        m_startTime = timeGetTime();
    }

	// Check if the fps from the previous frame was the same, if so don't need to update the text string.
    if(m_previousFps == m_fps) {
        return -1;
    }

    m_previousFps = m_fps;
    return m_fps;
}
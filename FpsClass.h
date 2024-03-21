#pragma once

class FpsClass {
public:
    FpsClass();
    FpsClass(const FpsClass&);
    ~FpsClass();

    void Initialize(float sampleDuration);
    float Frame(float deltaTime);

private:
    int m_FrameCount {};
    unsigned long m_LastInterval {};
    float m_SampleDuration {};
    int m_previousFps {};

    float m_CurrentFrameCount {};
    float m_CurrentFrameDuration {};
    float m_BestFrameDuration {};
    float m_WorstFrameDuration {};
};
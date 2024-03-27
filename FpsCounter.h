#pragma once

class FpsCounter {
public:
    FpsCounter();
    FpsCounter(const FpsCounter&);
    ~FpsCounter();

    void Initialize(float sampleDuration);
    float Frame(float deltaTime);

    int GetCurrentFPS() const { return m_CurrentFPS; }

private:
    float m_SampleDuration {};
    int m_CurrentFPS {};

    float m_CurrentFrameCount {};
    float m_CurrentFrameDuration {};
    float m_BestFrameDuration {};
    float m_WorstFrameDuration {};
};
#include "FpsCounter.h"

#include <limits>
#include <iostream>

// Sample duration is in seconds
void FpsCounter::Initialize(float sampleDuration) {
    m_SampleDuration = sampleDuration;
}

float FpsCounter::Frame(float deltaTime) {
	float frameDuration = deltaTime;
	m_CurrentFrameCount++;
	m_CurrentFrameDuration += frameDuration;

	if(frameDuration < m_BestFrameDuration) {
		m_BestFrameDuration = frameDuration;
	}
	if(frameDuration > m_WorstFrameDuration) {
		m_WorstFrameDuration = frameDuration;
	}

	if(m_CurrentFrameDuration >= m_SampleDuration) {
		m_CurrentFPS = m_CurrentFrameCount / m_CurrentFrameDuration;
		m_CurrentFrameCount = 0;
		m_CurrentFrameDuration = 0.0f;
		m_BestFrameDuration = std::numeric_limits<float>::max();
		m_WorstFrameDuration = 0.0f;
		return m_CurrentFPS;
	}
	else {
		return -1;
	}
}

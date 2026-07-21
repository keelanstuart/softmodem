#include "pch.h"
#include "ToneRecognizer.h"
#include <math.h>
#include <numbers>

ToneRecognizer::ToneRecognizer(float frequency)
{
	// assume 48Khz sampling
	const float omega = 2.0f * std::numbers::pi_v<float> * frequency / 48000.0f;
	m_Coefficient = 2.0f * cosf(omega);

	Reset();
}

void ToneRecognizer::Reset()
{
	m_S1 = m_S2 = 0.0f;
}

void ToneRecognizer::AddSample(float sample)
{
	float s0 = sample + m_Coefficient * m_S1 - m_S2;

	m_S2 = m_S1;
	m_S1 = s0;
}

float ToneRecognizer::Power() const
{
	return
		m_S1 * m_S1 +
		m_S2 * m_S2 -
		m_Coefficient * m_S1 * m_S2;
}

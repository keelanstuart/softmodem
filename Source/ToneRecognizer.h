#pragma once

class ToneRecognizer
{

public:
	ToneRecognizer(float frequency, float sample_rate = 48000.0f);

	void Reset();

	void AddSample(float sample);

	float Power() const;

protected:
	float m_S1, m_S2;
	float m_Coefficient;

};
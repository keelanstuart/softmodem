#include "Modem_Base.h"
#include "ToneRecognizer.h"
#include <array>

#pragma once


#define BELL103_ORIGINATE_MARK		1070.0f
#define BELL103_ORIGINATE_SPACE		1270.0f
#define BELL103_ANSWER_MARK			2225.0f
#define BELL103_ANSWER_SPACE		2025.0f

class Modem_Bell103 : public Modem
{

public:

	Modem_Bell103();
	virtual ~Modem_Bell103();

	virtual bool Initialize();
	virtual bool Start(Role r);
	virtual bool Stop();


protected:

	using Tone = enum
	{
		UNKNOWN = -1,
		MARK,
		SPACE
	};

	uint32_t m_FramesPerBit;

	// transmit stuff
	virtual void TxWaveform(float *poutput, uint32_t frame_count);
	void AdvanceTxState();
	double CurrentTxFrequency() const;

	uint32_t m_TxFramesRemaining;
	ma_waveform_config m_TxWaveConfig;
	ma_waveform m_TxWave;

	// receive stuff
	virtual void RxWaveform(const float *pinput, uint32_t frame_count);
	void RxAnalyzeWindow();
	void RxAdvanceFraming();

	std::array<float, 96> m_RxHistory;
	size_t m_RxHistoryIdx;
	size_t m_RxHistoryCount;
	size_t m_RxSamplesSinceAnalysis;
	float m_RxCarrierThreshold;
	float m_RxDecision;
	Tone m_RxDetectedTone;
	uint32_t m_RxSamplesRemaining;
	ToneRecognizer m_RxMark_Originate, m_RxSpace_Originate;
	ToneRecognizer m_RxMark_Answer, m_RxSpace_Answer;

	ToneRecognizer *m_pRxMark, *m_pRxSpace;
	float m_TxMark_Freq, m_TxSpace_Freq;
};
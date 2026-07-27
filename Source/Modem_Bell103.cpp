#include "pch.h"
#include "Modem_Bell103.h"


Modem_Bell103::Modem_Bell103()
	: Modem(),
	m_RxMark_Originate(BELL103_ORIGINATE_MARK),
	m_RxSpace_Originate(BELL103_ORIGINATE_SPACE),
	m_RxMark_Answer(BELL103_ANSWER_MARK),
	m_RxSpace_Answer(BELL103_ANSWER_SPACE),
	m_RxHistory{},
	m_RxHistoryIdx(0),
	m_RxHistoryCount(0),
	m_RxSamplesSinceAnalysis(0),
	m_pRxMark(nullptr),
	m_pRxSpace(nullptr),
	m_FramesPerBit(0),
	m_TxFramesRemaining(0),
	m_RxCarrierThreshold(0.005f),
	m_RxDecision(0.0f),
	m_RxDetectedTone(Tone::UNKNOWN),
	m_RxSamplesRemaining(0),
	m_TxMark_Freq(0.0f),
	m_TxSpace_Freq(0.0f)
{
	m_RxState = State::NoCarrier;
	m_RxBit = 0;
	m_RxByte = 0;
}

Modem_Bell103::~Modem_Bell103()
{

}


bool Modem_Bell103::Initialize()
{
	if (!__super::Initialize())
		return false;

	m_FramesPerBit = m_Device.sampleRate / 300;

	m_TxState = State::NoCarrier;

	m_TxWaveConfig = ma_waveform_config_init(m_Device.playback.format, m_Device.playback.channels, m_Device.sampleRate,
		ma_waveform_type_sine, 1.0, 0.0);

	ma_waveform_init(&m_TxWaveConfig, &m_TxWave);

	return true;
}


bool Modem_Bell103::Start(Role r)
{
	switch (r)
	{
		case Role::Originate:
			m_pRxMark = &m_RxMark_Answer;
			m_pRxSpace = &m_RxSpace_Answer;
			m_TxMark_Freq = BELL103_ORIGINATE_MARK;
			m_TxSpace_Freq = BELL103_ORIGINATE_SPACE;
			break;

		case Role::Answer:
			m_pRxMark = &m_RxMark_Originate;
			m_pRxSpace = &m_RxSpace_Originate;
			m_TxMark_Freq = BELL103_ANSWER_MARK;
			m_TxSpace_Freq = BELL103_ANSWER_SPACE;
			break;

		default:
			return false;
	}

	m_RxHistory.fill(0.0f);
	m_RxHistoryIdx = 0;
	m_RxHistoryCount = 0;
	m_RxSamplesSinceAnalysis = 0;
	m_RxDetectedTone = Tone::UNKNOWN;
	m_RxDecision = 0.0f;
	m_RxSamplesRemaining = 0;
	m_RxState = State::NoCarrier;
	m_RxBit = 0;
	m_RxByte = 0;

	constexpr float LeadInSeconds = 0.250f;
	m_TxFramesRemaining = static_cast<ma_uint32>(m_Device.sampleRate * LeadInSeconds);

	return __super::Start(r);
}


bool Modem_Bell103::Stop()
{
	if (!__super::Stop())
		return false;

	return true;
}


void Modem_Bell103::TxWaveform(float *poutput, uint32_t frame_count)
{
	while (frame_count > 0)
	{
		if (m_TxState == State::NoCarrier)
		{
			ma_silence_pcm_frames(poutput, frame_count, m_Device.playback.format, m_Device.playback.channels);
			return;
		}

		if (m_TxFramesRemaining == 0)
			AdvanceTxState();

		const ma_uint32 framesToGenerate = std::min<uint32_t>(frame_count, m_TxFramesRemaining);

		const double frequency = CurrentTxFrequency();

		ma_waveform_set_frequency(&m_TxWave, frequency);
		ma_waveform_read_pcm_frames(&m_TxWave, poutput, framesToGenerate, nullptr);

		poutput += framesToGenerate * m_Device.playback.channels;
		frame_count -= framesToGenerate;
		m_TxFramesRemaining -= framesToGenerate;
	}
}


void Modem_Bell103::AdvanceTxState()
{
	switch (m_TxState)
	{
		case State::NoCarrier:
			m_TxFramesRemaining = 0;
			return;

		case State::Idle:
		case State::StopBit:
		{
			uint8_t b = 0;

			if (m_TxBytes.Read(&b, 1) == 1)
			{
				m_TxByte = b;
				m_TxBit = 0;
				m_TxState = State::StartBit;
			}
			else
				m_TxState = State::Idle;

			break;
		}

		case State::StartBit:
			m_TxBit = 0;
			m_TxState = State::Data;
			break;

		case State::Data:
			m_TxBit++;
			if (m_TxBit >= 8)
				m_TxState = State::StopBit;

			break;
	}

	m_TxFramesRemaining = m_FramesPerBit;
}

double Modem_Bell103::CurrentTxFrequency() const
{
	switch (m_TxState)
	{
		case State::NoCarrier:
			return 0.0;

		case State::Idle:
		case State::StopBit:
			return m_TxMark_Freq;

		case State::StartBit:
			return m_TxSpace_Freq;

		case State::Data:
			return ((m_TxByte >> m_TxBit) & 1) ? m_TxMark_Freq : m_TxSpace_Freq;
	}

	return 0.0;
}


constexpr size_t AnalysisWindow = 96;
constexpr size_t AnalysisHop = 4;
constexpr float ToneThreshold = 0.20f;
constexpr float DecisionEpsilon = 1.0e-9f;


void Modem_Bell103::RxWaveform(const float* input, uint32_t frameCount)
{
	if (!input || (frameCount == 0))
		return;

	for (uint32_t sample_idx = 0; sample_idx < frameCount; sample_idx++)
	{
		m_RxHistory[m_RxHistoryIdx] = input[sample_idx];

		if (++m_RxHistoryIdx == m_RxHistory.size())
			m_RxHistoryIdx = 0;

		if (m_RxHistoryCount < m_RxHistory.size())
			m_RxHistoryCount++;

		// update the tone estimate every AnalysisHop samples once enough
		// history exists to fill an analysis window.

		m_RxSamplesSinceAnalysis++;
		if ((m_RxSamplesSinceAnalysis >= AnalysisHop) && (m_RxHistoryCount >= AnalysisWindow))
		{
			m_RxSamplesSinceAnalysis = 0;
			RxAnalyzeWindow();
		}

		// UART timing is sample-driven, independently of how frequently
		// the tone detector is evaluated

		RxAdvanceFraming();
	}
}


void Modem_Bell103::RxAnalyzeWindow()
{
	m_pRxMark->Reset();
	m_pRxSpace->Reset();

	const size_t historySize = m_RxHistory.size();

	// m_RxHistoryIdx points to the next position to be written, so this
	// locates the oldest sample in the newest AnalysisWindow samples

	const size_t start = (m_RxHistoryIdx + historySize - AnalysisWindow) % historySize;

	for (size_t i = 0; i < AnalysisWindow; ++i)
	{
		const float sample =
			m_RxHistory[(start + i) % historySize];

		m_pRxMark->AddSample(sample);
		m_pRxSpace->AddSample(sample);
	}

	const float mark_power = m_pRxMark->Power();
	const float space_power = m_pRxSpace->Power();
	const float total_power = mark_power + space_power;

	if (total_power < m_RxCarrierThreshold)
	{
		m_RxDecision = 0.0f;
		m_RxDetectedTone = Tone::UNKNOWN;
		m_RxState = State::NoCarrier;
		m_RxSamplesRemaining = 0;
		m_RxBit = 0;
		m_RxByte = 0;
		return;
	}

	m_RxDecision = (mark_power - space_power) /	(total_power + DecisionEpsilon);

	const Tone previous = m_RxDetectedTone;
	Tone detected = previous;

	// hysteresis: when neither tone wins clearly, retain the previous
	// result rather than rapidly switching around zero

	if (m_RxDecision > ToneThreshold)
		detected = Tone::MARK;
	else if (m_RxDecision < -ToneThreshold)
		detected = Tone::SPACE;

	m_RxDetectedTone = detected;

	switch (m_RxState)
	{
		case State::NoCarrier:
			 // this currently accepts one confident mark window

			if (detected == Tone::MARK)
				m_RxState = State::Idle;
			break;

		case State::Idle:
			if ((previous == Tone::MARK) && (detected == Tone::SPACE))
			{
				m_RxState = State::StartBit;

				// from the beginning of the start bit:
				// 0.5 bit -> center of start bit
				// 1.5 bit -> center of data bit zero

				m_RxSamplesRemaining = m_FramesPerBit / 2;
			}
			break;

		case State::StartBit:
		case State::Data:
		case State::StopBit:
			 // framing states advance from RxAdvanceFraming(), not from
			 // every overlapping analysis window
			break;
	}
}


void Modem_Bell103::RxAdvanceFraming()
{
	if ((m_RxState == State::Idle) || (m_RxState == State::NoCarrier))
		return;

	if (m_RxSamplesRemaining)
	{
		m_RxSamplesRemaining--;

		if (m_RxSamplesRemaining > 0)
			return;
	}

	switch (m_RxState)
	{
		case State::StartBit:

			// we are approximately at the center of the proposed
			// start bit... it must has to be SPACE!

			if (m_RxDecision >= 0.0f)
			{
				m_RxDetectedTone = Tone::MARK;
				m_RxState = State::Idle;
				m_RxSamplesRemaining = 0;
				return;
			}

			m_RxByte = 0;
			m_RxBit = 0;
			m_RxState = State::Data;
			m_RxSamplesRemaining = m_FramesPerBit;
			break;

		case State::Data:
			if (m_RxDecision > 0.0f)
				m_RxByte |=	(1 << m_RxBit);

			m_RxBit++;
			m_RxSamplesRemaining = m_FramesPerBit;

			if (m_RxBit == 8)
				m_RxState = State::StopBit;

			break;

		case State::StopBit:
			if (m_RxDecision > 0.0f)
				m_RxBytes.Write(&m_RxByte, 1);

			m_RxState = State::Idle;
			m_RxSamplesRemaining = 0;
			m_RxBit = 0;
			m_RxByte = 0;
			break;

		default:
			break;
	}
}
#include "pch.h"
#include "Modem_Base.h"
#include "miniaudio.h"


Modem::Modem() : m_RxBytes(4096, nullptr, nullptr), m_TxBytes(4096, nullptr, nullptr)
{
	m_DeviceConfig = ma_device_config_init(ma_device_type_duplex);

	m_DeviceConfig.playback.format		= ma_format_f32;
	m_DeviceConfig.playback.channels	= 1;

	m_DeviceConfig.capture.format		= ma_format_f32;
	m_DeviceConfig.capture.channels	= 1;

	m_DeviceConfig.sampleRate			= 48000;

	m_DeviceConfig.dataCallback			= (ma_device_data_proc)WaveformCallback;

	m_DeviceConfig.pUserData			= this;

	m_Role = Role::Off;
}

Modem::~Modem()
{
	ma_device_uninit(&m_Device);
}


bool Modem::Initialize()
{
	if (ma_device_init(nullptr, &m_DeviceConfig, &m_Device) != MA_SUCCESS)
		return false;

	return true;
}


void Modem::Release()
{
	Stop();
	delete this;
}


bool Modem::Start(Role r)
{
	if (m_Role != Role::Off)
		return false;

	if (ma_device_start(&m_Device) != MA_SUCCESS)
		return false;

	m_Role = r;

	//m_TxState = State::Idle;

	return true;
}


bool Modem::Stop()
{
	if (m_Role == Role::Off)
		return false;

	m_Role = Role::Off;

	ma_device_stop(&m_Device);

	m_TxState = State::NoCarrier;
	m_RxState = State::NoCarrier;

	return true;
}

bool Modem::Send(const uint8_t *buffer, size_t buffer_size)
{
	// the user may want to send more than our ring buffer can take if not enough have
	// been sent by the modem yet... so we wait until it's possible
	while (buffer_size)
	{
		if (m_TxState == State::NoCarrier)
			return false;

		size_t tx = m_TxBytes.Write(buffer, buffer_size);
		if (tx >= buffer_size)
			return true;

		buffer_size -= tx;
		buffer += tx;

		::Sleep(0);
	}

	return true;
}

bool Modem::Receive(uint8_t *buffer, size_t buffer_size, size_t expected, bool block)
{
	expected = std::min<size_t>(expected, buffer_size);

	// the user may want to send more than our ring buffer can take if not enough have
	// been sent by the modem yet... so we wait until it's possible
	while (expected)
	{
		if (m_RxState == State::NoCarrier)
			return false;

		size_t rx = m_RxBytes.Read(buffer, expected);
		if (!block && !rx)
			return false;

		expected -= rx;
		buffer += rx;

		if (rx < expected)
			::Sleep(0);
	}

	return true;
}

void Modem::WaveformCallback(ma_device *pDevice, float *pOutput, const float *pInput, ma_uint32 frameCount)
{
	Modem *_this = (Modem *)(pDevice->pUserData);

	_this->TxWaveform(pOutput, frameCount);
	_this->RxWaveform(pInput, frameCount);
}

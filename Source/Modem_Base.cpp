#include "pch.h"
#include "Modem_Base.h"
#include "miniaudio.h"


Modem::Modem(OutputDeviceIdx oidx, InputDeviceIdx iidx) : m_RxBytes(4096, nullptr, nullptr), m_TxBytes(4096, nullptr, nullptr)
{
	ma_context context;
	ma_context_init(nullptr, 0, nullptr, &context);

	m_DeviceConfig = ma_device_config_init(ma_device_type_duplex);

	ma_device_info *playback_devices = nullptr;
	ma_uint32 playback_count = 0;

	ma_device_info *capture_devices = nullptr;
	ma_uint32 capture_count = 0;

	ma_context_get_devices(&context, &playback_devices, &playback_count, &capture_devices, &capture_count);
		
	if ((oidx != -1) && (oidx < playback_count))
		m_DeviceConfig.playback.pDeviceID = &playback_devices[oidx].id;
	else
		m_DeviceConfig.playback.pDeviceID = nullptr;

	m_DeviceConfig.playback.format		= ma_format_f32;
	m_DeviceConfig.playback.channels	= 1;

	if ((oidx != -1) && (oidx < playback_count))
		m_DeviceConfig.capture.pDeviceID = &capture_devices[oidx].id;
	else
		m_DeviceConfig.capture.pDeviceID = nullptr;

	m_DeviceConfig.capture.format		= ma_format_f32;
	m_DeviceConfig.capture.channels	= 1;

	m_DeviceConfig.sampleRate			= 48000;

	m_DeviceConfig.dataCallback			= (ma_device_data_proc)WaveformCallback;

	m_DeviceConfig.pUserData			= this;

	m_Role = Role::Originate;
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
	if (ma_device_start(&m_Device) != MA_SUCCESS)
		return false;

	m_Role = r;

	return true;
}


bool Modem::Stop()
{
	ma_device_stop(&m_Device);

	m_TxState = State::NoCarrier;
	m_RxState = State::NoCarrier;

	return true;
}


size_t Modem::Send(const uint8_t *buffer, size_t buffer_size)
{
	size_t tx = 0;

	// the user may want to send more than our ring buffer can take if not enough have
	// been sent by the modem yet... so we wait until it's possible
	while (tx < buffer_size)
	{
		if (m_TxState == State::NoCarrier)
			return 0;

		tx += m_TxBytes.Write(&buffer[tx], buffer_size - tx);
		if (tx >= buffer_size)
			return tx;

		::Sleep(0);
	}

	return true;
}


size_t Modem::Receive(uint8_t *buffer, size_t buffer_size, size_t expected, ReceiveBlockFunc block_func)
{
	expected = std::min<size_t>(expected, buffer_size);

	size_t rx = 0;

	while (rx < expected)
	{
		if (m_RxState == State::NoCarrier)
			return 0;

		if (!block_func)
		{
			return m_RxBytes.Read(buffer, expected);
		}
		else
		{
			rx += m_RxBytes.Read(&buffer[rx], 1);

			switch (block_func(buffer[rx]))
			{
				case ReceiveBlockReturn::EndBlock:
					return rx;

				case ReceiveBlockReturn::DiscardBlock:
					return 0;
			}
		}

		if (rx < expected)
			::Sleep(0);
	}

	return rx;
}


void Modem::WaveformCallback(ma_device *pDevice, float *pOutput, const float *pInput, ma_uint32 frameCount)
{
	Modem *_this = (Modem *)(pDevice->pUserData);

	_this->TxWaveform(pOutput, frameCount);
	_this->RxWaveform(pInput, frameCount);
}


bool Modem::Online()
{
	return (m_RxState != State::NoCarrier);
}

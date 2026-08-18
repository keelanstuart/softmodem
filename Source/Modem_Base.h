#include "RingBuffer.h"
#include "miniaudio.h"
#include <softmodem.h>

#pragma once

class Modem : public IModem
{

public:

	using State = enum
	{
		NoCarrier = 0,
		Idle,
		StartBit,
		Data,
		StopBit
	};

	Modem(OutputDeviceIdx oidx, InputDeviceIdx iidx);
	virtual ~Modem();

	virtual void Release();
	virtual bool Initialize();
	virtual bool Start(Role r);
	virtual bool Stop();
	virtual size_t Send(const uint8_t *buffer, size_t buffer_size);
	virtual size_t Receive(uint8_t *buffer, size_t buffer_size, size_t expected, ReceiveBlockFunc block_func);
	virtual bool Online();


protected:

	static void WaveformCallback(ma_device* pDevice, float *pOutput, const float *pInput, ma_uint32 frameCount);

	virtual void TxWaveform(float *poutput, uint32_t frame_count) = 0;
	virtual void RxWaveform(const float *pinput, uint32_t frame_count) = 0;

	Role m_Role;

	State m_RxState, m_TxState;
	RingBuffer m_RxBytes, m_TxBytes;
	uint8_t m_TxByte, m_RxByte;
	uint32_t m_TxBit, m_RxBit;

	ma_device_config m_DeviceConfig;
	ma_device m_Device;

};

// softmodem - a modem that modulates and demodulates data through your sound card
// copyright 2026 (c), Keelan Stuart

#include <cstdint>

#pragma once

#ifdef SOFTMODEM_EXPORTS
#define SOFTMODEM_API __declspec(dllexport)
#else
#define SOFTMODEM_API __declspec(dllimport)
#endif

class IModem
{

public:

	using ModemStandard = enum
	{
		Bell103			// USA 300bps 8N1
	};

	using Role = enum
	{
		Off,
		Originate,
		Answer
	};


	// creates a new modem of the type you specify
	SOFTMODEM_API static IModem *Create(ModemStandard m);

	// releases all resources owned by the modem
	virtual void Release() = 0;

	// starts the modem in the role you specify, either Originate (the caller) or Answer (the callee)
	virtual bool Start(Role r) = 0;

	// stops the modem
	virtual bool Stop() = 0;

	// sends data, blocks unless carrier is lost, returns true if data was sent, false if carrier lost
	virtual bool Send(const uint8_t *buffer, size_t buffer_size) = 0;

	// receives data, blocks unless carrier is lost, returns true if the amount of data
	// expected was received, false if carrier lost
	virtual bool Receive(uint8_t *buffer, size_t buffer_size, size_t expected, bool block) = 0;

};

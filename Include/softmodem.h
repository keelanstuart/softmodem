// softmodem - a modem that modulates and demodulates data through your sound card
// copyright 2026 (c), Keelan Stuart

#include <cstdint>
#include <functional>

#pragma once

#ifdef SOFTMODEM_EXPORTS
#define SOFTMODEM_API __declspec(dllexport)
#else
#define SOFTMODEM_API __declspec(dllimport)
#endif

class IModem
{

public:

	#define DEFAULT_DEVICE	-1

	using OutputDeviceIdx	= uint32_t;
	using InputDeviceIdx	= uint32_t;

	using ModemStandard = enum
	{
		Bell103,			// USA 300bps 8N1
	};

	using Role = enum
	{
		Originate,
		Answer,

		Channel_0 = Originate,
		Channel_1 = Answer,
		Channel_2,
		Channel_3,
	};

	using ReceiveBlockReturn = enum
	{
		EndBlock,				// end the block and return
		ContinueBlock,			// keep on receiving this block
		DiscardBlock			// discard everything in this block
	};

	using ReceiveBlockFunc = std::function<ReceiveBlockReturn(uint8_t)>;

	// ReciveBlockFunc's are supplied to Receive. You might use it to capture VT/ANSI escape sequences with something like:
	/*
	*
	* uint8_t rb[128]; // no escape sequences longer than 128 bytes! (?)
	*
	* if (Receive(rb, 1, 1) == 1)
	* {
	*	// was an escape received?
	*	if (rb[0] == 0x1B)
	*	{
	*		// yes, so block until we get the whole sequence
	*		Receive(&rb[1], 127, 127, [](received_char) =>
	*		{
	*			// numbers and semi-colons are strung together
	*			if (isdigit(received_char) || (received_char == ';'))
	*				return ReceiveBlockReturn::ContinueBlock;
	*
	*			switch (received_char)
	*			{
	*				case 'm': return ReceiveBlockReturn::EndBlock;	// color
	*				case 'A': return ReceiveBlockReturn::EndBlock;	// cursor up
	*				case 'B': return ReceiveBlockReturn::EndBlock;	// cursor down
	*				case 'C': return ReceiveBlockReturn::EndBlock;	// cursor forward
	*				case 'D': return ReceiveBlockReturn::EndBlock;	// cursor back
	*
	*				// etc... whatever you handle
	*			}
	*
	*			return ReceiveBlockReturn::DiscardBlock; // we didn't recognize it, so it's ignored
	*		});
	*	}
	*	else
	*	{
	*		// handle normal character
	*	}
	* }
	*
	*/


	// Creates a new modem of the type you specify
	SOFTMODEM_API static IModem *Create(ModemStandard m, OutputDeviceIdx oidx = DEFAULT_DEVICE, InputDeviceIdx iidx = DEFAULT_DEVICE);

	// Releases all resources owned by the modem
	virtual void Release() = 0;

	// Starts the modem in the role you specify, either Originate (the caller) or Answer (the callee)
	virtual bool Start(Role r) = 0;

	// Stops the modem
	virtual bool Stop() = 0;

	// Sends data, returns the number of bytes sent
	virtual size_t Send(const uint8_t *buffer, size_t buffer_size) = 0;

	// Receives data, returns the number of bytes received, can take a function that can control block receives
	virtual size_t Receive(uint8_t *buffer, size_t buffer_size, size_t expected, ReceiveBlockFunc block_func = nullptr) = 0;

	// Returns true if we're online!
	virtual bool Online() = 0;

};

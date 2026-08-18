// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <softmodem.h>

#include "Modem_Base.h"
#include "Modem_Bell103.h"

// generate the miniaudio code here
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

IModem *IModem::Create(IModem::ModemStandard m, OutputDeviceIdx oidx, InputDeviceIdx iidx)
{
	IModem *ret = nullptr;

	switch (m)
	{
		case IModem::ModemStandard::Bell103:
			ret = new Modem_Bell103(oidx, iidx);
			break;
	}

	if (ret)
	{
		((Modem *)ret)->Initialize();
	}

	return ret;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}


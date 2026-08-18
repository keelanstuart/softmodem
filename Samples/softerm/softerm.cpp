// softerm.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <conio.h>
#include <Windows.h>
#include <softmodem.h>
#include <vadefs.h>
#include <windows.h>
#include <string>

uint64_t GetMachineGuid()
{
	char guid[256] = {};
	DWORD size = sizeof(guid);

	LSTATUS result = RegGetValueA(
		HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Cryptography",
		"MachineGuid",
		RRF_RT_REG_SZ,
		nullptr,
		guid,
		&size);

	uint64_t hash = 14695981039346656037ULL;
	char *g = guid;

	while (*g)
	{
		hash ^= (uint8_t)*g++;
		hash *= 1099511628211ULL;
	}

	return hash;
}


void SendString(IModem *pmodem, bool echo, const char *s, ...)
{
	if (!pmodem || !s)
		return;

#define PRINT_BUFSIZE	1024
	char buf[PRINT_BUFSIZE];	// Temporary buffer for output

	va_list marker;
	va_start(marker, s);
	vsnprintf_s(buf, PRINT_BUFSIZE - sizeof(TCHAR), s, marker);

	if (echo)
		printf(buf);

	s = buf;
	while (*s)
	{
		pmodem->Send((uint8_t *)s, 1);
		s++;
	}
}


void SendIcon(IModem *pmodem, uint64_t icon)
{
	SendString(pmodem, false, "\n\x1B[96cm");

	for (int j = 0; j < sizeof(uint64_t); j++)
	{
		uint8_t c = *((char *)&icon + j);
		uint8_t m = 0x80;

		for (int i = 0; i < 8; i++)
		{
			SendString(pmodem, false, (c & m) ? "##" : "  ");
			m >>= 1;
		}

		m = 0x02;

		for (int i = 1; i < 8; i++)
		{
			SendString(pmodem, false, (c & m) ? "##" : "  ");
			m <<= 1;
		}

		SendString(pmodem, false, "\n");
	}

	SendString(pmodem, false, "\n");
}



#if 0

#define MODE Answer

#else

#define MODE Originate

#endif


int main()
{
	IModem *pmodem = IModem::Create(IModem::ModemStandard::Bell103);

	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);

	DWORD mode;
	GetConsoleMode(hout, &mode);
	SetConsoleMode(hout, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	bool sent_icon = false;

	if (pmodem->Start(IModem::Role::MODE))
	{
		bool run = true;
		while (run)
		{
			uint8_t b;
			if (pmodem->Receive(&b, 1, 1) == 1)
			{
				printf("\x1B[92m");
				_putch(b);
			}

			if (!sent_icon && pmodem->Online())
			{
				Sleep(1000);
				SendIcon(pmodem, GetMachineGuid());
				sent_icon = true;
			}

			if (_kbhit())
			{
				int k = _getch();

				// special keys
				if ((k == 0) || (k == 0xE0))
				{
					int kk = _getch();

					switch (kk)
					{
						case 0x86:	//F12
							run = false;
							break;
					}
				}
				else
				{
					pmodem->Send((uint8_t *)(&k), 1);

					printf("\x1B[90m");
					_putch(k);
				}
			}
		}

		pmodem->Stop();
		pmodem->Release();
	}

	return 0;
}

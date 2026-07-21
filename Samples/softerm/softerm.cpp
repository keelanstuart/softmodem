// softerm.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <conio.h>
#include <Windows.h>
#include <softmodem.h>

void SendString(IModem *pmodem, const char *s)
{
	if (!pmodem)
		return;

	while (s && *s)
	{
		pmodem->Send((uint8_t *)s, 1);
		s++;
	}
}

int main()
{
	IModem *pmodem = IModem::Create(IModem::ModemStandard::Bell103);

	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);

	DWORD mode;
	GetConsoleMode(hout, &mode);
	SetConsoleMode(hout, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	if (pmodem->Start(IModem::Role::Originate))
	{
		bool run = true;
		while (run)
		{
			uint8_t b;
			if (pmodem->Receive(&b, 1, 1, false))
				_putch(b);

#if 0
			for (int i = 0; i < 1000; i++)
			{
				SendString(pmodem, "This is a message. Can you hear it?\r\n");
				::Sleep(100);
			}
#endif

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
					_putch(k);
				}
			}
		}

		pmodem->Stop();
		pmodem->Release();
	}

	return 0;
}

#pragma once
#include <TCHAR.h>

namespace Message
{
	static void ConvertToUTF16(const char* str, LPTSTR* buf) {
		if (!IsWindowUnicode(GetForegroundWindow())) { return; }

		INT Length = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
		(*buf) = (TCHAR*)malloc(sizeof(TCHAR) * Length + 1);
		MultiByteToWideChar(CP_ACP, 0, str, -1, *buf, Length);
	}

	static void Trace(LPCTSTR format, ...) {
		TCHAR Buffer[0x400];

		va_list Mark;
		va_start(Mark, format);

		_vstprintf_s(Buffer, sizeof(Buffer) / sizeof(Buffer[0]), format, Mark);
		lstrcat(Buffer, L"\r\n");

		OutputDebugString(Buffer);

		va_end(Mark);
	}

	static void Err_Display(LPCTSTR format) {
		LPVOID lpMsgBuf;

		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0,
			NULL
		);

		TCHAR Temp[0x400];
		wsprintf(Temp, TEXT("[%s] %s\r\n"), format, (LPTSTR)lpMsgBuf);

		OutputDebugString(Temp);
		LocalFree(lpMsgBuf);
	}

	static void Err_Quit(LPCTSTR format) {
		LPVOID lpMsgBuf;

		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0,
			NULL
		);

		TCHAR Temp[0x400];
		wsprintf(Temp, TEXT("[%s] %s\r\n"), format, (LPTSTR)lpMsgBuf);
		OutputDebugString((LPTSTR)lpMsgBuf);

		LocalFree(lpMsgBuf);
		exit(EXIT_FAILURE);
	}
};
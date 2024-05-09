#include "pch.h"

#define SERVERPORT 9000

DWORD WINAPI ReadThread(LPVOID lpArg);
DWORD WINAPI WriteThread(LPVOID lpArg);
SOCKET client_sock;

char Sendbuffer[0x400] = {}, RecvBuffer[0x400] = {};
HANDLE hWriteEvent, hReadEvent;

int main()
{
	SocketTool::Initialize();

	hReadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	hWriteEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

	client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_sock == INVALID_SOCKET) { Message::Err_Quit(TEXT("socket() error")); }

	struct sockaddr_in ServerInfo = { 0 };
	// ServerInfo.sin_addr.s_addr = htonl(INADDR_ANY);
	ServerInfo.sin_port = htons(SERVERPORT);
	ServerInfo.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &ServerInfo.sin_addr);

	INT Result = connect(client_sock, (struct sockaddr*)&ServerInfo, sizeof(ServerInfo));
	if (Result == SOCKET_ERROR) { Message::Err_Quit(TEXT("connect() error")); }

	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL, 0, ReadThread, NULL, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, WriteThread, NULL, 0, NULL);

	if (hThread[0] == NULL || hThread[1] == NULL) {
		Message::Err_Quit(TEXT("작업 스레드 생성 실패"));
	}

	Result = WaitForMultipleObjects(2, hThread, FALSE, INFINITE);

	Result -= WAIT_OBJECT_0;
	if (Result == 0) {
		TerminateThread(hThread[1], 1);
	}
	else {
		TerminateThread(hThread[0], 1);
	}

	// 둘 중 하나라도 종료되면 접속 끊긴 상태
	CloseHandle(hThread[0]); CloseHandle(hThread[1]);
	Message::Trace(TEXT("접속을 종료하였습니다."));

	closesocket(client_sock);
	SocketTool::Clear();
}

DWORD WINAPI ReadThread(LPVOID lpArg) {
	INT Result;
	
	while (1) {
		WaitForSingleObject(hReadEvent, INFINITE);

		Result = recv(client_sock, RecvBuffer, 0x400, 0);
		if (Result == 0 || Result == SOCKET_ERROR) { 
			if (WSAGetLastError() == WSAEWOULDBLOCK) { continue; }
			Message::Err_Display(TEXT("recv error\n")); break;
		}

		RecvBuffer[Result] = 0;
		printf("[Recv Data] %s\r\n", RecvBuffer);

		SetEvent(hWriteEvent);
	}

	return 0;
}

DWORD WINAPI WriteThread(LPVOID lpArg) {
	INT Result;

	while (1) {
		WaitForSingleObject(hWriteEvent, INFINITE);

		printf("[Input] : ");
		if (fgets(Sendbuffer, 0x400 + 1, stdin) == NULL) { break; }

		INT length = strlen(Sendbuffer);
		if (Sendbuffer[length - 1] == '\n') { Sendbuffer[length - 1] = 0; }
		if (strlen(Sendbuffer) == 0) { break; }

		Result = send(client_sock, Sendbuffer, strlen(Sendbuffer), 0);
		if (Result == 0 || Result == SOCKET_ERROR) { 
			if (WSAGetLastError() == WSAEWOULDBLOCK) { continue; }
			Message::Err_Display(TEXT("send error\n")); break;
		}
		printf("[TCP Client] %d Bytes Sending.\n", Result);

		SetEvent(hReadEvent);
	}

	return 0;
}
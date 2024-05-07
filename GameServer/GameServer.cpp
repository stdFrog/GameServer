#include "pch.h"
#include <iostream>
#include <thread>
#include <vector>
#include <windows.h>
#include <atomic>
#include <mutex>
#include <map>
#include <queue>
#include "ThreadManager.h"

/*
	서버를 제작할 때 주의사항

	처음보는 함수가 등장할 때 전체적인 구조만 파악하면 된다.
	자세한 내용은 디버깅할 때만 필요하고 해당 함수가 어떤 부분을 담당하는지 당장 필요한 내용만 파악한다.

	단계별로 살펴보자.
	1) 새로운 소켓을 생성한다.
	2) 소켓에 로컬 주소와 프로세스간 통신에 필요한 포트 번호를 설정한다.
	3) 소켓을 열린 상태로 만든다.
	4) 소켓으로 접속 요청이 오면 해당 요청으로부터 통신 전용 소켓을 만든다.
	5) 통신 전용 함수를 호출한다. 이때 비동기 통신이 가능한 WSARecv 등을 활용한다.
*/
#define SERVERPORT 9000

DWORD WINAPI Thread(LPVOID lpArg);

struct Session {
	OVERLAPPED ov;
	SOCKET sock;
	CHAR buf[0x400];
	WSABUF wsabuf;
	int rb;
	int sb;
};

int main()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { return -1; };

	HANDLE hComplete = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hComplete == NULL) { Message::Err_Quit(TEXT("CreateIoCompletionPort() error")); }

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	for (DWORD i = 0; i < si.dwNumberOfProcessors * 2; i++) {
		DWORD dwThread;
		HANDLE hThread = CreateThread(NULL, 0, Thread, hComplete, 0, &dwThread);
		if (hThread == NULL) { Message::Err_Quit(TEXT("CreateThread() error")); }
		CloseHandle(hThread);
	}

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock == INVALID_SOCKET) { Message::Err_Quit(TEXT("socket() error")); }

	struct sockaddr_in ServerInfo = { 0 };
	ServerInfo.sin_addr.s_addr = htonl(INADDR_ANY);
	ServerInfo.sin_port = htons(SERVERPORT);
	ServerInfo.sin_family = AF_INET;

	INT Result = bind(listen_sock, (struct sockaddr*)&ServerInfo, sizeof(ServerInfo));
	if (Result == SOCKET_ERROR) { Message::Err_Quit(TEXT("bind() error")); }

	Result = listen(listen_sock, SOMAXCONN);
	if (Result == SOCKET_ERROR) { Message::Err_Quit(TEXT("listen() error")); }

	INT cbAddr;
	struct sockaddr_in ClientInfo;
	SOCKET client_sock;

	while (1) {
		cbAddr = sizeof(ClientInfo);
		client_sock = accept(listen_sock, (struct sockaddr*)&ClientInfo, &cbAddr);
		if (client_sock == INVALID_SOCKET) { Message::Err_Display(TEXT("accept() error")); break; }

		char IPAddress[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &ClientInfo.sin_addr, IPAddress, sizeof(IPAddress));
		
		// char Buffer[0x400];
		// Message::ConvertToUTF16(IPAddress, &Buffer);
		// Message::Trace(TEXT("Client Connected!\r\n[Address] : %s"), Buffer);
		printf("Client Connected!\r\n[Address] : %s\r\n", IPAddress);

		// TODO
		{
			CreateIoCompletionPort((HANDLE)client_sock, hComplete, client_sock, 0);

			Session* NewSession = new Session;
			if (NewSession == NULL) { break; }

			memset(&NewSession->ov, 0, sizeof(NewSession->ov));
			NewSession->sock = client_sock;
			NewSession->rb = NewSession->sb = 0;
			NewSession->wsabuf.buf = NewSession->buf;
			NewSession->wsabuf.len = 0x400;

			DWORD recvbytes, flags;
			flags = 0;
			Result = WSARecv(client_sock, &NewSession->wsabuf, 1, &recvbytes, &flags, &NewSession->ov, NULL);
			if (Result == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					Message::Err_Display(TEXT("Error : WSARecv()"));
				}
				continue;
			}
		}
	}

	closesocket(listen_sock);
	WSACleanup();
}

DWORD WINAPI Thread(LPVOID lpArg) {
	BOOL Result;

	HANDLE hComplete = (HANDLE)lpArg;
	while (1) {
		DWORD dwTrans;
		SOCKET client_sock;
		Session* NewSession;

		Result = GetQueuedCompletionStatus(
			hComplete,
			&dwTrans,
			(PULONG_PTR)&client_sock,
			(LPOVERLAPPED*)&NewSession,
			INFINITE);

		struct sockaddr_in ClientInfo;
		int Size = sizeof(ClientInfo);
		getpeername(NewSession->sock, (struct sockaddr*)&ClientInfo, &Size);

		char IPAddress[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &ClientInfo.sin_addr, IPAddress, sizeof(IPAddress));

		if (Result == FALSE || dwTrans == 0) {
			printf("[TCP Server] Client Exit : IP Addr = %s, Port = %d\r\n", IPAddress, ntohs(ClientInfo.sin_port));
			closesocket(NewSession->sock);
			delete NewSession;
			continue;
		}

		if (NewSession->rb == 0) {
			NewSession->rb = dwTrans;
			NewSession->sb = 0;

			NewSession->buf[NewSession->rb] = 0;
			printf("[TCP %s : %d] : %s\n", IPAddress, ntohs(ClientInfo.sin_port), NewSession->buf);
		}
		else {
			NewSession->sb += dwTrans;
		}

		if(NewSession->rb > NewSession->sb){
			memset(&NewSession->ov, 0, sizeof(NewSession->ov));
			NewSession->wsabuf.buf = NewSession->buf + NewSession->sb;
			NewSession->wsabuf.len = NewSession->rb - NewSession->sb;

			DWORD dwSendBytes;
			Result = WSASend(NewSession->sock, &NewSession->wsabuf, 1, &dwSendBytes, 0, &NewSession->ov, NULL);
			if (Result == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					Message::Err_Display(TEXT("Error : WSASend()"));
				}
				continue;
			}
		}
		else {
			NewSession->rb = 0;
			memset(&NewSession->ov, 0, sizeof(NewSession->ov));
			NewSession->wsabuf.buf = NewSession->buf;
			NewSession->wsabuf.len = 0x400;

			DWORD dwRecvBytes;
			DWORD flags = 0;

			Result = WSARecv(NewSession->sock, &NewSession->wsabuf, 1, &dwRecvBytes, &flags, &NewSession->ov, NULL);
			if (Result == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					Message::Err_Display(TEXT("Error : WSARecv()"));
				}
				continue;
			}
		}
	}

	return 0;
}
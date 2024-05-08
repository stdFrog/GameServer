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
	SocketTool::Initialize();

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

	/* 
		넌블로킹 소켓은 교착 상태가 생기지 않는다는 장점이 있다.
		또, 멀티스레드 환경이 아니어도 여러 소켓에 대해 입출력 처리가 가능하다.
		곧, 필요할 때마다 소켓과 직접 관계가 없는 다른 작업이 가능하다는 것이다.

		단, 예외 분기가 늘어나며, CPU의 사용률이 항상 최고치이다.
	*/
	ULONG ON = 1;
	INT Result = ioctlsocket(listen_sock, FIONBIO, &ON);
	if (Result == SOCKET_ERROR) { Message::Err_Quit(TEXT("ioctlsocket() error")); }

	SocketTool::SetReuseAddress(listen_sock, TRUE);
	if (SocketTool::BindAnyAddress(listen_sock, 9000) == FALSE) {
		Message::Err_Quit(TEXT("bind() error"));
	}

	if (SocketTool::Listen(listen_sock) == FALSE) {
		Message::Err_Quit(TEXT("listen() error"));
	}

	INT cbAddr;
	struct sockaddr_in ClientInfo;
	SOCKET client_sock;

	while (1) {
		cbAddr = sizeof(ClientInfo);
		client_sock = accept(listen_sock, (struct sockaddr*)&ClientInfo, &cbAddr);
		if (client_sock == INVALID_SOCKET) {
			if (WSAGetLastError() == WSAEWOULDBLOCK) { continue; }
			Message::Err_Display(TEXT("accept() error")); break;
		}

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
	SocketTool::Clear();
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
#include "pch.h"
#include "ThreadManager.h"
#include "SocketTool.h"
#include "Listener.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"

#define SERVERIP TEXT("127.0.0.1")
#define SERVERPORT 9000

using std::this_thread::sleep_for;

/*
DWORD WINAPI Thread(LPVOID lpArg);

struct Session {
	OVERLAPPED ov;
	SOCKET sock;
	CHAR buf[0x400];
	WSABUF wsabuf;
	int rb;
	int sb;
};

enum IO_TYPE {
	READ,WRITE,ACCEPT,CONNECT
};

struct OVERLAPPEDEX {
	WSAOVERLAPPED ov = {};
	INT Type = 0;

};
*/

/*
class GameSession : public Session {
public:
		소켓 정보외에 추가하고 싶은 정보를 기입하면 된다.
		
		서버측에서 관리할 정보와 협업시 필요한 공용 정보 따위가 이에 해당된다.
	~GameSession() {
		std::cout << "~GameSession()" << std::endl;
	}

	virtual INT OnRecv(PBYTE Buffer, INT Length) {
		std::cout << "OnRecv Length = " << Length << std::endl;
		Send(Buffer, Length);
		return Length;
	}

	virtual void OnSend(INT Length) {
		std::cout << "OnSend Length = " << Length << std::endl;
	}
};
*/

int main()
{
	SocketTool::Initialize();

	/*
		SocketTool::SetReuseAddress(listen_sock, TRUE);
		if (SocketTool::BindAnyAddress(listen_sock, 9000) == FALSE) {
			Message::Err_Quit(TEXT("bind() error"));
		}

		if (SocketTool::Listen(listen_sock) == FALSE) {
			Message::Err_Quit(TEXT("listen() error"));
		}

		cbAddr = sizeof(ClientInfo);
		client_sock = accept(listen_sock, (struct sockaddr*)&ClientInfo, &cbAddr);
		if (client_sock == INVALID_SOCKET) {
			// if (WSAGetLastError() == WSAEWOULDBLOCK) { continue; }
			Message::Err_Display(TEXT("accept() error")); break;
		}

		char IPAddress[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &ClientInfo.sin_addr, IPAddress, sizeof(IPAddress));

		// char Buffer[0x400];
		// Message::ConvertToUTF16(IPAddress, &Buffer);
		// Message::Trace(TEXT("Client Connected!\r\n[Address] : %s"), Buffer);
		printf("Client Connected!\r\n[Address] : %s\r\n", IPAddress);

		CreateIoCompletionPort((HANDLE)client_sock, hComplete, client_sock, 0);

		Session* NewSession = new Session;
		if (NewSession == NULL) { break; }

		memset(&NewSession->ov, 0, sizeof(NewSession->ov));
		NewSession->sock = client_sock;
		NewSession->rb = NewSession->sb = 0;
		NewSession->wsabuf.buf = NewSession->buf;
		NewSession->wsabuf.len = 0x400;

		위 일련의 과정이 아래 StartAccept 함수로 대체되었다.
		또한, 비동기 소켓 함수 AcceptEx를 사용하기 때문에 연결 요청이 없어도 즉시 반환한다.

		이때 WSA_IO_PENDING 따위의 에러 코드를 확인하여야 한다.
	*/
	/*Listener listner;
	listner.StartAccept(NetAddress(SERVERIP, SERVERPORT));*/

	/* 서비스 클래스 제작 후 */
	std::shared_ptr<ServerService> NewService = std::make_shared<ServerService>(
		NetAddress(SERVERIP, SERVERPORT),
		std::make_shared<IOCPCore>(),
		[]() { return std::make_shared<GameSession>(); },
		100
	);

	/* 
		엔진측에서 모든 내부적 처리를 끝내므로 서버측에선 서비스 생성 후 Start 함수만 호출하면 된다.

		아래 Start 함수가 Listener 생성 및 RegisterAccept까지의 과정을 한번에 처리한다.
	*/
	assert(NewService->Start());

	/*
		연결 요청 대기 함수 곧, AcceptEx까지 위에서 미리 끝냈다.

		다음은 작업자 스레드를 생성하여 대기시켜 놓으면 된다.
	*/
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	for (DWORD i = 0; i < 5/*si.dwNumberOfProcessors * 2*/; i++) {
		GThreadManager->Launch([=]() {
			while (1) {
				NewService->GetMainCore()->Dispatch();
			}
		});
	}

	/* 컨텐츠(AI 등) 작업 추가 */
	while (true) {
		std::vector<BufferData> Buffers{ BufferData{ 100, 1.5f }, BufferData{ 200, 2.3f }, BufferData{ 300, 0.7f } };
		std::shared_ptr<SendBuffer> SendBuf = ServerPacketHandler::Make_S_TEST(1001, 100, 10, Buffers);
		GSessionManager.Broadcast(SendBuf);

		sleep_for(std::chrono::milliseconds(250));
	}

	GThreadManager->Join();

	/*
		넌블로킹 소켓은 교착 상태가 생기지 않는다는 장점이 있다.
		또, 멀티스레드 환경이 아니어도 여러 소켓에 대해 입출력 처리가 가능하다.
		곧, 필요할 때마다 소켓과 직접 관계가 없는 다른 작업이 가능하다는 것이다.

		단, 예외 분기가 늘어나며, CPU의 사용률이 항상 최고치이다.

		소켓은 소켓 함수 호출 시 동작하는 방식에 따라 블로킹과 넌블로킹 소켓으로 구분한다.
		블로킹 소켓은 함수 호출 시 조건을 만족하지 않으면 함수가 리턴하지 않으며,
		넌블로킹 소켓은 함수 호출 시 조건을 만족하지 않더라도 함수가 리턴한다.

		단, 함수의 동작이 정상적으로 이루어졌다는 것을 뜻하는게 아니므로, 오류값을 리턴한다.
		따라서 WSAGetLastError 함수를 이용하여 오류 코드를 점검해야 한다.

		참고로, Completion port 모델이 사용하는 소켓 모드는 알려져 있지 않다.

		Completion Port 모델은 동기 입출력이 아닌 비동기 입출력 함수를 사용하기 때문에
		넌블로킹이건, 블로킹이건 구분할 필요가 전혀 없다.

		응용프로그램 수준에선 비동기 입출력이 시작된 시점과, 끝난 시점에 대한 분기 처리에만 신경쓰도록 되어 있다.

		곧, 내부적으로 넌블로킹 소켓을 사용하는지 블로킹 소켓을 사용하는지 전혀 구분할 필요가 없다.
		
		단, 비동기 통지 방식을 사용하는 다른 모델들과 달리, 작업자 스레드를 따로 필요로 한다는 점에서
		블로킹 소켓으로 인한 교착 상태를 방지하기 위한 것이 아닐까 추측해 볼 수 있다.

		Completion Port 모델에서는 블로킹과 넌블로킹의 구분보다도, 대기 함수와 운영체제의 동작에 더 신경써야 한다.
	*/

	/*
	while (1) {
		

		// TODO
		{
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
	*/

	SocketTool::Clear();
}

/*
DWORD WINAPI Thread(LPVOID lpArg) {
	BOOL Result;

	HANDLE hComplete = (HANDLE)lpArg;
	while (1) {
		DWORD dwTrans;
		SOCKET client_sock;
		Session* NewSession;

		// 유효한 포인터가 아닐 수 있으므로 점검 필요 | 스마트 포인터 사용해도 좋음
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
*/
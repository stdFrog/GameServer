#include "pch.h"
#include "Listener.h"
#include "SocketTool.h"
#include "IOCPEvent.h"
#include "Session.h"
#include "Service.h"

Listener::Listener() {
	
}

Listener::~Listener() {
	CloseSocket();

	for (IOCPEvent* E : _AcceptEvents) {
		delete E;
	}
}

HANDLE Listener::GetHandle() {
	return (HANDLE)_Socket;
}

void Listener::CloseSocket() {
	SocketTool::Close(_Socket);
}

/* 프로세싱 전, 해야될 일련의 과정을 이 함수에서 처리하면 된다. */
void Listener::Dispatch(IOCPEvent* NewEvent, DWORD dwTrans) {
	/* Core에서 수신 대기 소켓에 대한 비동기 입출력이 완료되면 아래 구문을 실행한다. */
	assert(NewEvent->_Type == EventType::ACCEPT);
	ProcessAccept(NewEvent);
}

BOOL Listener::StartAccept(std::shared_ptr<ServerService> NewService) {
	/* 서비스 클래스 추가 후 NetAddress 대신 서비스 객체 전달 */
	_Service = NewService;
	if (_Service == nullptr) {
		return FALSE;
	}

	/* 윈도우 전용 비동기 소켓 생성(WSASocket) */
	_Socket = SocketTool::CreateSocket();
	if (_Socket == INVALID_SOCKET) {
		return FALSE;
	}

	/* 
		패킷에 추가될 부가 정보 전달(포인터 응용) - vtable 주의
		여기서 IOCPCore의 Register 함수를 호출한다.
		곧, Listener가 가지는 Socket을 입출력 완료 포트에 등록한다.
	*/
	if (_Service->GetMainCore()->Register(shared_from_this()) == FALSE) {
		return FALSE;
	}
	if (SocketTool::SetReuseAddress(_Socket, TRUE) == FALSE) { 
		return FALSE;
	}
	if (SocketTool::SetLinger(_Socket, 0, 0) == FALSE) { 
		return FALSE;
	}

	NetAddress NewAddress = NewService->GetNetAddress();
	if (SocketTool::Bind(_Socket, NewAddress) == FALSE) {
		return FALSE;
	}
	
	/*
	if (SocketTool::Bind(_Socket, NewService->GetNetAddress()) == FALSE) {
		return FALSE;
	}
	*/

	if (SocketTool::Listen(_Socket) == FALSE) { 
		return FALSE;
	}

	/* 
		구조와 함수가 달라져서 약간 헤맸다.
		해당 수신 대기 소켓으로부터 몇 개의 통신 전용 소켓을 만들 것인지
		아래와 같이 상수값을 이용하여 연결 요청 이벤트를 추가한다.

		곧, 이 서버(또는 리스너)에서 관리할 사용자 수라고 볼 수 있다.
		서버에서 여러 개의 포트를 사용한다면 리스너 역시 여러 개 생길 수 있다.
	*/
	const int Count = 1;
	for (int i = 0; i < Count; i++) {
		IOCPEvent* NewEvent = new IOCPEvent(EventType::ACCEPT);
		NewEvent->_Owner = shared_from_this();
		/* 
			이벤트가 발생한 이후 처리되기까지 Listener객체가 유효해야 하므로
			이벤트를 소유한 대상이 누구인지 관리하여 Reference Count를 유지한다.
		*/
		_AcceptEvents.push_back(NewEvent);
		RegisterAccept(NewEvent);
	}

	return TRUE;
}

void Listener::RegisterAccept(IOCPEvent* Target) {
	/* 세션의 생성자에서 WSASocket 함수 호출, 유효 소켓을 가진다. */
	// Session* NewSession = new Session;
	// std::shared_ptr<Session> NewSession = std::make_shared<Session>();

	/* 
		이 호출만으로 세션을 만든 후 세션을 만든 주체인 서비스를 설정하고,
		입출력 완료 포트의 감시 대상으로 등록한다.
	*/
	std::shared_ptr<Session> NewSession = _Service->CreateSession();

	/* 
		이때 이벤트를 소유한 주체는 Listener 객체이고 이 객체는 입출력 완료 포트에 감시 대상으로 등록되어 있다.
		또한, 위 CreateSession에서 세션의 생성과 감시 대상으로 등록하는 작업까지 수행한다.
	*/
	Target->Initialize();
	Target->_Session = NewSession;					// 소켓 정보(원격지 정보)
	
	DWORD dwRecvBytes = 0;
	/* 
		소켓 변환 전에 미리 Session이 가지는 Socket을 입출력 완료 포트에 등록하며,
		AcceptEx 함수가 완료(비동기)되면 완료 패킷이 전달된다.

		사실상 listen 함수에서 당장 처리되지 않는 연결에 대하여 최댓값(SOMAXCONN)을 전달했기 때문에
		AcceptEx가 아닌 accept 함수를 호출해도 별 문제는 없을 것을 보인다.

		당장 처리되지 않는 연결 곧, 동기 함수인 accept가 반응이 늦어져 처리하지 못한 클라이언트 접속에 대하여
		연결 큐(Connection Queue)에 그 접속 정보를 저장하므로 잘 짜여진 서버라면 크게 문제되지 않는다.

		다만, 복잡한 분기 처리 등을 다수 생략할 수 있고, 운영체제가 맡아 알아서 처리해주는 일련의 과정이
		있으므로 편의성에선 굉장한 차이를 보인다.
	*/
	if (SocketTool::AcceptEx(
		_Socket,									// 수신 대기 전용 소켓
		NewSession->GetSocket(),					// 통신 전용 소켓(변환)
		NewSession->_RecvBuffer.WritePosition(),	// 수신 버퍼(데이터 및 고유 정보 전달)
		0,											// 특이값(0)을 전달하면 연결(accept) 즉시 리턴하여 성공했음을 알린다
		sizeof(struct sockaddr_in) + 16,			// 고정값
		sizeof(struct sockaddr_in) + 16,			// 고정값
		&dwRecvBytes,								// 입출력 작업이 끝날 때 실제 송수신된 데이터의 크기를 반환
		(LPOVERLAPPED)Target						// 비동기 소켓 함수 내부적으로 사용할 정보 구조체
		) == FALSE) {
		
		const int Error = WSAGetLastError();
		if (Error != WSA_IO_PENDING) {
			/* 비동기 소켓 함수이므로 보류 상태가 아닐 때에는 완전한 실패이므로 다시 등록해야 한다. */
			RegisterAccept(Target);
		}
	}
}

void Listener::ProcessAccept(IOCPEvent* Target) {
	/* 
		IOCPCore에서 GetQueuedCompletionStatus 함수를 사용하여 대기 상태로 있다가
		AcceptEx 함수가 실행되고 연결 소켓이 생성되어 Session에 등록되면 부가 정보(Event)를 전달받아
		Event의 소유자인 Listener의 Dispatch 함수를 호출한다.

		해당 함수 호출시 이미 Session의 소켓은 통신을 위한 준비가 끝난 상태이며,
		연결 정보를 불러와 접속자의 IP및 PORT번호를 조사한다.

		이에 대한 설명이 일절 없어서 굉장히 오랜 시간 분석했는데,
		CompletionPort 모델과 AcceptEx 함수의 동작을 제대로 모르는 상태에선 전체 코드를 분석할 수 없다.
	*/
	std::shared_ptr<Session> TargetSession = Target->_Session;

	/* 
		AcceptEx가 변환한 통신 전용 소켓은 그 구조가 일반 소켓과 다르다.

		5번과 6번 인수를 보면, 본래 주소 구조체보다 2배 큰 크기를 전달하는데
		이는 원격 및 로컬 주소 정보를 한 번에 전달받기 위해서인 것으로 보인다.

		정리하면, 어떠한 전송 계층의 주소 구조체가 16바이트일 때 원격과 로컬, 두 개의 정보를
		받아와야 하므로 그 두 배의 크기를 전달하는 것으로 보인다.

		AcceptEx는 이런 특이한 구조 때문에 getpeername, getsocckname 등의
		조사 함수를 사용하려면 아래 함수를 반드시 호출해야 한다.
	*/
	if (SocketTool::SetUpdateAcceptSocket(TargetSession->GetSocket(), _Socket) == FALSE) {
		RegisterAccept(Target);
		return;
	}

	struct sockaddr_in SocketInfo;
	int lSockSize = sizeof(SocketInfo);

	if (getpeername(
		TargetSession->GetSocket(),				// Session 소켓 곧, 통신 전용 소켓
		(struct sockaddr*)&SocketInfo,
		&lSockSize) == SOCKET_ERROR) {

		RegisterAccept(Target);
		return;
	}

	/* 접속 성공 후 TODO : 접속 알림 */
	CHAR IPAddress[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &SocketInfo.sin_addr, IPAddress, sizeof(IPAddress));

	std::cout << "Client Connected!" << std::endl << "[IP Address] : " << IPAddress << std::endl;

	TargetSession->SetNetAddress(NetAddress(SocketInfo));
	TargetSession->ProcessConnect(); // -> 연결 이벤트(확인용) 등록후 관리 세션 추가 -> RegisterRecv까지 호출

	/*
		앞선 호출(ProcessConnect)에서 수신 이벤트까지 등록하면 모든 처리가 끝난다.
		RegisterAccept에서 생성한 하나의 세션에 대하여 Recv 이벤트를 활성했고
		이로써 해당 세션은 서버와 통신만 반복해서 주고받으면 된다.

		아래 RegisterAccept는 다시 새로운 접속자를 받아들이기 위해 꼭 필요한 호출이다.
	*/
	RegisterAccept(Target);
}
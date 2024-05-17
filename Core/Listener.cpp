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
	/* Core에서 비동기 접속이 완료되면 아래 구문을 실행한다. */
	assert(NewEvent->_Type == EventType::ACCEPT);
	ProcessAccept(NewEvent);
}

BOOL Listener::StartAccept(std::shared_ptr<ServerService> NewService) {
	/* 서비스 클래스 추가 후 NetAddress 대신 서비스 객체 전달 */
	_Service = NewService;
	if (_Service == NULL) {
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
	//if (SocketTool::Bind(_Socket, NewAddress) == FALSE) { return FALSE; }
	if (SocketTool::Bind(_Socket, NewService->GetNetAddress()) == FALSE) {
		return FALSE;
	}
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
		/* 기존의 동기 함수(accept)가 아닌 비동기 함수(AcceptEx)를 사용한다. */
		IOCPEvent* NewEvent = new IOCPEvent(EventType::ACCEPT);
		NewEvent->_Owner = shared_from_this();
		_AcceptEvents.push_back(NewEvent);
		RegisterAccept(NewEvent);
	}

	return TRUE;
}

void Listener::RegisterAccept(IOCPEvent* Target) {
	/* 세션의 생성자에서 WSASocket 함수 호출, 유효 소켓을 가진다. */
	// Session* NewSession = new Session;
	// std::shared_ptr<Session> NewSession = std::make_shared<Session>();

	std::shared_ptr<Session> NewSession = _Service->CreateSession();

	Target->Initialize();
	Target->_Session = NewSession;					// 소켓 정보(원격지 정보)
	
	DWORD dwRecvBytes = 0;
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
		TargetSession->GetSocket(),
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
	TargetSession->ProcessConnect();

	/* 
		위의 ProcessConnect 함수가 완성되지 않은 상태라 구조가 어색해 보일 수 있어 약간의 주석을 남긴다.

		접속 요청 이벤트를 등록하는 분기 처리를 살펴보자.
		
		대부분 함수가 실패할 때, 즉 읽을 수 없는 메모리이거나 이전 함수가 완료되지 않았을 때
		또는 새로 등록할 때에 RegisterAccept를 호출한다.

		이는 세션을 생성/재생성하거나 교체하는 과정이며, 일반적인 상황이다.
		그런데, 함수의 끝에서 접속이 성공한 이후에 다시 AcceptEx 함수를 호출한다.

		이는 이벤트를 재사용하는 구조로 작성되어 있어 그런 것인데,
		문제는 현재까지 생성된 세션에 대하여 아무런 처리를 하지 않고 있다는 것이다.

		때문에 그 구조가 어색해 보일 수 있다.

		이후 작성할 Session의 ProcessConnect에서 세션과 서비스를 연결하는 처리를 하므로,
		아직 그 구조가 온전하지 못하다는 것에 유의하자.

		서비스 클래스가 세션을 관리하는 주체가 되므로 추후 서비스 클래스가 완성되면
		전체 구조를 분석해보는 것으로 한다.
	*/
	RegisterAccept(Target);
}
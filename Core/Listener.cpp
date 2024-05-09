#include "pch.h"
#include "Listener.h"
#include "SocketTool.h"
#include "IOCPEvent.h"
#include "Session.h"

Listener::Listener() {
	
}

Listener::~Listener() {
	SocketTool::Close(_Socket);

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

void Listener::Dispatch(class IOCPEvent* NewEvent, DWORD dwTrans) {
	assert(NewEvent->_Type == EventType::ACCEPT);
	ProcessAccept(NewEvent);
}

BOOL Listener::StartAccept(NetAddress NewAddress) {
	/* 윈도우 전용 비동기 소켓 생성(WSASocket) */
	_Socket = SocketTool::CreateSocket();

	if (_Socket == INVALID_SOCKET) { return FALSE; }

	/* 
		패킷에 추가될 부가 정보 전달(포인터 응용) - vtable 주의
		이로써 Listener가 OVERLAPPED 구조를 가진 클래스를 포함하고 있어야 함
	*/
	if (!GlobalCore.Register(this)) { return FALSE; }
	if (!SocketTool::SetReuseAddress(_Socket, TRUE)) { return FALSE; }
	if (!SocketTool::SetLinger(_Socket, 0, 0)) { return FALSE; }
	if (!SocketTool::Bind(_Socket, NewAddress)) { return FALSE; }
	if (!SocketTool::Listen(_Socket)) { return FALSE; }

	const int Count = 1;
	for (int i = 0; i < Count; i++) {
		/* 기존의 동기 함수(accept)가 아닌 비동기 함수(AcceptEx)를 사용한다. */
		IOCPEvent* NewEvent = new IOCPEvent(EventType::ACCEPT);
		_AcceptEvents.push_back(NewEvent);
		RegisterAccept(NewEvent);
	}

	return TRUE;
}

void Listener::RegisterAccept(IOCPEvent* Target) {
	Session* NewSession = new Session();

	Target->Initialize();
	Target->_Session = NewSession;			// 소켓 정보
	
	DWORD dwRecvBytes = 0;
	if (SocketTool::AcceptEx(
		_Socket,
		NewSession->GetSocket(),
		NewSession->_RecvBuffer,
		0,
		sizeof(struct sockaddr_in),
		sizeof(struct sockaddr_in),
		&dwRecvBytes,
		(LPOVERLAPPED)Target
		) == FALSE) {

		const int Error = WSAGetLastError();
		if (Error != WSA_IO_PENDING) {
			/* 비동기 소켓 함수이므로 보류 상태가 아닐 때에는 완전한 실패이므로 다시 등록해야 한다. */
			RegisterAccept(Target);
		}
	}
}

void Listener::ProcessAccept(IOCPEvent* Target) {
	SessionRef TargetSession = Target->_Session;

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

	/* 접속 성공 후 TODO : */
	std::cout << "Client Connected!" << std::endl;
	TargetSession->SetNetAddress(NetAddress(SocketInfo));
	TargetSession->ProcessConnect();
	RegisterAccept(Target);
}
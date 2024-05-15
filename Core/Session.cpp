 #include "pch.h"
#include "Session.h"
#include "Service.h"
#include "SocketTool.h"

Session::Session() {
	/* 
		AcceptEx의 두 번째 인수로 전달할 소켓 핸들
		곧, 통신 전용 소켓이 된다.
	*/
	_Socket = SocketTool::CreateSocket();
}

Session::~Session() {
	SocketTool::Close(_Socket);
}

HANDLE Session::GetHandle() {
	return (HANDLE)_Socket;
}

void Session::Dispatch(IOCPEvent* NewEvent, DWORD dwTrans) {
	// TODO: Recv, Send
	switch (NewEvent->_Type) {
	case EventType::CONNECT:
		ProcessConnect();
		break;
	case EventType::DISCONNECT:
		ProcessDisconnect();
		break;
	case EventType::SEND:
		ProcessSend(NewEvent, dwTrans);
		break;
	case EventType::RECV:
		ProcessRecv(dwTrans);
		break;

	default:
		break;
	}
}

BOOL Session::Connect() {
	return RegisterConnect();
}

void Session::Disconnect(const WCHAR* Reason) {
	if (_Connected.exchange(FALSE) == FALSE) { return; }

	std::wcout << "Disconnect : " << Reason << std::endl;

	OnDisconnected();
	GetService()->ReleaseSession(GetSession());

	RegisterDisconnect();
}

void Session::Send(PBYTE Buffer, INT Length) {
	IOCPEvent* SendEvent = new IOCPEvent(EventType::SEND);

	SendEvent->_Owner = shared_from_this();
	SendEvent->_Buffer.resize(Length);
	memcpy(SendEvent->_Buffer.data(), Buffer, Length);

	std::lock_guard<std::mutex> WriteGuard(_Locks[0]);
	RegisterSend(SendEvent);
}





/*
	등록
*/
void Session::RegisterRecv() {
	if (IsConnected() == FALSE) { return; }

	_RecvEvent.Initialize();
	_RecvEvent._Owner = shared_from_this();

	WSABUF wsabuf;
	wsabuf.buf = reinterpret_cast<char*>(_RecvBuffer);
	wsabuf.len = sizeof(_RecvBuffer) / sizeof(_RecvBuffer[0]);

	DWORD dwBytes, Flags;
	dwBytes = Flags = 0;

	if (WSARecv(_Socket, &wsabuf, 1, &dwBytes, &Flags, (LPOVERLAPPED)&_RecvEvent, NULL)) {
		INT Error = WSAGetLastError();

		if (Error != WSA_IO_PENDING) {
			HandleError(Error);
			_RecvEvent._Owner = NULL;
		}
	}
}

void Session::RegisterSend(IOCPEvent* SendEvent) {
	if (IsConnected() == FALSE) { return; }

	WSABUF wsabuf;
	wsabuf.buf = (char*)SendEvent->_Buffer.data();
	wsabuf.len = (ULONG)SendEvent->_Buffer.size();

	DWORD dwBytes = 0;
	if (WSASend(_Socket, &wsabuf, 1, &dwBytes, 0, (LPOVERLAPPED)&SendEvent, NULL)) {
		INT Error = WSAGetLastError();
		
		if (Error != WSA_IO_PENDING) {
			HandleError(Error);
			SendEvent->_Owner = NULL;
			delete SendEvent;
		}
	}
}

BOOL Session::RegisterConnect() {
	if (IsConnected()) { return FALSE; }
	if (GetService()->GetType() != ServiceType::CLIENT) { return FALSE; }
	if (SocketTool::SetReuseAddress(_Socket, TRUE) == FALSE) { return FALSE; }
	if (SocketTool::BindAnyAddress(_Socket, 0) == FALSE) { return FALSE; }

	_ConnectEvent.Initialize();
	_ConnectEvent._Owner = shared_from_this();

	DWORD dwBytes = 0;
	struct sockaddr_in SockInfo = GetService()->GetNetAddress().GetSocketInfo();
	if (SocketTool::ConnectEx(_Socket, reinterpret_cast<struct sockaddr*>(&SockInfo), sizeof(SockInfo), NULL, 0, &dwBytes, &_ConnectEvent)) {
		INT Error = WSAGetLastError();

		if (Error != WSA_IO_PENDING) {
			_ConnectEvent._Owner = NULL;
			return FALSE;
		}
	}

	return TRUE;
}

BOOL Session::RegisterDisconnect() {
	_DisconnectEvent.Initialize();
	_DisconnectEvent._Owner = shared_from_this();

	if (SocketTool::DisconnectEx(_Socket, &_DisconnectEvent, TF_REUSE_SOCKET, 0)) {
		INT Error = WSAGetLastError();

		if (Error != WSA_IO_PENDING) {
			_DisconnectEvent._Owner = NULL;
			return FALSE;
		}
	}

	return TRUE;
}

/*
	프로세싱
*/
void Session::ProcessRecv(DWORD dwRecvBytes) {
	_RecvEvent._Owner = NULL;

	if (dwRecvBytes == 0) {
		Disconnect(L"RecvBytes is zero");
		return;
	}

	OnRecv(_RecvBuffer, dwRecvBytes);

	RegisterRecv();
}

void Session::ProcessSend(IOCPEvent* SendEvent, DWORD dwSendBytes) {
	SendEvent->_Owner = NULL;
	delete SendEvent;

	if (dwSendBytes == 0) {
		Disconnect(L"SendBytes is zero");
		return;
	}

	OnSend(dwSendBytes);
}

void Session::ProcessConnect() {
	_ConnectEvent._Owner = NULL;
	_Connected.store(TRUE);

	GetService()->AppendSession(GetSession());
	OnConnected();

	/* 
		원격지의 접속(AcceptEx)이 성공하면 연결 이벤트를 초기화하여 사용 가능한 상태로 만든다.

		이후 상태값(_Connected)을 조정하고 관리 주체(Service)에 접속한 유저(Session)의 정보를 등록한다.
		연결 직후엔 아무런 교점이 없으므로 수신 이벤트 곧, _RecvEvent를 초기화 하고
		WSARecv 함수를 호출하여 수신 대기 상태로 만든다.

		이후부터 발생하는 모든 네트워크 이벤트는 세션의 Dispatch 함수로 이어진다.
	*/
	RegisterRecv();
}

void Session::ProcessDisconnect() {
	_DisconnectEvent._Owner = NULL;
}

/*
	에러 처리
*/
void Session::HandleError(INT ErrorCode) {
	switch (ErrorCode) {
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect(L"Handle Error");
		break;

	default:
		// TODO : Write Log
		std::cout << "Handle Error : " << ErrorCode << std::endl;
		break;
	}
}
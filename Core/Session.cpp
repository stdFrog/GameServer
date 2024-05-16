 #include "pch.h"
#include "Session.h"
#include "Service.h"
#include "SocketTool.h"

Session::Session() : BUFFER_SIZE(0x10000), _RecvBuffer(BUFFER_SIZE) {
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

void Session::Send(std::shared_ptr<SendBuffer> Buffer) {
	BOOL Registered = FALSE;
	
	std::lock_guard<std::mutex> WriteGuard(_Locks[0]);
	_Queue.push(Buffer);
	Registered = _SendRegistered.exchange(TRUE) == FALSE;
	WriteGuard.~lock_guard();
	
	if (Registered) {
		RegisterSend();
	}
	
	/*
	IOCPEvent* SendEvent = new IOCPEvent(EventType::SEND);

	SendEvent->_Owner = shared_from_this();
	SendEvent->_Buffer.resize(Length);
	memcpy(SendEvent->_Buffer.data(), Buffer, Length);

	std::lock_guard<std::mutex> WriteGuard(_Locks[0]);
	RegisterSend(SendEvent);
	*/
}





/*
	등록
*/
void Session::RegisterRecv() {
	if (IsConnected() == FALSE) { return; }

	_RecvEvent.Initialize();
	_RecvEvent._Owner = shared_from_this();

	/*
		임시 버퍼의 시작 번지를 넘기는 것만으로 제대로 통신이 가능한가?
		통신에 사용할 데이터를 구조화하지 않았으므로 의미있는 통신은 아직 불가능하다.

		구조화 작업은 컨텐츠 작업시 만들 것이므로 일단 넘어가기로 한다.
		
		그 외에 또 다른 문제가 하나 있는데,
		수신 버퍼가 가득차거나 사용중인 버퍼를 덮어쓰는 등의 이유로 인해
		클라이언트가 보내는 데이터중 일부만을 전달받았다고 생각해보자.
		
		곧, 데이터가 일부 유실된 상황이다.

		서버측에선 이러한 상황이 발생하지 않도록 예방할 필요가 있으며,
		이를 위한 수단으로써 자동화된 버퍼 객체를 유지/관리하는 방법이 있다.
	*/
	WSABUF wsabuf;
	wsabuf.buf = reinterpret_cast<char*>(_RecvBuffer.WritePosition());
	wsabuf.len = _RecvBuffer.FreeSize();			// 최대 크기를 지정한다.

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

/* 
	현재 구조를 보면 이 함수를 호출하는 세션 객체는 단 하나뿐이다.

	SendEvent를 하나만 두고 재사용하는 구조이므로 여러 객체가 사용할 수 없으며
	최초 Send 이벤트를 호출한 세션 객체가 아래 RegisterSend까지 수행한다.
*/
void Session::RegisterSend(/*IOCPEvent* SendEvent*/) {
	if (IsConnected() == FALSE) { return; }

	_SendEvent.Initialize();
	_SendEvent._Owner = shared_from_this();

	_SendEvent._SendBuffers.clear();

	/* 
		WSASend, WSARecv는 Scatter-Gather 입출력을 지원하므로
		처리해야할 데이터 크기를 합산하고 이벤트가 관리하는 SendBuffer 벡터에 밀어넣는다.
	*/
	std::lock_guard<std::mutex> WriteGuard(_Locks[0]);
	INT WriteSize = 0;
	while (_Queue.empty() == FALSE) {
		std::shared_ptr<SendBuffer> Buffer = _Queue.front();
		WriteSize += Buffer->WriteSize();
		_Queue.pop();
		_SendEvent._SendBuffers.push_back(Buffer);
	}
	WriteGuard.~lock_guard();
	
	std::vector<WSABUF> Bufs;
	Bufs.reserve(_SendEvent._SendBuffers.size());
	for (std::shared_ptr<SendBuffer> SendBuffer : _SendEvent._SendBuffers) {
		WSABUF wsabuf;
		wsabuf.buf = (char*)SendBuffer->Buffer();
		wsabuf.len = (ULONG)SendBuffer->WriteSize();
		Bufs.push_back(wsabuf);
	}
	
	/*
		한번에 보낸다.
	*/
	DWORD dwBytes = 0;
	if (WSASend(_Socket, /* &wsabuf */Bufs.data(), /* 1 */Bufs.size(), &dwBytes, 0, (LPOVERLAPPED)&_SendEvent, NULL)) {
		INT Error = WSAGetLastError();
		
		if (Error != WSA_IO_PENDING) {
			HandleError(Error);
			_SendEvent._Owner = NULL;
			// delete SendEvent;
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
/*
	TCP 특성상 DGRAM 곧, 데이터간 경계 구분을 하지 않으므로,
	응용 프로그램 프로토콜 즉, 데이터의 형식과 의미, 처리 방식을 정의할 필요가 있다.

	통신 양단이 주고받을 데이터는 컨텐츠 수준에서 정하고,
	경계를 구분하기 위한 형식을 서버와 컨텐츠가 서로 통합하면
	응용 프로그램 프로토콜을 구성했다고 볼 수 있다.

	데이터 경계를 구분하기 위한 방법은 여러가지가 있는데
	이 프로젝트에선 부가 정보가 포함된 고정 길이의 헤더를 먼저 보내고
	가용 데이터를 이어서 보내는 방법을 활용하기로 한다.

	추가로, WSARecv의 플래그 중 MSG_WAITALL이라는 비트 마스크가 있다.
	레퍼런스를 보면, 이 플래그를 적용한 후 넌블로킹 모드의 소켓을 사용하면 항상 실패한다고 나와 있다.

	이를 이용해 현 프로젝트에서 사용되는 네트워크 통신 모델이 어떤 소켓 모드를 사용하는지 알 수 있을 것으로 보인다.
	확인 결과, 넌블로킹 소켓이 아님을 알 수 있었다.

	MSG_WAITALL 플래그를 적용한 후 WSARecv 호출자 즉, 서버에서 버퍼 크기를 줄인 후 시험해봤다.
	MSG_WAITALL 플래그가 적용될 경우 제공된 버퍼가 가득차거나 연결이 끊기거나, 취소 또는 오류 발생시
	수신 요청이 완료되는 것으로 나와있다.

	여기에 예외 분기까지 추가하였으나 아무런 문제없이 통신되는 것을 알 수 있었다.
	정리하면, CompletionPort 모델은 넌블로킹 모드를 사용하지 않으며 중첩 모드를 사용한다.
*/
void Session::ProcessRecv(DWORD dwRecvBytes) {
	_RecvEvent._Owner = NULL;

	if (dwRecvBytes == 0) {
		Disconnect(L"RecvBytes is zero");
		return;
	}

	if (_RecvBuffer.OnWrite(dwRecvBytes) == FALSE) {
		Disconnect(L"OnWrite Failed");
		return;
	}

	INT Usaged = _RecvBuffer.DataSize();
	INT ProcessLength = OnRecv(_RecvBuffer.ReadPosition(), Usaged);

	if (ProcessLength < 0 || Usaged < ProcessLength) {
		Disconnect(L"OnRecv Failed");
		return;
	}

	if (_RecvBuffer.OnRead(ProcessLength) == FALSE) {
		Disconnect(L"OnRead Failed");
		return;
	}

	_RecvBuffer.Clean();

	RegisterRecv();
}

/*
	당연한 얘기지만, WSASend의 리턴 시점은 함수 호출 후 내부 코드가 실행된 직후이다.

	그런데, 이 구조를 이해하지 못한 상황에선 아래 함수가 완료되는 시점을 이해하기 어려울 수 있다.
	WSASend는 비동기 함수로, 내부 코드를 실행한 후 곧바로 리턴하며 입출력 작업이 모두 끝나면
	운영체제의 도움을 받아 완료 통지를 보낸다.

	즉, 모든 데이터의 입출력 작업이 끝났을 때에만 완료 통지가 발생하므로 완료 시점이 일치하는가에 대한
	여부는 의심할 필요 없다.
*/
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
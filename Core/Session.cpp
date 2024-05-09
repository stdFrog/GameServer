#include "pch.h"
#include "Session.h"

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
	
}
#include "pch.h"
#include "SocketTool.h"

LPFN_ACCEPTEX SocketTool::AcceptEx = NULL;
LPFN_CONNECTEX SocketTool::ConnectEx = NULL;
LPFN_DISCONNECTEX SocketTool::DisconnectEx = NULL;

void SocketTool::Initialize() {
	WSADATA wsa;
	assert(WSAStartup(MAKEWORD(2, 2), &wsa) == 0);

	SOCKET Dummy = CreateSocket();

	assert(BindWindowFunction(Dummy, WSAID_ACCEPTEX, reinterpret_cast<LPVOID*>(&AcceptEx)));
	assert(BindWindowFunction(Dummy, WSAID_CONNECTEX, reinterpret_cast<LPVOID*>(&ConnectEx)));
	assert(BindWindowFunction(Dummy, WSAID_DISCONNECTEX, reinterpret_cast<LPVOID*>(&DisconnectEx)));

	Close(Dummy);
}

void SocketTool::Clear() {
	WSACleanup();
}

BOOL SocketTool::Bind(SOCKET Socket, struct sockaddr_in Info) {
	return bind(Socket, (struct sockaddr*)&Info, sizeof(Info));
}

BOOL SocketTool::Listen(SOCKET Socket, UINT Maximum) {
	return listen(Socket, Maximum) != SOCKET_ERROR;
}

BOOL SocketTool::BindAnyAddress(SOCKET Socket, UINT Port) {
	struct sockaddr_in Local;
	Local.sin_family = AF_INET;
	Local.sin_addr.s_addr = htonl(INADDR_ANY);
	Local.sin_port = htons(Port);

	return bind(Socket, (struct sockaddr*)&Local, sizeof(Local)) != SOCKET_ERROR;
}

void SocketTool::Close(SOCKET& Socket) {
	if (Socket != INVALID_SOCKET) {
		closesocket(Socket);
		Socket = INVALID_SOCKET;
	}
}

BOOL SocketTool::SetTCPNoDelay(SOCKET Socket, BOOL bFlag) {
	return SetSocketOption(Socket, SOL_SOCKET, TCP_NODELAY, bFlag);
}

BOOL SocketTool::SetUpdateAcceptSocket(SOCKET Socket, SOCKET ListenSocket) {
	return SetSocketOption(Socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, ListenSocket);
}

BOOL SocketTool::SetLinger(SOCKET Socket, USHORT Turn, USHORT Linger) {
	LINGER Opt;

	Opt.l_linger = Linger;
	Opt.l_onoff = Turn;

	return SetSocketOption(Socket, SOL_SOCKET, SO_LINGER, Opt);
}

BOOL SocketTool::SetReuseAddress(SOCKET Socket, BOOL bFlag) {
	return SetSocketOption(Socket, SOL_SOCKET, SO_REUSEADDR, bFlag);
}

BOOL SocketTool::SetRecvBufferSize(SOCKET Socket, UINT Size) {
	return SetSocketOption(Socket, SOL_SOCKET, SO_RCVBUF, Size);
}

BOOL SocketTool::SetSendBufferSize(SOCKET Socket, UINT Size) {
	return SetSocketOption(Socket, SOL_SOCKET, SO_SNDBUF, Size);
}

SOCKET SocketTool::CreateSocket() {
	return WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}

BOOL SocketTool::BindWindowFunction(SOCKET Socket, GUID Serial, LPVOID* fn) {
	DWORD dwBytes = 0;

	return WSAIoctl(
		Socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&Serial,
		sizeof(Serial),
		fn,
		sizeof(*fn),
		&dwBytes,
		NULL,
		NULL) != SOCKET_ERROR;
}
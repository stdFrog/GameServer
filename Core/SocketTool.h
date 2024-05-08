#pragma once

class SocketTool
{
public:
	static LPFN_ACCEPTEX AcceptEx;
	static LPFN_CONNECTEX ConnectEx;
	static LPFN_DISCONNECTEX DisconnectEx;

public:
	static BOOL Bind(SOCKET, struct sockaddr_in);
	static BOOL BindAnyAddress(SOCKET, UINT);
	static BOOL Listen(SOCKET, UINT Maximum = SOMAXCONN);
	static void Close(SOCKET&);

public:
	static BOOL SetTCPNoDelay(SOCKET, BOOL);
	static BOOL SetUpdateAcceptSocket(SOCKET, SOCKET);

	static BOOL SetLinger(SOCKET, USHORT, USHORT);
	static BOOL SetReuseAddress(SOCKET, BOOL);
	static BOOL SetRecvBufferSize(SOCKET, UINT);
	static BOOL SetSendBufferSize(SOCKET, UINT);


public:
	static SOCKET CreateSocket();
	static BOOL BindWindowFunction(SOCKET, GUID, LPVOID* fn);

public:
	static void Initialize();
	static void Clear();
};

template <typename T>
static inline BOOL SetSocketOption(SOCKET Socket, UINT Level, UINT Option, T Value) {
	return SOCKET_ERROR != setsockopt(Socket, Level, Option, (const char*)&Value, sizeof(T));
}


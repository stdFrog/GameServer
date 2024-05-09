#pragma once

/*
	IP, Port 관리 클래스

	IP : 통신에 참여하는 개체 각각의 고유한 논리적 주소를 말한다. 이는 전세계적으로 유일성을 보장한다.
	PORT : 통신의 종착점을 구분하는 16비트 주소값이며, 일반적으로 호스트의 프로세스를 나타내는 주소값으로 보기도 한다.
*/
class NetAddress
{
	struct sockaddr_in _SocketInfo;

public:
	static in_addr IPtoAddress(const WCHAR* IP);

public:
	struct sockaddr_in& GetSocketInfo() { return _SocketInfo; }

	std::wstring GetIP();
	USHORT GetPort() { return ntohs(_SocketInfo.sin_port); }

public:
	NetAddress();
	NetAddress(struct sockaddr_in Info);
	NetAddress(std::wstring IP, USHORT Port);
};


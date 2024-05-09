#include "pch.h"
#include "NetAddress.h"

NetAddress::NetAddress() {

}

NetAddress::NetAddress(struct sockaddr_in Info)  : _SocketInfo(Info){

}

NetAddress::NetAddress(std::wstring IP, USHORT Port) {
	memset(&_SocketInfo, 0, sizeof(_SocketInfo));

	_SocketInfo.sin_family = AF_INET;
	_SocketInfo.sin_addr = IPtoAddress(IP.c_str());
	_SocketInfo.sin_port = htons(Port);
}

std::wstring NetAddress::GetIP() {
	WCHAR buf[100];

	/* inet_ntop은 PSTR 타입만 받기 때문에 Wide char를 받는 함수를 사용한다. */
	InetNtopW(AF_INET, &_SocketInfo.sin_addr, buf, sizeof(buf) / sizeof(WCHAR));
	return std::wstring(buf);
}

struct in_addr NetAddress::IPtoAddress(const WCHAR* IP) {
	in_addr Address = {};

	InetPtonW(AF_INET, IP, &Address);
	return Address;
}
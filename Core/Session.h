#pragma once
#include "IOCPCore.h"
#include "IOCPEvent.h"
#include "NetAddress.h"

/*
	접속한 클라이언트의 정보를 포함하며, 서버와 연결된 통신 전용 소켓을 가지는
	주요 관리 대상 클래스이다.
*/
class Session : public IOCPObject
{
private:
	SOCKET _Socket = INVALID_SOCKET;
	NetAddress _NetAddress;							// 클라이언트의 주소를 담는다.
	std::atomic<BOOL> _Connected = FALSE;

public:
	/*
		임시 버퍼이며 AcceptEx 함수가 사용한다.
		
		송수신 데이터를 메모리 선두에 전달하며 원격 주소와 로컬 주소에 대한 정보도 포함한다.
		단, 데이터가 선두에 오고 후미에 주소 정보가 전달된다.
	*/
	CHAR _RecvBuffer[0x400];						

public:
	void SetNetAddress(NetAddress NewAddress) { _NetAddress = NewAddress; }
	NetAddress GetAddress() { return _NetAddress; }
	SOCKET GetSocket() { return _Socket; }

public:
	virtual HANDLE GetHandle();
	virtual void Dispatch(IOCPEvent* NewEvent, DWORD dwTrans = 0);

public:
	Session();
	virtual ~Session();

};


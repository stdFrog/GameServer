#pragma once
#include "IOCPCore.h"
#include "IOCPEvent.h"
#include "NetAddress.h"

class Service;

/*
	접속한 클라이언트의 정보를 포함하며, 서버와 연결된 통신 전용 소켓을 가지는
	주요 관리 대상 클래스이다.
*/
class Session : public IOCPObject
{
	friend class Listener;
	friend class IOCPCore;
	friend class Service;

private:
	SOCKET _Socket = INVALID_SOCKET;
	NetAddress _NetAddress;							// 클라이언트의 주소를 담는다.
	std::atomic<BOOL> _Connected = FALSE;
	std::weak_ptr<Service> _Service;				// 누구로부터 생성되었는지

private:
	std::mutex _Locks[100];
	/* 송수신 관련 함수 */

private:
	IOCPEvent _RecvEvent{EventType::RECV};
	IOCPEvent _ConnectEvent{EventType::CONNECT};
	IOCPEvent _DisconnectEvent{EventType::DISCONNECT};

private:
	void RegisterRecv();
	void RegisterSend(IOCPEvent* SendEvent);
	BOOL RegisterConnect();
	BOOL RegisterDisconnect();

private:
	void ProcessRecv(DWORD dwRecvBytes);
	void ProcessSend(IOCPEvent* SendEvent, DWORD dwSendBytes);
	void ProcessConnect();
	void ProcessDisconnect();

private:
	void HandleError(INT ErrorCode);

protected:
	/* 컨텐츠 코드 */
	virtual void OnConnected() {}
	virtual void OnDisconnected() {}

	virtual INT OnRecv(PBYTE Buffer, INT Length) { return Length; }
	virtual void OnSend(INT Length) {}

public:
	BOOL Connect();
	void Disconnect(const WCHAR* Reason);
	void Send(PBYTE Buffer, INT Length);

public:
	std::shared_ptr<Service> GetService() { return _Service.lock(); }
	void SetService(std::shared_ptr<Service> Service) { _Service = Service; }

public:
	/*
		임시 버퍼이며 AcceptEx 함수가 사용한다.
		
		송수신 데이터를 메모리 선두에 전달하며 원격 주소와 로컬 주소에 대한 정보도 포함한다.
		단, 데이터가 선두에 오고 후미에 주소 정보가 전달된다.
	*/
	BYTE _RecvBuffer[0x400];						

public:
	BOOL IsConnected() { return _Connected; }
	std::shared_ptr<Session> GetSession() { return std::static_pointer_cast<Session>(shared_from_this()); }

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


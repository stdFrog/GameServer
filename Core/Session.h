#pragma once
#include "IOCPCore.h"
#include "IOCPEvent.h"
#include "NetAddress.h"
#include "RecvBuffer.h"
#include "SendBuffer.h"

class Service;

/*
	접속한 클라이언트의 정보를 포함하며, 서버와 연결된 통신 전용 소켓을 가지는 주요 관리 대상 클래스이다.

	세 개의 클래스를 프렌드 지정하여 숨겨진 멤버에 대한 접근을 허용하고 있다.
	굳이 필요없는 멤버에 대한 액세스까지 허용하고 있으므로 이 구조가 마음에 들지 않는다면,
	아래와 같이 프렌드 멤버 함수를 이용해 특정 함수에서만 접근 가능하게끔 만들 수도 있다.
	
	> friend void Listener::AccessOnConnect(const Session& T);

	리팩토링 때 수정해보는 것으로 하자.
*/
class Session : public IOCPObject
{
	friend class Listener;
	friend class IOCPCore;
	friend class Service;

	const INT BUFFER_SIZE;

private:
	SOCKET _Socket = INVALID_SOCKET;
	NetAddress _NetAddress;							// 클라이언트의 주소를 담는다.
	std::atomic<BOOL> _Connected = FALSE;
	std::weak_ptr<Service> _Service;				// 누구로부터 생성되었는지

private:
	std::mutex _Locks[100];
	/*
		송수신 버퍼를 임시 버퍼가 아닌 하나의 클래스로 만들어 관리한다.
	*/
	RecvBuffer _RecvBuffer;
	std::queue<std::shared_ptr<SendBuffer>> _Queue;
	std::atomic<BOOL> _SendRegistered = FALSE;

private:
	IOCPEvent _RecvEvent{ EventType::RECV };
	IOCPEvent _ConnectEvent{ EventType::CONNECT };
	IOCPEvent _DisconnectEvent{ EventType::DISCONNECT };

	/* 
		현재 프로젝트에선 SendEvent도 하나만 만들어 재사용하기로 한다.
		크게 문제가 되지 않는다.
	*/
	IOCPEvent _SendEvent{ EventType::SEND };

private:
	void RegisterRecv();
	// void RegisterSend(IOCPEvent* SendEvent);		// 버퍼 클래스 제작 후 매개변수 받지 않음
	void RegisterSend();
	BOOL RegisterConnect();
	BOOL RegisterDisconnect();

private:
	void ProcessRecv(DWORD dwRecvBytes);
	// void ProcessSend(IOCPEvent* SendEvent, DWORD dwSendBytes);
	void ProcessSend(DWORD dwSendBytes);
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
	// void Send(PBYTE Buffer, INT Length);
	void Send(std::shared_ptr<SendBuffer> Buffer);

public:
	std::shared_ptr<Service> GetService() { return _Service.lock(); }
	void SetService(std::shared_ptr<Service> Service) { _Service = Service; }

public:
	/*
		임시 버퍼이며 AcceptEx 함수가 사용한다.
		
		송수신 데이터를 메모리 선두에 전달하며 원격 주소와 로컬 주소에 대한 정보도 포함한다.
		단, 데이터가 선두에 오고 후미에 주소 정보가 전달된다.
	
		BYTE _RecvBuffer[0x400];	
	*/

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


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
	SOCKET _Socket = INVALID_SOCKET;				// 통신 전용 소켓이다. -> WSARecv, WSASend 등의 비동기 함수 호출시 반응한다.
	NetAddress _NetAddress;							// 클라이언트의 주소를 담는다.
	std::atomic<BOOL> _Connected = FALSE;
	std::weak_ptr<Service> _Service;				// 누구로부터 생성되었는지 주체를 지정한다.

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

/* 
	패킷 핸들러를 패킷 세션이라는 이름으로 만들어 사용한다.
	
	응용 프로그램 수준에서 주고받는 데이터의 형식과 의미, 처리 방식을 정의하는 클래스이다.
	방식은 여러가지가 있는데 고정 길이의 데이터를 전송하는 방식과 가변 길이의 데이터를 전송하는 방식 두 가지를
	모두 활용할 수 있는 방법을 채택했다.

	송신 측에서 가변 길이 데이터의 크기를 미리 계산할 수 있고, 수신 측에서는 약속된 방법으로 전달받으면 되므로
	가장 효율적이다.

	이 방법을 이용하면 비트 플래그를 활용하여 4바이트 크기의 데이터만으로도 여러가지 정보를 헤더에 추가하여 보낼 수 있다
*/
struct PacketHeader
{
	unsigned short Size;
	unsigned short ID;
};

class PacketSession : public Session
{
public:
	PacketSession();
	virtual ~PacketSession();

	std::shared_ptr<PacketSession> GetPacketSession() { return std::static_pointer_cast<PacketSession>(shared_from_this()); }

protected:
	/* 
		sealed는 표준 C++이 아닌 MS의 확장이라고 한다
		표준을 원한다면 final을 사용하자.
		
		해당 키워드는 가상 멤버를 재정의할 수 없음을 나타낸다.
		정확히는 클래스 간의 상속에 관한 명시적 표현을 위해 사용되며 아래 사용된 sealed는
		해당 클래스로부터 파생된 클래스가 OnRecv 인터페이스를 사용하지 못하도록 제한한다(오버라이딩 포함).
	*/
	virtual INT OnRecv(BYTE* Buffer, int Length) sealed;
	virtual void OnRecvPacket(BYTE* Buffer, int Length) = 0;
	/* 
		OnRecv 함수에선 전달된 패킷의 헤더만을 확인한다.
		간단한 분기로, 패킷의 크기가 4바이트 보다 작으면 무시한다.

		문제가 없다면 OnRecvPacket 함수를 호출하여 필요한 처리를 한다.		
	*/
};

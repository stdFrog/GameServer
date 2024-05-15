#pragma once
#include "NetAddress.h"
#include "IOCPCore.h"
#include "Listener.h"
#include "Session.h"
#include <functional>

// class Session;

enum class ServiceType : UCHAR {
	SERVER,
	CLIENT
};

using SessionFactory = std::function<std::shared_ptr<Session>(void)>;
// std::shared_ptr<Session>(*fn)(void);

/*
	서비스는 보통 백그라운드에서 실행되는 프로그램을 말한다.
	
	곧, 사용자에게 알릴 필요없는 부가 기능을 담당한다.
	예로, 시스템의 유지, 데이터 제공, 하드웨어 관리 등등 여러 가지 일을 맡아 처리한다.

	일반적으로 서비스는 사용자와의 직접적인 상호작용이 필요하지 않다는 것이 특징이다.
	서버측에서 제공하는 서비스 역시 크게 다르지 않아 보인다.
*/
/*
	여기서 서비스는 리스너와 세션을 관리하는 주체가 된다.

	엔진 관점에서 볼 때 서비스는 접속자, 즉 세션으로부터 원격지에 대한 정보
	곧, 서버 또는 클라이언트의 정보를 얻어와 서로간 통신할 수 있도록 설계해야 한다.
	
	이를 위해선 서비스와 세션이 상호간 포함 관계로 이루어져야 하며,
	스마트 포인터(shared_ptr)를 사용하는 현재 구조에선 참조 횟수만 잘 관리하면 된다.

	단, 세션은 접속자의 정보, 요컨데 소켓 정보 + @ 만큼의 추가 정보를 가지는 것이 일반적이며
	이를 위해 컨텐츠 측에서 세션 객체를 유지/관리하게 된다.

	엔진은 컨텐츠가 생성한 세션 정보를 전달받아야 하므로 마땅한 수단을 컨텐츠측에 제공해야 한다.
	
	이때, 콜백 함수를 활용하는 것이 일반적이며 당장 떠오르는 유일한 수단이므로
	이 프로젝트에서도 콜백 함수를 이용하기로 한다.

	별도의 서비스 프로그램을 작성한다면 여러 가지 방법이 있을 수 있다.
*/
class Service : public std::enable_shared_from_this<Service>
{
protected:
	std::mutex _Locks[100];					// USE_LOCK;

protected:
	ServiceType _Type;
	NetAddress _NetAddress = {};
	std::shared_ptr<IOCPCore> _MainCore;

protected:
	INT _SessionCount = 0;
	INT _MaxSessionCount = 0;
	std::set<std::shared_ptr<Session>> _Sessions;
	SessionFactory _InitSession;
	// std::shared_ptr<Session>(*_InitSession)(void);

public:
	virtual BOOL Start() = 0;						// 클라이언트와 서버 연결 관리 함수
	virtual void Close();

public:
	void SetCallback(SessionFactory fn) { _InitSession = fn; }
	BOOL IsPossible() { return _InitSession != NULL; }
	
public:
	std::shared_ptr<Session> CreateSession();
	void AppendSession(std::shared_ptr<Session> NewSession);
	void ReleaseSession(std::shared_ptr<Session> Target);

public:
	INT GetSessionCount() { return _SessionCount; }
	INT GetMaxSessionCount() { return _MaxSessionCount; }

public:
	ServiceType GetType() { return _Type; }
	NetAddress GetNetAddress() { return _NetAddress; }
	std::shared_ptr<IOCPCore>& GetMainCore() { return _MainCore; }

public:
	Service(
		ServiceType Type,
		NetAddress NewAddress,
		std::shared_ptr<IOCPCore> Core,
		SessionFactory InitSession,
		INT MaxSessionCount = 1
	);

	virtual ~Service();
};

class ClientService : public Service {
public:
	virtual BOOL Start();

public:
	ClientService(
		NetAddress RemoteAddress,
		std::shared_ptr<IOCPCore> MainCore,
		SessionFactory InitSession,
		INT MaxSessionCount);

	virtual ~ClientService();
};

class ServerService : public Service {
	std::shared_ptr<Listener> _Listener = NULL;

public:
	virtual BOOL Start();
	virtual void Close();

public:
	ServerService(
		NetAddress RemoteAddress,
		std::shared_ptr<IOCPCore> MainCore,
		SessionFactory InitSession,
		INT MaxSessionCount);

	virtual ~ServerService();
};
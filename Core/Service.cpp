#include "pch.h"
#include "Service.h"

Service::Service(
	ServiceType Type,
	NetAddress NewAddress,
	std::shared_ptr<IOCPCore> Core,
	SessionFactory InitSession,
	INT MaxSessionCount
) : _Type(Type), _NetAddress(NewAddress), _MainCore(Core), _InitSession(InitSession), _MaxSessionCount(MaxSessionCount)
{

}

Service::~Service() {

}

void Service::Close() {
	// TODO : DB 등의 종료 처리
}

std::shared_ptr<Session> Service::CreateSession() {
	std::shared_ptr<Session> NewSession = _InitSession();

	if (_MainCore->Register(NewSession) == FALSE) {
		return NULL;
	}

	return NewSession;
}

void Service::AppendSession(std::shared_ptr<Session> NewSession) {
	std::lock_guard<std::mutex> WriteGuard(_Locks[0]);
	_SessionCount++;
	_Sessions.insert(NewSession);
}

void Service::RemoveSession(std::shared_ptr<Session> Target) {
	std::lock_guard<std::mutex> WriteGuard(_Locks[0]);
	assert(_Sessions.erase(Target) != 0);
	_SessionCount--;
}

/*
	Client Service
*/
ClientService::ClientService(
	NetAddress RemoteAddress,
	std::shared_ptr<IOCPCore> MainCore,
	SessionFactory InitSession,
	INT MaxSessionCount
) : Service(ServiceType::CLIENT, RemoteAddress, MainCore, InitSession, MaxSessionCount)
{

}

ClientService::~ClientService() {

}

BOOL ClientService::Start() {
	return TRUE;
}

/*
	Server Service
*/
ServerService::ServerService(
	NetAddress RemoteAddress,
	std::shared_ptr<IOCPCore> MainCore,
	SessionFactory InitSession,
	INT MaxSessionCount
) : Service(ServiceType::SERVER, RemoteAddress, MainCore, InitSession, MaxSessionCount)
{

}

ServerService::~ServerService() {

}

BOOL ServerService::Start() {
	if (IsPossible() == FALSE) { return FALSE; }

	_Listener = std::make_shared<Listener>();
	if (_Listener == NULL) { return FALSE; }

	std::shared_ptr<ServerService> NewService = std::static_pointer_cast<ServerService>(shared_from_this());
	if (_Listener->StartAccept(NewService) == FALSE) {
		return FALSE;
	}

	return TRUE;
}

void ServerService::Close() {
	Service::Close();
}
#pragma once
#include "IOCPCore.h"
#include "NetAddress.h"

class IOCPEvent;
class ServerService;

class Listener : public IOCPObject
{
	void RegisterAccept(IOCPEvent* Target);
	void ProcessAccept(IOCPEvent* Target);

protected:
	SOCKET _Socket = INVALID_SOCKET;
	std::vector<IOCPEvent*> _AcceptEvents;
	std::shared_ptr<ServerService> _Service;

public:
	// BOOL StartAccept(NetAddress NewAddress);
	BOOL StartAccept(std::shared_ptr<ServerService> NewService);
	void CloseSocket();

public:
	virtual HANDLE GetHandle();
	virtual void Dispatch(IOCPEvent* NewEvent, DWORD dwTrans = 0);

public:
	Listener();
	~Listener();
};


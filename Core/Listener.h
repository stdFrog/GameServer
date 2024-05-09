#pragma once
#include "IOCPCore.h"
#include "NetAddress.h"

class IOCPEvent;

class Listener : public IOCPObject
{
	void RegisterAccept(IOCPEvent* Target);
	void ProcessAccept(IOCPEvent* Target);

protected:
	SOCKET _Socket = INVALID_SOCKET;
	std::vector<IOCPEvent*> _AcceptEvents;

public:
	BOOL StartAccept(NetAddress NewAddress);
	void CloseSocket();

public:
	virtual HANDLE GetHandle();
	virtual void Dispatch(class IOCPEvent* NewEvent, DWORD dwTrans = 0);

public:
	Listener();
	~Listener();
};


#pragma once
#include "Session.h"

class GameSession : public Session
{
public:
	~GameSession() {
		std::cout << "~GameSession" << std::endl;
	}

	virtual void OnConnected();
	virtual void OnDisconnected();
	virtual INT OnRecv(PBYTE Buffer, INT Length);
	virtual void OnSend(INT Length);
};


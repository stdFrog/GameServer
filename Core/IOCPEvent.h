#pragma once

class Session;

enum class EventType : UCHAR {
	CONNECT,
	DISCONNECT,
	ACCEPT,
	RECV,
	SEND
};

/*
	비동기 입출력에 필요한 OVERLAPPED 구조를 이용하여 발생한 이벤트의 종류를 구분하고,
	통신에 필요한 소켓 정보를 전달받는다.
*/
class IOCPEvent : public OVERLAPPED {
public:
	EventType _Type;
	std::shared_ptr<Session> _Session = NULL;
	std::shared_ptr<IOCPObject> _Owner = NULL;

	IOCPEvent(EventType);
	void Initialize();
};


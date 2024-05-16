#pragma once

class Session;
class IOCPObject;

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
	/* 보내야 하는 버퍼가 여러개 일 수 있으므로 벡터로 관리한다. */
	std::vector<std::shared_ptr<SendBuffer>> _SendBuffers;

public:
	EventType _Type;
	std::vector<BYTE> _Buffer;
	std::shared_ptr<Session> _Session = NULL;
	std::shared_ptr<IOCPObject> _Owner = NULL;

	IOCPEvent(EventType);
	void Initialize();
};


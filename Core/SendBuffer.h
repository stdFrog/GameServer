#pragma once

/*
	Send의 경우 브로드캐스팅 동작에서 여러 스레드를 깨워 수행할 가능성이 높으므로
	TLS에 저장하여 스레드 고유의 SendBuffer로 동작하게끔 만드는 것이 좋다.

	그런데, 현재 프로젝트에서 활용하고 있는 STL의 thread와 mutex는 유저 레벨로 예상되기 때문에 별 의미가 없다.
	
	Session에서 제공하는 인터페이스 곧, public 수준의 Send 함수를 보자.
	이 함수처럼 여러 스레드가 접근할 수 있는 공개된 인터페이스의 경우 인터락 함수와 TLS를 적절히 사용하는 것이 좋다.

	예로, 이벤트 등록 함수가 이에 해당되며 버퍼를 등록할 때
	스레드의 메모리 슬롯 곧, TLS를 얻어온 후 SendBuffer 객체를 저장하여
	스레드별로 고유의 SendBuffer 객체를 유지/관리 할 수 있을 것으로 보인다.
*/
class SendBuffer : std::enable_shared_from_this<SendBuffer>
{
	std::vector<BYTE> _Buffer;
	INT _WriteSize = 0;

public:
	BYTE* Buffer() { return _Buffer.data(); }
	INT WriteSize() { return _WriteSize; }
	INT Capacity() { return static_cast<INT>(_Buffer.size()); }

public:
	void CopyData(void* Data, INT Length);
	void Close(UINT WriteSize);

public:
	SendBuffer(INT BufferSize);
	~SendBuffer();
};


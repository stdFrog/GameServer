#pragma once

class IOCPEvent;

class IOCPObject : public std::enable_shared_from_this<IOCPObject> {
public:
	// virtual ~IOCPObject() = 0;
	virtual HANDLE GetHandle() = 0;
	virtual void Dispatch(IOCPEvent* NewEvent, DWORD dwTrans = 0) = 0;
};

/*
	Completion Port 모델 생성과 파괴 담당 클래스

	곧, 운영체제가 관리하는 입출력 완료 포트 큐를 생성할 때 사용된다.
	프로세스 한정적이며, 프로세스에 속한 스레드끼리는 공유 가능하다.

	여러 개 생성 가능하므로 사용자가 용도별로 추가하면 된다.
*/
class IOCPCore
{
	HANDLE _Handle;

public:
	HANDLE GetHandle() { return _Handle; }

public:
	/* 세션, 리스너 등 */
	bool Register(std::shared_ptr<IOCPObject> NewObject);
	BOOL Dispatch(DWORD dwMilliSeconds = INFINITE);
	
public:
	IOCPCore();
	~IOCPCore();
};

// extern IOCPCore GlobalCore;
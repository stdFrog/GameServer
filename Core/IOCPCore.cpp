#include "pch.h"
#include "IOCPCore.h"
#include "IOCPEvent.h"

/* 용도에 따라 여러개 운용해도 된다. */
IOCPCore GlobalCore;

IOCPCore::IOCPCore() {
	_Handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	assert(_Handle != INVALID_HANDLE_VALUE);
}

IOCPCore::~IOCPCore() {
	CloseHandle(_Handle);
}

BOOL IOCPCore::Register(std::shared_ptr<IOCPObject> NewObject) {
	/* 관찰 대상 등록 단계 */
	// return (BOOL)CreateIoCompletionPort(NewObject->GetHandle(), _Handle, (ULONG_PTR)NewObject, 0);
	
	/* 
		서비스와 스마트 포인터를 추가한 이후부턴 
		관리 대상이었던 리스너와 세션의 기반 클래스 곧, IOCPObject를 부가 정보로 전달하지 않는다.

		대신, Dispatch 함수에서 변화가 생긴다.
	*/
	return (BOOL)CreateIoCompletionPort(NewObject->GetHandle(), _Handle, 0, 0);
}

/* 리스너 또는 세션에 대한 Dispatch 함수를 호출한다. */
BOOL IOCPCore::Dispatch(DWORD dwMilliSeconds) {
	DWORD dwTrans = 0;
	ULONG_PTR Info;
	// IOCPObject* NewObject = NULL;
	IOCPEvent* NewEvent = NULL;
	std::shared_ptr<IOCPObject> NewObject = NULL;

	/* 
		StartAccept에서 IOCPCore의 Register 함수를 호출한다.

		서버 메인의 선두에서 관리 대상이 등록되며, AcceptEx가 모두 완료되었을 때
		곧, 비동기 함수가 완료되었을 때 운영체제가 프로그램에게 이 사실을 알리는 것으로 보인다.

		AcceptEx를 완전히 분석해봐야 상세한 구조를 파악할 수 있을 것이다.
	*/
	if (GetQueuedCompletionStatus(
		_Handle,
		&dwTrans,
		(PULONG_PTR)&Info,
		(LPOVERLAPPED*)&NewEvent,
		dwMilliSeconds
	)) {
		NewObject = NewEvent->_Owner;
		NewObject->Dispatch(NewEvent, dwTrans);
	}
	else {
		int Error = WSAGetLastError();

		switch (Error) {
		case WAIT_TIMEOUT:
			return FALSE;

		default:
			// TODO : 
			NewObject = NewEvent->_Owner;
			NewObject->Dispatch(NewEvent, dwTrans);
			break;
		}
	}

	return TRUE;
}
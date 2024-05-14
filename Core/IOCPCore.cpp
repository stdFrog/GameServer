#include "pch.h"
#include "IOCPCore.h"
#include "IOCPEvent.h"

/* 용도에 따라 여러개 운용해도 된다. */
// IOCPCore GlobalCore;

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

		AcceptEx가 호출된 이후 곧바로 리턴하면서 대기 상태에 빠지지 않으며,
		비동기 접속이 성공적으로 이루어지면 확장 함수(AcceptEx) 호출시 전달한 중첩 구조체가
		운영체제에 의해 내부적으로 사용된 뒤 GetQueuedCompletionStatus의 네 번째 인수로 다시 전달된다.

		이후 Dispatch 함수를 호출하는데 Listener와 Session이 그 대상이며,
		Session의 Dispatch 함수는 호환을 위해 간단한 래퍼 함수로 사용된다.
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
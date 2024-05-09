#include "pch.h"
#include "IOCPCore.h"

/* 용도에 따라 여러개 운용해도 된다. */
IOCPCore GlobalCore;

IOCPCore::IOCPCore() {
	_Handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	assert(_Handle != INVALID_HANDLE_VALUE);
}

IOCPCore::~IOCPCore() {
	CloseHandle(_Handle);
}

BOOL IOCPCore::Register(IOCPObject* NewObject) {
	/* 관찰 대상 등록 단계 */
	return (BOOL)CreateIoCompletionPort(NewObject->GetHandle(), _Handle, (ULONG_PTR)NewObject, 0);
}

/* 리스너 또는 세션에 대한 Dispatch 함수를 호출한다. */
BOOL IOCPCore::Dispatch(DWORD dwMilliSeconds) {
	DWORD dwTrans = 0;
	IOCPObject* NewObject = NULL;
	IOCPEvent* NewEvent = NULL;

	/* 
		StartAccept에서 IOCPCore의 Register 함수를 호출한다.

		서버 메인의 선두에서 관리 대상이 등록되며, AcceptEx가 모두 완료되었을 때
		곧, 비동기 함수가 완료되었을 때 운영체제가 프로그램에게 이 사실을 알리는 것으로 보인다.

		AcceptEx를 완전히 분석해봐야 상세한 구조를 파악할 수 있을 것이다.
	*/
	if (GetQueuedCompletionStatus(
		_Handle,
		&dwTrans,
		(PULONG_PTR)&NewObject,
		(LPOVERLAPPED*)&NewEvent,
		dwMilliSeconds
	)) {
		NewObject->Dispatch(NewEvent, dwTrans);
	}
	else {
		int Error = WSAGetLastError();

		switch (Error) {
		case WAIT_TIMEOUT:
			return FALSE;

		default:
			// TODO : 
			NewObject->Dispatch(NewEvent, dwTrans);
			break;
		}
	}

	return TRUE;
}
#include "pch.h"
#include "IOCPCore.h"

IOCPCore::IOCPCore() {
	_Handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	assert(_Handle != INVALID_HANDLE_VALUE);
}

IOCPCore::~IOCPCore() {
	CloseHandle(_Handle);
}

BOOL IOCPCore::Register(class IOCPObject* NewObject) {
	/* 관찰 대상 등록 단계 */
	return (BOOL)CreateIoCompletionPort(NewObject->GetHandle(), _Handle, (ULONG_PTR)NewObject, 0);
}

BOOL IOCPCore::Dispatch(DWORD dwMilliSeconds) {
	DWORD dwTrans = 0;
	IOCPObject* NewObject = NULL;
	IOCPEvent* NewEvent = NULL;

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
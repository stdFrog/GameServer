#include "pch.h"
#include "IOCPEvent.h"

IOCPEvent::IOCPEvent(EventType Type) : _Type(Type) {
	Initialize();
}

void IOCPEvent::Initialize() {
	/* 맨 마지막 멤버 변수를 제외한 나머지 멤버 변수는 운영체제 내부적으로만 사용된다. */
	Internal = 0;
	InternalHigh = 0;
	Offset = 0;
	OffsetHigh = 0;

	hEvent = NULL;
}
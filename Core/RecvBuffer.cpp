#include "pch.h"
#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(INT BufferSize) {
	_Capacity = BufferSize * BUFFER_COUNT;
	_Buffer.resize(_Capacity);
}

RecvBuffer::~RecvBuffer() {

}

void RecvBuffer::Clean() {
	INT RemDataSize = DataSize();

	if (RemDataSize == 0) {
		_Front = _Rear = 0;
	}
	else {
		if (FreeSize() < _BufferSize) {
			memcpy(&_Buffer[0], &_Buffer[_Rear], RemDataSize);
			_Rear = 0;
			_Front = RemDataSize;
		}
	}
}

BOOL RecvBuffer::OnRead(DWORD dwBytes) {
	if (dwBytes > DataSize()) { return FALSE; }

	_Rear += dwBytes;
	return TRUE;
}

BOOL RecvBuffer::OnWrite(DWORD dwBytes) {
	if (dwBytes > FreeSize()) { return FALSE; }

	_Front += dwBytes;
	return TRUE;
}
#include "pch.h"
#include "SendBuffer.h"

SendBuffer::SendBuffer(INT BufferSize) {
	_Buffer.resize(BufferSize);
}

SendBuffer::~SendBuffer() {
	
}

void SendBuffer::CopyData(void* Data, int Length) {
	assert(Capacity() >= Length);
	memcpy(_Buffer.data(), Data, Length);
	_WriteSize = Length;
}

void SendBuffer::Close(UINT WriteSize){
	/* 데이터 크기 조절을 위해 필요 */
	_WriteSize = WriteSize;
}
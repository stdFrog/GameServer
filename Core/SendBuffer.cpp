#include "pch.h"
#include "SendBuffer.h"

SendBuffer::SendBuffer(INT BufferSize) {

}

SendBuffer::~SendBuffer() {
	
}

void SendBuffer::CopyData(void* Data, int Length) {
	assert(Capacity() >= Length);
	memcpy(_Buffer.data(), Data, Length);
	_WriteSize = Length;
}
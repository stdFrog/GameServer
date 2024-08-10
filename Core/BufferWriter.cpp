#include "pch.h"
#include "BufferWriter.h"

BufferWriter::BufferWriter() {

}

BufferWriter::BufferWriter(BYTE* Buffer, UINT Size, UINT Posiiton) {

}

BufferWriter::~BufferWriter() {

}

BOOL BufferWriter::Write(void* Src, UINT Length) {
	if (FreeSize() < Length) { return FALSE; }

	memcpy(&_Buffer[_Position], Src, Length);
	_Position += Length;

	return TRUE;
}
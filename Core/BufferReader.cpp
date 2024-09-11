#include "pch.h"
#include "BufferReader.h"

BufferReader::BufferReader() {

}

BufferReader::BufferReader(BYTE* Buffer, UINT Size, UINT Position) : _Buffer(Buffer), _Size(Size), _Position(Position){

}

BufferReader::~BufferReader() {

}

BOOL BufferReader::Peek(void* Destination, UINT Length) {
	if (FreeSize() < Length) { return FALSE; }

	memcpy(Destination, &_Buffer[_Position], Length);
	return TRUE;
}

BOOL BufferReader::Read(void* Destination, UINT Length) {
	if (Peek(Destination, Length) == FALSE) { return FALSE; }

	_Position += Length;
	return TRUE;
}

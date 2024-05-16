#pragma once

/*
	
*/
class RecvBuffer
{
	const INT BUFFER_COUNT = 10;

	INT _Front = 0;
	INT _Rear = 0;
	INT _Capacity = 0;
	INT _BufferSize = 0;
	
	std::vector<BYTE> _Buffer;

public:
	void Clean();
	BOOL OnRead(DWORD dwBytes);
	BOOL OnWrite(DWORD dwBytes);

public:
	BYTE* ReadPosition() { return &_Buffer[_Rear]; }
	BYTE* WritePosition() { return &_Buffer[_Front]; }

public:
	INT DataSize() const { return _Front - _Rear; }
	INT FreeSize() const { return _Capacity - _Front; }

public:
	RecvBuffer(INT BufferSize);
	~RecvBuffer();
};


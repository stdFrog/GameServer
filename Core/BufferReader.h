#pragma once

/*
	현재 클라이언트와 서버는 약속된 구조의 패킷을 사용하기로 되어 있다.
	이 구조를 조금 더 쉽고 빠르게 만들기 위해 즉, 데이터를 편하게 조립할 수 있도록
	유틸리티 개념의 클래스를 추가했다.  
*/
class BufferReader
{
	BYTE* _Buffer = NULL;
	UINT _Size = 0;
	UINT _Position = 0;

public:
	BufferReader();
	BufferReader(BYTE* Buffer, UINT Size, UINT Position = 0);
	~BufferReader();

public:
	BYTE* Buffer() { return _Buffer; }
	UINT SIze() { return _Size; }
	UINT ReadSize() { return _Position; }
	UINT FreeSize() { return _Size - _Position; }

public:
	BOOL Peek(void* Destination, UINT Length);
	template<typename T> BOOL Peek(T* Destination) { return Peek(Destination, sizeof(T)); }

	BOOL Read(void* Destination, UINT Length);
	template<typename T>BOOL Read(void* Destination, UINT Length) { return Read(Destination, sizeof(T)); }

	template<typename T> BufferReader& operator>>(T& Destination);
};

template<typename T>
inline BufferReader& BufferReader::operator>>(T& Destination) {
	Destination = *reinterpret_cast<T*>(&_Buffer[_Position]);
	_Position += sizeof(T);
	return *this;
}

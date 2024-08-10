#pragma once

class BufferWriter
{
	BYTE* _Buffer = NULL;
	UINT _Size = 0;
	UINT _Position = 0;

public:
	BufferWriter();
	BufferWriter(BYTE* Buffer, UINT Size, UINT Posiiton = 0);
	~BufferWriter();

public:
	BYTE* Buffer() { return _Buffer; }
	UINT Size() { return _Size; }
	UINT WriteSize() { return _Position; }
	UINT FreeSize() { return _Size - _Position; }

	BOOL Write(void* Src, UINT Length);
	template<typename T> BOOL Write(T* Src) { return Write(Src, sizeof(T)); }

	template<typename T> T* Reserve();

	template<typename T> BufferWriter& operator <<(T&& Src);
};

template<typename T> T* BufferWriter::Reserve() {
	if ((_Size - _Position) < sizeof(T)) { return NULL; }
	
	T* ret = reinterpret_cast<T*>(&_Buffer[_Position]);
	_Position += sizeof(T);
	return ret;
}

template<typename T> BufferWriter& BufferWriter::operator <<(T&& Src) {
	/*
		T로 전달된 데이터의 타입이 레퍼런스 곧, 참조 형식일 경우 이를 비참조 형식으로 바꾼다.

		원형은 다음과 같으며 여러 형태가 있다.
		template<class T> struct remove_reference<T&> {typedef T type;}
		-> 특수화된 클래스이며 템플릿 타입으로 T&타입이 전달되면 일반 타입으로 변환된다.
	*/
	using DataType = std::remove_reference_t<T>;

	/*
		std::forward는 보편적 레퍼런스(Universal reference) 즉, rvalue를 전달하면
		적당히 변환해서 lvalue 또는 rvalue를 반환한다.

		std::move와 비슷한데 이동과 복사 연산을 호출하는 차이 정도만 있다.
	*/
	*(reinterpret_cast<DataType*>(&_Buffer[_Position])) = std::forward<DataType>(Src);
	_Position += sizeof(T);
	return *this;
}
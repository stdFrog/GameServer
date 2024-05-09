#include "pch.h"
#include "SocketTool.h"

LPFN_ACCEPTEX SocketTool::AcceptEx = NULL;
LPFN_CONNECTEX SocketTool::ConnectEx = NULL;
LPFN_DISCONNECTEX SocketTool::DisconnectEx = NULL;

void SocketTool::Initialize() {
	WSADATA wsa;
	assert(WSAStartup(MAKEWORD(2, 2), &wsa) == 0);

	SOCKET Dummy = CreateSocket();

	assert(BindWindowFunction(Dummy, WSAID_ACCEPTEX, reinterpret_cast<LPVOID*>(&AcceptEx)));
	assert(BindWindowFunction(Dummy, WSAID_CONNECTEX, reinterpret_cast<LPVOID*>(&ConnectEx)));
	assert(BindWindowFunction(Dummy, WSAID_DISCONNECTEX, reinterpret_cast<LPVOID*>(&DisconnectEx)));

	Close(Dummy);
}

void SocketTool::Clear() {
	WSACleanup();
}

BOOL SocketTool::Bind(SOCKET Socket, struct sockaddr_in Info) {
	return bind(Socket, (struct sockaddr*)&Info, sizeof(Info));
}

BOOL SocketTool::Bind(SOCKET Socket, NetAddress NewAddress) {
	return bind(Socket, (struct sockaddr*)&NewAddress.GetSocketInfo(), sizeof(struct sockaddr));
}

BOOL SocketTool::Listen(SOCKET Socket, UINT Maximum) {
	return listen(Socket, Maximum) != SOCKET_ERROR;
}

BOOL SocketTool::BindAnyAddress(SOCKET Socket, UINT Port) {
	struct sockaddr_in Local;
	Local.sin_family = AF_INET;
	Local.sin_addr.s_addr = htonl(INADDR_ANY);
	Local.sin_port = htons(Port);

	return bind(Socket, (struct sockaddr*)&Local, sizeof(Local)) != SOCKET_ERROR;
}

void SocketTool::Close(SOCKET& Socket) {
	if (Socket != INVALID_SOCKET) {
		closesocket(Socket);
		Socket = INVALID_SOCKET;
	}
}

BOOL SocketTool::SetTCPNoDelay(SOCKET Socket, BOOL bFlag) {
	return SetSocketOption(Socket, SOL_SOCKET, TCP_NODELAY, bFlag);
}

BOOL SocketTool::SetUpdateAcceptSocket(SOCKET Socket, SOCKET ListenSocket) {
	return SetSocketOption(Socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, ListenSocket);
}

BOOL SocketTool::SetLinger(SOCKET Socket, USHORT Turn, USHORT Linger) {
	LINGER Opt;

	Opt.l_linger = Linger;
	Opt.l_onoff = Turn;

	return SetSocketOption(Socket, SOL_SOCKET, SO_LINGER, Opt);
}

BOOL SocketTool::SetReuseAddress(SOCKET Socket, BOOL bFlag) {
	return SetSocketOption(Socket, SOL_SOCKET, SO_REUSEADDR, bFlag);
}

BOOL SocketTool::SetRecvBufferSize(SOCKET Socket, UINT Size) {
	return SetSocketOption(Socket, SOL_SOCKET, SO_RCVBUF, Size);
}

BOOL SocketTool::SetSendBufferSize(SOCKET Socket, UINT Size) {
	return SetSocketOption(Socket, SOL_SOCKET, SO_SNDBUF, Size);
}

SOCKET SocketTool::CreateSocket() {
	return WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}

/*
	소켓 모드 제어 함수

	윈도우 전용 소켓에 대한 모드 제어가 가능한 함수이며, 입력 버퍼에 대한 포인터로 GUID 곧, 전역 고유 식별자를 전달한다.

	GUID는 현재 수준에서 잘 사용되지 않는다.
	다중 상속, 중첩 클래스 등의 복잡한 구조를 가지는 COM에서나 볼 수 있다.

	GUID의 사용 이유는 비교적 단순한데, 구성 요소(Components)들의 인터페이스를 구별하기 위해서이다.
	여기서 말하는 인터페이스란 곧, 공개된 이름이나 정보 따위 등을 말한다.

	다시, 여기서 말하는 공개된 이름이나 정보는 모듈의 이름이나 모듈이 포함하는 구조체, 클래스,
	해당 클래스가 포함하는 중첩 클래스, 중첩 클래스와 같은 공간에 존재하는 다른 클래스,
	위 클래스의 public 멤버 변수와 함수 등이 모두 포함한다.

	곧, 대규모 프로그램이나 모듈을 제작할 때를 예로 들 수 있다.
	큰 규모의 프로젝트에선 이러한 인터페이스가 중복될 경우, 
	각각을 구별하기 위해 GUID가 흔히 사용되며 그 종류는 네 가지가 있다.

	- IID, CLSID, LIBID, CATID
	차례대로 인터페이스와 클래스, 타입 라이브러리, 카테고리의 식별자를 뜻한다.
	
	MSWSock 헤더나 dInput 헤더 등을 보면 GUID 구조를 살펴볼 수 있으며 비교적 간단한 구조를 가진다.
	4,2,2,8 크기 즉, 16바이트 크기의 구조체이며 실제 값을 보면
	{38a52be4-9352-453e-af97-5c3b448652f0}의 꼴을 가진다는 것을 알 수 있다.

	더 자세한 내용은 관련 토픽을 읽어보기로 하고, 특별히 규정된 포맷이 없다는 것과
	로컬 또는 시스템 전역적으로 사용하는 고유 번호라는 것만 기억해두자.
*/
BOOL SocketTool::BindWindowFunction(SOCKET Socket, GUID Serial, LPVOID* fn) {
	DWORD dwBytes = 0;

	/*
		함수의 세 번째 인수를 보면 입력 버퍼에 대한 포인터로 GUID를 전달받는데,
		이때 전달하는 값은 시스템 전역적으로 존재하는 함수 식별자인 것으로 보인다.

		포인터 타입의 매개변수는 입출력 인수이며 GUID를 전달하면 확장 함수의 주소를 리턴하여
		사용자가 포인터로 그 주소 공간에 접근할 수 있도록 만들었다.

		이 함수로 전달된 소켓과 함수는 여러가지 속성을 가지는 것으로 보인다.
		
		자세한건 이후 리팩토링할 때 관련 레퍼런스를 읽어보기로 한다.
	*/
	return WSAIoctl(
		Socket,										// 대상 소켓
		SIO_GET_EXTENSION_FUNCTION_POINTER,			// 입출력 컨트롤 옵션이자 제어 코드
		&Serial,									// 입력 버퍼에 대한 포인터
		sizeof(Serial),								// 입력 버퍼의 크기(바이트)
		fn,											// 출력 버퍼에 대한 포인터
		sizeof(*fn),								// 출력 버퍼의 크기(바이트)
		&dwBytes,									// 실제 입출력 바이트 수
		NULL,										// 비동기 구조체
		NULL) != SOCKET_ERROR;						// 콜백 함수
}
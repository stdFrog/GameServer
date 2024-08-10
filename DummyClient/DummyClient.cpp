#include "pch.h"
#include <iostream>
#include <chrono>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "ClientPacketHandler.h"

#define SERVERPORT 9000
using std::this_thread::sleep_for;

char SendData[] = "Hello World";

class ServerSession : public PacketSession {
public:
	~ServerSession() {
		std::cout << "~ServerSession" << std::endl;
	}

	virtual void OnConnected() {
		std::cout << "Connected To Server" << std::endl;

		/*std::shared_ptr<SendBuffer> NewBuffer = std::make_shared<SendBuffer>(0x1000);
		NewBuffer->CopyData(SendData, sizeof(SendData));
		Send(NewBuffer);*/
	}

	// virtual INT OnRecv(PBYTE Buffer, INT Length) {
	virtual void OnRecvPacket(PBYTE Buffer, INT Length) {
		std::cout << "OnRecv Length = " << Length << std::endl;

		// 서버에서 브로드캐스트로 정보를 뿌릴 때 주로 사용된다.
		ClientPacketHandler::HandlePacket(Buffer, Length);
		
		/*sleep_for(std::chrono::milliseconds(100));

		std::shared_ptr<SendBuffer> NewBuffer = std::make_shared<SendBuffer>(0x1000);
		NewBuffer->CopyData(SendData, sizeof(SendData));
		Send(NewBuffer);

		return Length;*/
	}

	virtual void OnSend(INT Length) {
		std::cout << "OnSend Length = " << Length << std::endl;
	}

	virtual void OnDisconnected() {
		std::cout << "Disconnected" << std::endl;
	}
};

int main()
{
	SocketTool::Initialize();

	sleep_for(std::chrono::milliseconds(2000));

	std::shared_ptr<ClientService> Service = std::make_shared<ClientService>(
		NetAddress(L"127.0.0.1", SERVERPORT),
		std::make_shared<IOCPCore>(),
		[]() {return std::make_shared<ServerSession>(); },
		5
	);

	assert(Service->Start());
	/*
		금일, 디버깅 결과 : 
		(1). 서버측 오류 해결 -> bind 함수 에러코드 분기 수정
		(2). 더미 클라이언트 unlock of unowned mutex 현상 발생 -> 스레드 매니저 클래스의 Launch 호출로부터
		뮤텍스 unlock이 호출되지 않는 현상 발견 추후 수정 필요
	*/
	for (INT i = 0; i < 5; i++){
		GThreadManager->Launch([=]() {
			while (1) {
				Service->GetMainCore()->Dispatch();
			}
		});
	}
	GThreadManager->Join();
	SocketTool::Clear();
}
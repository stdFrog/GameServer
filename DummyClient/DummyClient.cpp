#include "pch.h"
#include <iostream>
#include <chrono>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"

#define SERVERPORT 9000
using std::this_thread::sleep_for;

char SendData[] = "Hello World";

class ServerSession : public Session {
public:
	~ServerSession() {
		std::cout << "~ServerSession" << std::endl;
	}

	virtual void OnConnected() {
		std::cout << "Connected To Server" << std::endl;

		std::shared_ptr<SendBuffer> NewBuffer = std::make_shared<SendBuffer>(0x1000);
		NewBuffer->CopyData(SendData, sizeof(SendData));
		Send(NewBuffer);
	}

	virtual INT OnRecv(PBYTE Buffer, INT Length) {
		std::cout << "OnRecv Length = " << Length << std::endl;

		sleep_for(std::chrono::milliseconds(100));

		std::shared_ptr<SendBuffer> NewBuffer = std::make_shared<SendBuffer>(0x1000);
		NewBuffer->CopyData(SendData, sizeof(SendData));
		Send(NewBuffer);

		return Length;
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
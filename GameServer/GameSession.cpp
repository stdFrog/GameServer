#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"

void GameSession::OnConnected() {
	GSessionManager.Add(std::static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnDisconnected() {
	GSessionManager.Remove(std::static_pointer_cast<GameSession>(shared_from_this()));
}

// INT GameSession::OnRecv(PBYTE Buffer, INT Length) {
void GameSession::OnRecvPacket(PBYTE Buffer, INT Length) {
	PacketHeader Header = *((PacketHeader*)Buffer);
	std::cout << "Packet ID : " << Header.ID << "Size : " << Header.Size << std::endl;

	/*
		switch문으로 분기하는 것이 애매한 상황이므로 핸들러를 만들어 관리한다.
	*/

	/*std::cout << "OnRecv Length = " << Length << std::endl;

	std::shared_ptr<SendBuffer> NewBuffer = std::make_shared<SendBuffer>(0x1000);
	NewBuffer->CopyData(Buffer, Length);

	GSessionManager.Broadcast(NewBuffer);

	return Length;*/
}

void GameSession::OnSend(INT Length) {
	// std::cout << "OnSend Length = " << Length << std::endl;
}
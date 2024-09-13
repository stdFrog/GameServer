#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"

void ServerPacketHandler::HandlePacket(BYTE* Buffer, int Length) {
	BufferReader br(Buffer, Length);

	PacketHeader Header;
	br.Peek(&Header);

	switch (Header.ID) {
	default:
		break;
	}
}

// [Buf Size | Buf ID][ID | HP | Attack]
/* 
	현재 구조만 보면 게임에 접속한 대상에게 ID, HP 따위의 정보를 갱신하는 정도의 용도로만
	쓰이는 브로드캐스트용 함수라고 할 수 있다.

	송/수신간 주고받는 구조화된 데이터가 서로 다르거나, 순서가 약간 바뀌었을 경우에도
	문제가 발생할 수 있으므로 주의해야 한다.
*/
/*
std::shared_ptr<SendBuffer> ServerPacketHandler::Make_S_TEST(unsigned long long ID, UINT HP, USHORT Attack, std::vector<BufferData> Buffers) {
	std::shared_ptr<SendBuffer> NewBuffer = std::make_shared<SendBuffer>(0x1000);

	BufferWriter bw(NewBuffer->Buffer(), NewBuffer->Capacity());
	PacketHeader* Header = bw.Reserve<PacketHeader>();

	bw << ID << HP << Attack;

	bw << (USHORT)Buffers.size();
	for (BufferData& Buf : Buffers) {
		bw << Buf.BufferID << Buf.RemainTime;
	}

	Header->Size = bw.WriteSize();
	Header->ID = S_TEST;

	NewBuffer->Close(bw.WriteSize());

	return NewBuffer;
}
*/


/*
	혼합 프로그래밍시, 예를 들어 C++ 서버와 유니티를 연결한다고 해보자.
	유니티는 C#을 사용하므로 데이터 타입을 읽고 해석하는데에 차이가 있을 수 있다.
	이를 중간에서 중재해줄 방법이 필요한데 흔히 DB에서 발생하는 문제와 같다고 할 수 있다.

	이 프로젝트에선 구글의 프로토버퍼을 활용해 패킷 데이터를 생성하고 통신 하는 방법에 대해 공부한다.
*/
std::shared_ptr<SendBuffer> ServerPacketHandler::Make_S_TEST(unsigned long long ID, UINT HP, USHORT Attack, std::vector<BufferData> Buffers)
{
	Protocol::S_TEST pkt;

	pkt.set_id(10);
	pkt.set_hp(100);
	pkt.set_attack(10);

	{
		Protocol::BuffData* data = pkt.add_buffs();
		data->set_buffid(100);
		data->set_remaintime(1.2f);
		{
			data->add_victims(10);
		}
	}
	{
		Protocol::BuffData* data = pkt.add_buffs();
		data->set_buffid(200);
		data->set_remaintime(2.2f);
		{
			data->add_victims(20);
		}
	}

	return MakeSendBuffer(pkt, S_TEST);
}
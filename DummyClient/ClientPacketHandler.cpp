#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"

//struct BufferData {
//	unsigned long long BufferID;
//	float RemainTime;
//};
//
//struct S_TEST {
//	unsigned long long ID;
//	UINT HP;
//	USHORT Attack;
//
//	// 가변데이터
//	// 1) 문자열 (ex. name)
//	// 2) 그냥 바이트 배열 (ex. 길드 이미지)
//	// 3) 일반 리스트
//
//	std::vector<BufferData> Buffers;
//};

void ClientPacketHandler::HandlePacket(BYTE* Buffer, INT Length) {
	BufferReader br(Buffer, Length);

	PacketHeader Header;
	br >> Header;

	switch (Header.ID) {
	case S_TEST:
		Handle_S_TEST(Buffer, Length);
		break;
	default:
		break;
	}
}

/*
void ClientPacketHandler::Handle_S_TEST(BYTE* Buffer, INT Length) {
	BufferReader br(Buffer, Length);

	PacketHeader Header;
	br >> Header;

	unsigned long long ID;
	UINT HP;
	USHORT Attack;

	br >> ID >> HP >> Attack;

	std::cout << "ID : " << ID << "HP : " << HP << "ATT : " << Attack << std::endl;

	std::vector<BufferData> Buffers;
	USHORT BufferCount;
	br >> BufferCount;

	Buffers.resize(BufferCount);
	for (int i = 0; i < BufferCount; i++) {
		br >> Buffers[i].BufferID >> Buffers[i].RemainTime;
	}

	// 서버로부터 데이터를 전송받아 게임내 정보를 갱신한 이후 클라이언트가 해야될 처리를 이곳에 추가하면 된다.

}
*/

void ClientPacketHandler::Handle_S_TEST(BYTE* buffer, INT len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	//uint16 id = header->id;
	USHORT size = header->Size;

	Protocol::S_TEST pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	unsigned long long id = pkt.id();
	UINT hp = pkt.hp();
	USHORT attack = pkt.attack();

	std::cout << "ID: " << id << " HP : " << hp << " ATT : " << attack << std::endl;

	for (int i = 0; i < pkt.buffs_size(); i++)
	{
		const Protocol::BuffData& data = pkt.buffs(i);
		std::cout << "BuffInfo : " << data.buffid() << " " << data.remaintime() << std::endl;
	}
}
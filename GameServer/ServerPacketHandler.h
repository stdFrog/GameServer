#pragma once

/*
	래퍼 클래스로 단순히 패킷을 쉽게 만들 때 사용된다. 
*/
enum { S_TEST = 1 };

struct BufferData {
	unsigned long long BufferID;
	float RemainTime;
};

class ServerPacketHandler
{
public:
	static void HandlePacket(BYTE* Buffer, int Length);
	static std::shared_ptr<SendBuffer> Make_S_TEST(unsigned long long ID, UINT HP, USHORT Attack, std::vector<BufferData> Buffers);
};
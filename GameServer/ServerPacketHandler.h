#pragma once

/*
	래퍼 클래스로 클라이언트는 단순히 해당 클래스의 함수를 호출하기만 하면 된다.
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
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

	template <typename T>
	static std::shared_ptr<SendBuffer> MakeSendBuffer(T& Packet, USHORT PacketID) {
		const USHORT DataSize = static_cast<USHORT>(Packet.ByteSizeLong());
		const USHORT PacketSize = DataSize + sizeof(PacketHeader);

		std::shared_ptr<SendBuffer> NewBuffer = std::make_shared<SendBuffer>(PacketSize);
		PacketHeader* Header = reinterpret_cast<PacketHeader*>(NewBuffer->Buffer());

		Header->Size = PacketSize;
		Header->ID = PacketID;

		assert(Packet.SerializeToArray(&Header[1], DataSize));
		NewBuffer->Close(PacketSize);

		return NewBuffer;
	}
};
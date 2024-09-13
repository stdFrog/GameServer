#pragma once

enum { S_TEST = 1 };

class ClientPacketHandler
{
public:
	static void HandlePacket(BYTE* Buffer, INT Length);
	static void Handle_S_TEST(BYTE* Buffer, INT Length);

	template<typename T>
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
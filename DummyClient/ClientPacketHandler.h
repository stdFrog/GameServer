#pragma once

enum { S_TEST = 1 };

class ClientPacketHandler
{
public:
	static void HandlePacket(BYTE* Buffer, INT Length);
	static void Handle_S_TEST(BYTE* Buffer, INT Length);
};
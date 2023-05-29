#include "stdafx.h"
#include "packetNetworks.h"


VOID Make_PACKET_SC_CREATE_CHARACTER_MINE(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY, BYTE hp)
{
	PACKET_HEADER packetHeader;
	packetHeader.byteCode = PACKET_CODE;
	packetHeader.byteSize = sizeof(PACKET_SC_CREATE_CHARACTER_MINE) - sizeof(PACKET_HEADER);
	packetHeader.byteType = PACKET_SC_CREATE_CHARACTER_MINE_;
	*serializedBuffer << packetHeader.byteCode << packetHeader.byteSize << packetHeader.byteType << id << direction << posX << posY << hp;
	return;
}

VOID Make_PACKET_SC_CREATE_CHARACTER_OTHERS(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY, BYTE hp)
{
	PACKET_HEADER packetHeader;
	packetHeader.byteCode = PACKET_CODE;
	packetHeader.byteSize = sizeof(PACKET_SC_CREATE_CHARACTER_OTHERS) - sizeof(PACKET_HEADER);
	packetHeader.byteType = PACKET_SC_CREATE_CHARACTER_OTHERS_;
	*serializedBuffer << packetHeader.byteCode << packetHeader.byteSize << packetHeader.byteType << id << direction << posX << posY << hp;
	return;
}

VOID Make_PACKET_SC_DELETE_CHARACTER(SerializedBuffer *serializedBuffer, INT32 id)
{
	PACKET_HEADER packetHeader;
	packetHeader.byteCode = PACKET_CODE;
	packetHeader.byteSize = sizeof(PACKET_SC_DELETE_CHARACTER) - sizeof(PACKET_HEADER);
	packetHeader.byteType = PACKET_SC_DELETE_CHARACTER_;
	*serializedBuffer << packetHeader.byteCode << packetHeader.byteSize << packetHeader.byteType << id;
	return;
}

VOID Make_PACKET_SC_MOVE_START(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY)
{
	PACKET_HEADER packetHeader;
	packetHeader.byteCode = PACKET_CODE;
	packetHeader.byteSize = sizeof(PACKET_SC_MOVE_START) - sizeof(PACKET_HEADER);
	packetHeader.byteType = PACKET_SC_MOVE_START_;
	*serializedBuffer << packetHeader.byteCode << packetHeader.byteSize << packetHeader.byteType << id << direction << posX << posY;
	return;
}

VOID Make_PACKET_SC_MOVE_STOP(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY)
{
	PACKET_HEADER packetHeader;
	packetHeader.byteCode = PACKET_CODE;
	packetHeader.byteSize = sizeof(PACKET_SC_MOVE_STOP) - sizeof(PACKET_HEADER);
	packetHeader.byteType = PACKET_SC_MOVE_STOP_;
	*serializedBuffer << packetHeader.byteCode << packetHeader.byteSize << packetHeader.byteType << id << direction << posX << posY;
	return;
}

VOID Make_PACKET_SC_ATTACK1(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY)
{
	PACKET_HEADER packetHeader;
	packetHeader.byteCode = PACKET_CODE;
	packetHeader.byteSize = sizeof(PACKET_SC_ATTACK1) - sizeof(PACKET_HEADER);
	packetHeader.byteType = PACKET_SC_ATTACK1_;
	*serializedBuffer << packetHeader.byteCode << packetHeader.byteSize << packetHeader.byteType << id << direction << posX << posY;
	return;
}

VOID Make_PACKET_SC_ATTACK2(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY)
{
	PACKET_HEADER packetHeader;
	packetHeader.byteCode = PACKET_CODE;
	packetHeader.byteSize = sizeof(PACKET_SC_ATTACK2) - sizeof(PACKET_HEADER);
	packetHeader.byteType = PACKET_SC_ATTACK2_;
	*serializedBuffer << packetHeader.byteCode << packetHeader.byteSize << packetHeader.byteType << id << direction << posX << posY;
	return;
}

VOID Make_PACKET_SC_ATTACK3(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY)
{
	PACKET_HEADER packetHeader;
	packetHeader.byteCode = PACKET_CODE;
	packetHeader.byteSize = sizeof(PACKET_SC_ATTACK3) - sizeof(PACKET_HEADER);
	packetHeader.byteType = PACKET_SC_ATTACK3_;
	*serializedBuffer << packetHeader.byteCode << packetHeader.byteSize << packetHeader.byteType << id << direction << posX << posY;
	return;
}

VOID Make_PACKET_SC_DAMAGE(SerializedBuffer *serializedBuffer, INT32 idAttacker, INT32 idDamager, BYTE hpDamagerRemain)
{
	PACKET_HEADER packetHeader;
	packetHeader.byteCode = PACKET_CODE;
	packetHeader.byteSize = sizeof(PACKET_SC_DAMAGE) - sizeof(PACKET_HEADER);
	packetHeader.byteType = PACKET_SC_DAMAGE_;
	*serializedBuffer << packetHeader.byteCode << packetHeader.byteSize << packetHeader.byteType << idAttacker << idDamager << hpDamagerRemain;
	return;
}

VOID Make_PACKET_SC_SYNC(SerializedBuffer *serializedBuffer, INT32 id, USHORT posX, USHORT posY)
{
	PACKET_HEADER packetHeader;
	packetHeader.byteCode = PACKET_CODE;
	packetHeader.byteSize = sizeof(PACKET_SC_SYNC) - sizeof(PACKET_HEADER);
	packetHeader.byteType = PACKET_SC_SYNC_;
	*serializedBuffer << packetHeader.byteCode << packetHeader.byteSize << packetHeader.byteType << id << posX << posY;
	return;
}

VOID Make_PACKET_SC_ECHO(SerializedBuffer *serializedBuffer, DWORD time)
{
	PACKET_HEADER packetHeader;
	packetHeader.byteCode = PACKET_CODE;
	packetHeader.byteSize = sizeof(PACKET_SC_ECHO) - sizeof(PACKET_HEADER);
	packetHeader.byteType = PACKET_SC_ECHO_;
	*serializedBuffer << packetHeader.byteCode << packetHeader.byteSize << packetHeader.byteType << time;
	return;
}

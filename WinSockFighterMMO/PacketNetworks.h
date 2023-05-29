#pragma once

#include "PacketDefine.h"
#include "SerializedBuffer.h"
#include "Player.h"

VOID	Make_PACKET_SC_CREATE_CHARACTER_MINE(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY, BYTE hp);
VOID	Make_PACKET_SC_CREATE_CHARACTER_OTHERS(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY, BYTE hp);
VOID	Make_PACKET_SC_DELETE_CHARACTER(SerializedBuffer *serializedBuffer, INT32 id);
VOID	Make_PACKET_SC_MOVE_START(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY);
VOID	Make_PACKET_SC_MOVE_STOP(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY);
VOID	Make_PACKET_SC_ATTACK1(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY);
VOID	Make_PACKET_SC_ATTACK2(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY);
VOID	Make_PACKET_SC_ATTACK3(SerializedBuffer *serializedBuffer, INT32 id, BYTE direction, USHORT posX, USHORT posY);
VOID	Make_PACKET_SC_DAMAGE(SerializedBuffer *serializedBuffer, INT32 idAttacker, INT32 idDamager, BYTE hpDamagerRemain);
VOID	Make_PACKET_SC_SYNC(SerializedBuffer *serializedBuffer, INT32 id, USHORT posX, USHORT posY);
VOID	Make_PACKET_SC_ECHO(SerializedBuffer *serializedBuffer, DWORD time);

VOID	On_PACKET_CS_MOVE_START(Player *player, BYTE direction, USHORT posX, USHORT posY);
VOID	On_PACKET_CS_MOVE_STOP(Player *player, BYTE direction, USHORT posX, USHORT posY);
VOID	On_PACKET_CS_ATTACK1(Player *player, BYTE direction, USHORT posX, USHORT posY);
VOID	On_PACKET_CS_ATTACK2(Player *player, BYTE direction, USHORT posX, USHORT posY);
VOID	On_PACKET_CS_ATTACK3(Player *player, BYTE direction, USHORT posX, USHORT posY);
VOID	On_PACKET_CS_ECHO(Player *player, DWORD time);

/*
VOID	On_PACKET_CS_MOVE_START(Player *player, SerializedBuffer *serializedBuffer);
VOID	On_PACKET_CS_MOVE_STOP(Player *player, SerializedBuffer *serializedBuffer);
VOID	On_PACKET_CS_ATTACK1(Player *player, SerializedBuffer *serializedBuffer);
VOID	On_PACKET_CS_ATTACK2(Player *player, SerializedBuffer *serializedBuffer);
VOID	On_PACKET_CS_ATTACK3(Player *player, SerializedBuffer *serializedBuffer);
VOID	On_PACKET_CS_ECHO(Player *player, SerializedBuffer *serializedBuffer);
*/
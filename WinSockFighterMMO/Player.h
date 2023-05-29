#pragma once

#include "Session.h"
#include "PacketDefine.h"
#include "Map.h"

namespace azely {

	enum PlayerStatus
	{
		STATUS_NONE = 0,
		STATUS_MOVE = 1,
		STATUS_STOP = 2,
		STATUS_ATTACK1 = 3,
		STATUS_ATTACK2 = 4,
		STATUS_ATTACK3 = 5,
		STATUS_DELETE = 9
	};

	enum PlayerDirection
	{
		DIRECTION_LL = PACKET_MOVE_DIR_LL_,
		DIRECTION_LU = PACKET_MOVE_DIR_LU_,
		DIRECTION_UU = PACKET_MOVE_DIR_UU_,
		DIRECTION_RU = PACKET_MOVE_DIR_RU_,
		DIRECTION_RR = PACKET_MOVE_DIR_RR_,
		DIRECTION_RD = PACKET_MOVE_DIR_RD_,
		DIRECTION_DD = PACKET_MOVE_DIR_DD_,
		DIRECTION_LD = PACKET_MOVE_DIR_LD_
	};

	struct Player {

		enum Constants
		{
			SPEED_X_PER_SEC = 150,
			SPEED_Y_PER_SEC = 100,
			ATTACK1_RANGE_X = 80,
			ATTACK1_RANGE_Y = 10,
			ATTACK1_DAMAGE = 4,
			ATTACK2_RANGE_X = 90,
			ATTACK2_RANGE_Y = 10,
			ATTACK2_DAMAGE = 6,
			ATTACK3_RANGE_X = 100,
			ATTACK3_RANGE_Y = 20,
			ATTACK3_DAMAGE = 8
		};

		Session			*session;
		DWORD			playerID;
		PlayerStatus	status;
		PlayerDirection	direction;
		DOUBLE			posX;
		DOUBLE			posY;
		BYTE			hp;
		MapSector		sector;
	};

	Player		*FindPlayer(INT32 playerID);
	Player		*FindPlayer(SOCKET socket);
	Player		*CreatePlayer(Session *session);
	VOID		RemovePlayer(INT32 playerID);
	VOID		RemovePlayer(Player *player);
	VOID		RemovePlayer(SOCKET socket);

	inline PlayerDirection getDirectionByLR(PlayerDirection direction)
	{
		if (direction == DIRECTION_LL) return DIRECTION_LL;
		if (direction == DIRECTION_LU) return DIRECTION_LL;
		if (direction == DIRECTION_UU) return DIRECTION_LL;
		if (direction == DIRECTION_RU) return DIRECTION_RR;
		if (direction == DIRECTION_RR) return DIRECTION_RR;
		if (direction == DIRECTION_RD) return DIRECTION_RR;
		if (direction == DIRECTION_DD) return DIRECTION_RR;
		if (direction == DIRECTION_LD) return DIRECTION_LL;
		return DIRECTION_LL;
	}

	inline int getDirectionXMultiplier(PlayerDirection direction)
	{
		if (direction == DIRECTION_LL) return -1;
		if (direction == DIRECTION_LU) return -1;
		if (direction == DIRECTION_UU) return 0;
		if (direction == DIRECTION_RU) return 1;
		if (direction == DIRECTION_RR) return 1;
		if (direction == DIRECTION_RD) return 1;
		if (direction == DIRECTION_DD) return 0;
		if (direction == DIRECTION_LD) return -1;
		return 0;
	}

	inline int getDirectionYMultiplier(PlayerDirection direction)
	{
		if (direction == DIRECTION_LL) return 0;
		if (direction == DIRECTION_LU) return -1;
		if (direction == DIRECTION_UU) return -1;
		if (direction == DIRECTION_RU) return -1;
		if (direction == DIRECTION_RR) return 0;
		if (direction == DIRECTION_RD) return 1;
		if (direction == DIRECTION_DD) return 1;
		if (direction == DIRECTION_LD) return 1;
		return 0;
	}

	BOOL GetAttackResult(const Player *attacker, const Player *checker, INT32 rangeX, INT32 rangeY);

}

extern SimpleLogger						*logger;
extern INT32							playerNextID;
extern MemoryPool<Player>				playerPool;
extern unordered_map<INT32, Player *>	playerMap;
extern unordered_map<SOCKET, Player *>	playerSocketMap;
extern unordered_map<INT32, Player *>	playerSectorMap[SECTOR_INDEX_SIZE_Y][SECTOR_INDEX_SIZE_X];
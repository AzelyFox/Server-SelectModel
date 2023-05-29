#include "stdafx.h"
#include "Player.h"
#include "PacketDefine.h"

namespace azely {

	Player *FindPlayer(INT32 playerID)
	{
		auto findPlayerResult = playerMap.find(playerID);
		if (findPlayerResult == playerMap.end()) {
			WCHAR buffer[32] = { 0, };
			_snwprintf_s(buffer, 32 * 2, L"FindPlayer(ID) %d not found", playerID);
			logger->SaveLogWarning(L"FindPlayer", buffer);
			return nullptr;
		}
		return findPlayerResult->second;
	}

	Player *FindPlayer(SOCKET socket)
	{
		auto findPlayerResult = playerSocketMap.find(socket);
		if (findPlayerResult == playerSocketMap.end()) {
			WCHAR buffer[32] = { 0, };
			_snwprintf_s(buffer, 32 * 2, L"FindPlayer(SOCKET) %d not found", socket);
			logger->SaveLogWarning(L"FindPlayer", buffer);
			return nullptr;
		}
		return findPlayerResult->second;
	}

	Player *CreatePlayer(Session *session)
	{
		Player *newPlayer = playerPool.Alloc();

		newPlayer->session = session;
		newPlayer->hp = 100;
		//newPlayer->posX = 0;
		//newPlayer->posY = 0;
		newPlayer->posX = rand() % Map::X_MAX;
		newPlayer->posY = rand() % Map::Y_MAX;
		newPlayer->status = STATUS_NONE;
		newPlayer->direction = rand() % 2 == 0 ? DIRECTION_LL : DIRECTION_RR;
		newPlayer->playerID = playerNextID++;
		newPlayer->sector.x = GetMapSectorIndexX(newPlayer->posX);
		newPlayer->sector.y = GetMapSectorIndexY(newPlayer->posY);

		playerMap[newPlayer->playerID] = newPlayer;
		playerSocketMap[newPlayer->session->socket] = newPlayer;
		/*  OK
		*   1. 접속한 사람에게 섹터 할당
		*/
		playerSectorMap[newPlayer->sector.y][newPlayer->sector.x].insert(pair<INT32, Player *>(newPlayer->playerID, newPlayer));

		return newPlayer;
	}

	VOID RemovePlayer(SOCKET socket)
	{
		auto findPlayerResult = playerSocketMap.find(socket);
		if (findPlayerResult == playerSocketMap.end()) {
			WCHAR buffer[32] = { 0, };
			_snwprintf_s(buffer, 32 * 2, L"RemovePlayer(SOCKET) %d not found", socket);
			logger->SaveLogWarning(L"RemovePlayer", buffer);
			return;
		}

		Player *removingPlayer = findPlayerResult->second;

		RemovePlayer(removingPlayer);
	}

	VOID RemovePlayer(INT32 playerID)
	{
		auto findPlayerResult = playerMap.find(playerID);
		if (findPlayerResult == playerMap.end()) {
			WCHAR buffer[32] = { 0, };
			_snwprintf_s(buffer, 32 * 2, L"RemovePlayer(ID) %d not found", playerID);
			logger->SaveLogWarning(L"RemovePlayer", buffer);
			return;
		}

		Player *removingPlayer = findPlayerResult->second;

		RemovePlayer(removingPlayer);
	}

	VOID RemovePlayer(Player *player)
	{
		playerMap.erase(player->playerID);
		playerSocketMap.erase(player->session->socket);
		playerSectorMap[player->sector.y][player->sector.x].erase(player->playerID);
		playerPool.Free(player);
	}

	BOOL GetAttackResult(const Player *attacker, const Player *checker, INT32 rangeX, INT32 rangeY)
	{
		if (attacker == nullptr || checker == nullptr) return false;
		if (getDirectionByLR(attacker->direction) == DIRECTION_LL)
		{
			if (attacker->posX < checker->posX) return false;
			if (attacker->posX - rangeX > checker->posX) return false;
		}
		else
		{
			if (attacker->posX > checker->posX) return false;
			if (attacker->posX + rangeX < checker->posX) return false;
		}
		if (attacker->posY + (rangeY / 2) < checker->posY) return false;
		if (attacker->posY - (rangeY / 2) > checker->posY) return false;
		return true;
	}

}
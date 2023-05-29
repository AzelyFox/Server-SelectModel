// WinSockFighterMMO.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include "stdafx.h"

#include <Psapi.h>

#define DEBUG_LOGGING_
#define TCP_NAGLE_OFF

#include "SimpleLogger.h"
#include "MemoryPool.h"
#include "SerializedBuffer.h"
#include "SimpleLogger.h"
#include "RingBuffer.h"
#include "PacketDefine.h"
#include "PacketNetworks.h"
#include "Constants.h"

#include "Player.h"
#include "Session.h"
#include "Map.h"

#define INITIAL_PLAYER_POOL 6000
#define INITIAL_SESSION_POOL 6000
#define INITIAL_PACKET_POOL 100
#define SERVER_FPS 50

SimpleLogger *logger;

MemoryPool<SerializedBuffer> packetPool = MemoryPool<SerializedBuffer>(false, INITIAL_PACKET_POOL);
MemoryPool<Session> sessionPool = MemoryPool<Session>(false, INITIAL_SESSION_POOL);
MemoryPool<Player> playerPool = MemoryPool<Player>(false, INITIAL_PLAYER_POOL);
unordered_map<SOCKET, Session *> sessionMap;
unordered_map<INT32, Player *> playerMap;
unordered_map<SOCKET, Player *> playerSocketMap;
unordered_map<INT32, Player *> playerSectorMap[SECTOR_INDEX_SIZE_Y][SECTOR_INDEX_SIZE_X];
//set<Session *> sessionSendSet;

BOOL        LoadData();
BOOL        InitializeData();
BOOL        NetSetUp();
VOID        NetCleanUp();
BOOL        ProcessNetwork();
BOOL        ProcessLogic();
VOID        ProcessLogicDisconnect();
VOID        ProcessLogicPlayerMove(DWORD timeElapsed);
VOID        ProcessLogicPlayerMove(DWORD timeElapsed, Player *player, DOUBLE movingX, DOUBLE movingY);
BOOL        ProcessControl();
VOID        ProcessBySecond();
VOID        ProcessMonitor();
VOID        ProcessShutDown();

INT32       NetworkSelect(FD_SET *readSet, FD_SET *writeSet, TIMEVAL *timeValue);
VOID        NetworkAccept(SOCKET socket);
VOID        NetworkAcceptPlayer(SOCKET socket, Player *player);
VOID        NetworkAcceptMonitor(SOCKET socket);
//VOID        NetworkSend(SOCKET socket);
VOID        NetworkSend(Session *session);
//VOID        NetworkRecv(SOCKET socket);
VOID        NetworkRecv(Session *session);
VOID        NetworkPacket(Player *player, PACKET_HEADER *netHeader, SerializedBuffer *packet);
VOID        SendPacketUnicast(Player *receiver, SerializedBuffer *packet);
VOID        SendPacketBroadcast(const Player *except, SerializedBuffer *packet);
VOID        SendPacketBroadcastSector(const Player *except, MapSectorGroup *sectors, SerializedBuffer *packet);
VOID        DisconnectClient(Player *player);
VOID        GetSectorNear(MapSector sector, MapSectorGroup *outSectorAround);
VOID        GetSectorChanges(MapSector sectorBefore, MapSector sectorAfter, MapSectorGroup *outScopeSectors, MapSectorGroup *inScopeSectors);

DWORD       timeInitialize          = 0;
DWORD       timeCurrentFrame        = 0;
DWORD       timeLastLogicStart      = 0;
DWORD       timeLastSecondStart     = 0;
DWORD       timeFramePerSecond      = 0;
DWORD       framePerSecondNet       = 0;
DWORD       framePerSecondLogic     = 0;
DWORD       framePerSecondRecv      = 0;
DWORD       framePerSecondSend      = 0;
DWORD       framePerSecondAccept    = 0;
DWORD       framePerSecondEcho      = 0;
DWORD       framePerSecondNetCnt    = 0;
DWORD       framePerSecondLogicCnt  = 0;
DWORD       framePerSecondRecvCnt   = 0;
DWORD       framePerSecondSendCnt   = 0;
DWORD       framePerSecondAcceptCnt = 0;
DWORD       framePerSecondEchoCnt   = 0;
DWORD       timeLogicIntervalMS     = 1000 / SERVER_FPS;

USHORT      monitorCPUT             = 0;
USHORT      monitorCPUP             = 0;
DWORD       monitorMemoryW          = 0;
DWORD       monitorMemoryP          = 0;
DWORD       monitorMemoryN          = 0;
DWORD       byteRecvPerSecond       = 0;
DWORD       byteSendPerSecond       = 0;
DWORD       byteRecvPerSecondCnt    = 0;
DWORD       byteSendPerSecondCnt    = 0;
DWORD       byteRecvProcPerSecond   = 0;
DWORD       byteSendReqPerSecond    = 0;
DWORD       byteRecvProcPerSecondCnt= 0;
DWORD       byteSendReqPerSecondCnt = 0;
USHORT      wrongSyncTotal          = 0;
USHORT      wrongCodeTotal          = 0;
USHORT      wrongErrorTotal         = 0;

INT32		sessionNextID           = 1000;
INT32       playerNextID            = 1000;

SOCKET      listenSocket            = INVALID_SOCKET;
SOCKET      listenMonitorSocket     = INVALID_SOCKET;
Session     *monitorSession         = nullptr;

ULONGLONG   monitorCPUPrevTotal     = 0;
ULONGLONG   monitorCPUPrevIdle      = 0;

using namespace std;
using namespace azely;

INT32 wmain(INT32 argc, PWSTR *argv)
{
    logger = SimpleLogger::GetInstance();
    logger->SetLogSetting(SimpleLogger::LEVEL_ERROR, SimpleLogger::MODE_CONSOLE | SimpleLogger::MODE_FILE, SimpleLogger::LEVEL_DEBUG);

    logger->SaveLogInfo(__FUNCTIONW__, L"Server Started");

    BOOL flagSuccess = false;

    timeBeginPeriod(1);

    flagSuccess = LoadData();
    if (!flagSuccess) {
        logger->SaveLogCritical(L"LoadData", L"LoadData Returned Error");
        return -1;
    }

    flagSuccess = NetSetUp();
    if (!flagSuccess) {
        logger->SaveLogCritical(L"NetSetUp", L"NetSetUp Returned Error");
        return -1;
    }

    timeInitialize = timeGetTime();

    while (true) {

        timeCurrentFrame = timeGetTime();

        flagSuccess = ProcessNetwork();
        if (!flagSuccess) {
            logger->SaveLogWarning(L"ProcessNetwork", L"ProcessNetwork Returned Shutdown");
            break;
        }

        flagSuccess = ProcessLogic();
        if (!flagSuccess) {
            logger->SaveLogWarning(L"ProcessLogic", L"ProcessLogic Returned Shutdown");
            break;
        }

        flagSuccess = ProcessControl();
        if (!flagSuccess) {
            logger->SaveLogWarning(L"ProcessControl", L"ProcessControl Returned Shutdown");
            break;
        }

        ProcessBySecond();
    }
    
    ProcessShutDown();

    NetCleanUp();

    timeEndPeriod(1);

    logger->SaveLogInfo(__FUNCTIONW__, L"Server Finished");
    system("pause");
}

BOOL LoadData() 
{
    // TODO load initial game resources

    return true;
}

BOOL InitializeData()
{
    srand(4321);

    timeCurrentFrame = timeGetTime();
    timeLastLogicStart = 0;
    timeLastSecondStart = 0;
    timeFramePerSecond = 0;
    framePerSecondNet = 0;
    framePerSecondLogic = 0;

    sessionMap.clear();
    playerMap.clear();
    playerSocketMap.clear();

    return true;
}

BOOL NetSetUp() 
{
    WSADATA wsa;
    INT32 startupResult = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (startupResult != 0) {
        logger->SaveLogCritical(__FUNCTIONW__, L"WSAStartup", startupResult);
        return false;
    }

    // 리슨 소켓 세팅
    {
        listenSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (listenSocket == INVALID_SOCKET) {
            logger->SaveLogCritical(__FUNCTIONW__, L"listenSocket INVALID_SOCKET", WSAGetLastError());
            return false;
        }

        ULONG ioctlValue = 1;
        INT32 ioctlResult = ioctlsocket(listenSocket, FIONBIO, &ioctlValue);
        if (ioctlResult == SOCKET_ERROR) {
            logger->SaveLogCritical(__FUNCTIONW__, L"listenSocket ioctl", WSAGetLastError());
            return false;
        }

        LINGER lingerValue = { 1, 0 };
        INT32 lingerResult = setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, reinterpret_cast<PCHAR>(&lingerValue), sizeof(LINGER));
        if (lingerResult == SOCKET_ERROR) {
            logger->SaveLogCritical(__FUNCTIONW__, L"listenSocket linger", WSAGetLastError());
            return false;
        }

#ifdef TCP_NAGLE_OFF
        INT32 nodelayValue = 1;
        INT32 nodelayResult = setsockopt(listenSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<PCHAR>(&nodelayValue), sizeof(INT32));
        if (nodelayResult == SOCKET_ERROR) {
            logger->SaveLogCritical(__FUNCTIONW__, L"listenSocket nodelay", WSAGetLastError());
            return false;
        }
#endif

        SOCKADDR_IN socketAddr;
        int sockAddrSize = sizeof(SOCKADDR_IN);
        socketAddr.sin_family = AF_INET;
        WSAHtonl(listenSocket, INADDR_ANY, &socketAddr.sin_addr.s_addr);
        WSAHtons(listenSocket, NETWORK_PORT, &socketAddr.sin_port);

        INT32 bindResult = bind(listenSocket, reinterpret_cast<PSOCKADDR>(&socketAddr), sockAddrSize);
        if (bindResult == SOCKET_ERROR) {
            logger->SaveLogCritical(__FUNCTIONW__, L"listenSocket bind", WSAGetLastError());
            return false;
        }

        PWSTR bindAddressIP = new WCHAR[30];
        DWORD sockAddrStrSize = 30;
        ZeroMemory(bindAddressIP, 30 * sizeof(WCHAR));
        WSAAddressToStringW(reinterpret_cast<LPSOCKADDR>(&socketAddr), sockAddrSize, 0, bindAddressIP, &sockAddrStrSize);

        constexpr SIZE_T messageSize = 60;
        WCHAR message[messageSize] = { 0, };
        _snwprintf_s(message, messageSize * sizeof(WCHAR), L"Server Binded (%s)", bindAddressIP);
        ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
        logger->SaveLogInfo(__FUNCTIONW__, message);

        delete[] bindAddressIP;

        INT32 listenResult = listen(listenSocket, SOMAXCONN);
        if (listenResult == SOCKET_ERROR) {
            logger->SaveLogCritical(__FUNCTIONW__, L"listenSocket listen", WSAGetLastError());
            return false;
        }

        ZeroMemory(message, 60 * 2);
        _snwprintf_s(message, messageSize * sizeof(WCHAR), L"Server Listening");
        ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
        logger->SaveLogInfo(__FUNCTIONW__, message);
    }

    // 모니터링 전용 소켓 세팅
    {
        listenMonitorSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (listenMonitorSocket == INVALID_SOCKET) {
            logger->SaveLogCritical(__FUNCTIONW__, L"listenMonitorSocket INVALID_SOCKET", WSAGetLastError());
            return false;
        }

        ULONG ioctlValue = 1;
        INT32 ioctlResult = ioctlsocket(listenMonitorSocket, FIONBIO, &ioctlValue);
        if (ioctlResult == SOCKET_ERROR) {
            logger->SaveLogCritical(__FUNCTIONW__, L"listenMonitorSocket ioctl", WSAGetLastError());
            return false;
        }

        LINGER lingerValue = { 1, 0 };
        INT32 lingerResult = setsockopt(listenMonitorSocket, SOL_SOCKET, SO_LINGER, reinterpret_cast<PCHAR>(&lingerValue), sizeof(LINGER));
        if (lingerResult == SOCKET_ERROR) {
            logger->SaveLogCritical(__FUNCTIONW__, L"listenMonitorSocket linger", WSAGetLastError());
            return false;
        }

#ifdef TCP_NAGLE_OFF
        INT32 nodelayValue = 1;
        INT32 nodelayResult = setsockopt(listenMonitorSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<PCHAR>(&nodelayValue), sizeof(INT32));
        if (nodelayValue == SOCKET_ERROR) {
            logger->SaveLogCritical(__FUNCTIONW__, L"listenSocket nodelay", WSAGetLastError());
            return false;
        }
#endif

        SOCKADDR_IN socketAddr;
        int sockAddrSize = sizeof(SOCKADDR_IN);
        socketAddr.sin_family = AF_INET;
        WSAHtonl(listenMonitorSocket, INADDR_ANY, &socketAddr.sin_addr.s_addr);
        WSAHtons(listenMonitorSocket, NETWORK_PORT_MONITOR, &socketAddr.sin_port);

        INT32 bindResult = bind(listenMonitorSocket, reinterpret_cast<PSOCKADDR>(&socketAddr), sockAddrSize);
        if (bindResult == SOCKET_ERROR) {
            logger->SaveLogCritical(__FUNCTIONW__, L"listenMonitorSocket bind", WSAGetLastError());
            return false;
        }

        PWSTR bindAddressIP = new WCHAR[30];
        DWORD sockAddrStrSize = 30;
        ZeroMemory(bindAddressIP, 30 * sizeof(WCHAR));
        WSAAddressToStringW(reinterpret_cast<LPSOCKADDR>(&socketAddr), sockAddrSize, 0, bindAddressIP, &sockAddrStrSize);

        constexpr SIZE_T messageSize = 60;
        WCHAR message[messageSize] = { 0, };
        _snwprintf_s(message, messageSize * sizeof(WCHAR), L"Server Monitor Binded (%s)", bindAddressIP);
        ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
        logger->SaveLogInfo(__FUNCTIONW__, message);

        delete[] bindAddressIP;

        INT32 listenResult = listen(listenMonitorSocket, SOMAXCONN);
        if (listenResult == SOCKET_ERROR) {
            logger->SaveLogCritical(__FUNCTIONW__, L"listenMonitorSocket listen", WSAGetLastError());
            return false;
        }

        ZeroMemory(message, 60 * 2);
        _snwprintf_s(message, messageSize * sizeof(WCHAR), L"Server Monitor Listening");
        ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
        logger->SaveLogInfo(__FUNCTIONW__, message);
    }

    return true;
}

VOID NetCleanUp() 
{
    // 접속중이던 연결을 종료한다
    Session *currentSession = nullptr;
    auto sessionIterator = sessionMap.begin();
    while (sessionIterator != sessionMap.end()) {
        currentSession = sessionIterator->second;
        ++sessionIterator;
        DisconnectSession(currentSession->socket);
    }
    if (listenSocket != INVALID_SOCKET) {
        closesocket(listenSocket);
    }

    WSACleanup();
}

INT32 NetworkSelect(FD_SET *readSet, FD_SET *writeSet, TIMEVAL *timeValue)
{
	return select(0, readSet, writeSet, nullptr, timeValue);
}

BOOL ProcessNetwork() 
{
    framePerSecondNetCnt++;

    SOCKET socketCheckTable[FD_SETSIZE - 2] = { INVALID_SOCKET, };
    Session *socketSessionTable[FD_SETSIZE - 2] = { nullptr, };

    FD_SET readSet;
    FD_SET writeSet;
    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);

    Session *currentSession;
    auto sessionIterator = sessionMap.begin();

    TIMEVAL timeValue = { 0, 0 };

    while (true) {
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);

        // 리슨 소켓은 항상 체크
        FD_SET(listenSocket, &readSet);
        FD_SET(listenMonitorSocket, &readSet);

        int userCount = 0;

        // 64-리슨소켓 개수 만큼씩 나눠서 플레이어 소켓 검사
        for (userCount = 0; userCount < 62; userCount++) {
            if (sessionIterator == sessionMap.end()) break;
            currentSession = sessionIterator->second;

            FD_SET(currentSession->socket, &readSet);
            if (currentSession->sendQueue.GetSizeUsed() > 0) {
                //FD_SET(currentSession->socket, &writeSet);
                NetworkSend(currentSession);
            }
            socketCheckTable[userCount] = currentSession->socket;
            socketSessionTable[userCount] = currentSession;
            ++sessionIterator;
        }
        
        INT32 selectResult = NetworkSelect(&readSet, nullptr, &timeValue);

        if (selectResult > 0) {
            if (FD_ISSET(listenSocket, &readSet)) {
                NetworkAccept(listenSocket);
                selectResult--;
            }
            if (FD_ISSET(listenMonitorSocket, &readSet)) {
                NetworkAcceptMonitor(listenMonitorSocket);
                selectResult--;
            }
            for (int checkUserCount = 0; checkUserCount < userCount; checkUserCount++) {
                if (selectResult <= 0) break;
                /*
                if (FD_ISSET(socketCheckTable[checkUserCount], &writeSet)) {
                    NetworkSend(socketSessionTable[checkUserCount]);
                    selectResult--;
                }
                */
                if (FD_ISSET(socketCheckTable[checkUserCount], &readSet)) {
                    NetworkRecv(socketSessionTable[checkUserCount]);
                    selectResult--;
                }
            }
        }
        else if (selectResult == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                logger->SaveLogError(L"ProcessNetwork", L"selectResult SOCKET_ERROR", WSAGetLastError());
            }
        }

        // 모든 플레이어 검사했다면 루프 종료
        if (sessionIterator == sessionMap.end()) break;
    }

    /*
    auto sendIterator = sessionSendSet.begin();
    currentSession = nullptr;
    while (sendIterator != sessionSendSet.end())
    {
        currentSession = *sendIterator;
        ++sendIterator;
        NetworkSend(currentSession);
    }
    sessionSendSet.clear();
    */

    return true;
}

BOOL ProcessLogic() 
{
    DWORD timeElapsed = timeGetTime() - timeLastLogicStart;
    if (timeElapsed < timeLogicIntervalMS) return true;

    framePerSecondLogicCnt++;
    timeLastLogicStart = timeGetTime();

#ifdef DEBUG_LOGGING
    {
        constexpr SIZE_T messageSize = 60;
        WCHAR message[messageSize] = { 0, };
        _snwprintf_s(message, messageSize * sizeof(WCHAR), L"LOGIC FRAME IS %d PLAYER SIZE IS %d", timeElapsed, playerMap.size());
        ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
        logger->SaveLogDebug(__FUNCTIONW__, message);
    }
#endif

    ProcessLogicDisconnect();

    ProcessLogicPlayerMove(timeElapsed);

    return true;
}

VOID ProcessLogicDisconnect()
{
    // 연결 종료 플래그 세션 종료
    Player *currentDisconnectCheckPlayer;
    auto playerDisconnectIterator = playerMap.begin();
    while (playerDisconnectIterator != playerMap.end()) {
        currentDisconnectCheckPlayer = playerDisconnectIterator->second;
        ++playerDisconnectIterator;
        if (currentDisconnectCheckPlayer->status == STATUS_DELETE) {
            DisconnectClient(currentDisconnectCheckPlayer);
        }
    }
}

VOID ProcessLogicPlayerMove(DWORD timeElapsed)
{
    DOUBLE movingX = (Player::SPEED_X_PER_SEC * ((DOUBLE)timeElapsed / 1000));
    DOUBLE movingY = (Player::SPEED_Y_PER_SEC * ((DOUBLE)timeElapsed / 1000));

    auto playerIterator = playerMap.begin();
    Player *currentPlayer;
    while (playerIterator != playerMap.end())
    {
        currentPlayer = playerIterator->second;
        ++playerIterator;
        if (currentPlayer->status == STATUS_MOVE)
        {
            ProcessLogicPlayerMove(timeElapsed, currentPlayer, movingX, movingY);
        }
    }
}

VOID ProcessLogicPlayerMove(DWORD timeElapsed, Player *player, DOUBLE movingX, DOUBLE movingY)
{
    /*  OK
    *   1. 이동 전 기존 섹터 백업
    *   2. 이동 후 섹터 업데이트
    *   3. 기존 섹터와 다르다면 아웃 섹터를 구한다
    *   3-1. 아웃 섹터 유저들에게 삭제 메시지 전송
    *   3-2. 아웃 섹터 유저들 삭제 메시지를 플레이어에게 전송
    *   4. 기존 섹터와 다르다면 인 섹터를 구한다
    *   4-1. 인 섹터 유저들에게 생성 메시지 전송
    *   4-1-1. 인 섹터 유저들에게 이동 메시지 전송
    *   4-2. 인 섹터 유저들 생성 메시지를 플레이어에게 전송
    *   4-2-1. 인 섹터 유저들 이동 메시지를 플레이어에게 전송
    *   5. 다 만들고 한단계씩 꼼꼼히 테스트...
    */

    // 이동 전 기존 섹터 백업
    MapSector sectorBefore = player->sector;

    // 이동 처리
    movingX *= getDirectionXMultiplier(player->direction);
    movingY *= getDirectionYMultiplier(player->direction);

    BOOL adjustXY = true;
    if (player->posX + movingX > Map::X_MAX)
    {
        player->posX = Map::X_MAX;
        adjustXY = false;
    }
    if (player->posX + movingX < Map::X_MIN)
    {
        player->posX = Map::X_MIN;
        adjustXY = false;
    }
    if (player->posY + movingY > Map::Y_MAX)
    {
        player->posY = Map::Y_MAX;
        adjustXY = false;
    }
    if (player->posY + movingY < Map::Y_MIN)
    {
        player->posY = Map::Y_MIN;
        adjustXY = false;
    }

    // 가장자리에 묶인게 아니라면 이동
    if (!adjustXY) return;
    player->posX += movingX;
    player->posY += movingY;

#ifdef DEBUG_LOGGING
    constexpr SIZE_T messageSize = 60;
    WCHAR message[messageSize] = { 0, };
    _snwprintf_s(message, messageSize * sizeof(WCHAR), L"PLAYER [%d] MOVE X [%lf] Y [%lf]", player->playerID, player->posX, player->posY);
    ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
    logger->SaveLogDebug(__FUNCTIONW__, message);
#endif

    // 이동 후 섹터 업데이트
    player->sector.x = GetMapSectorIndexX(player->posX);
    player->sector.y = GetMapSectorIndexY(player->posY);

    // 기존 섹터와 차이가 있는지 확인
    if (player->sector.x != sectorBefore.x || player->sector.y != sectorBefore.y)
    {
        // 섹터 업데이트
        playerSectorMap[sectorBefore.y][sectorBefore.x].erase(player->playerID);
        playerSectorMap[player->sector.y][player->sector.x].insert(pair<INT32, Player *>(player->playerID, player));

        // 아웃, 인 섹터를 구한다
        MapSectorGroup outSector, inSector;
        GetSectorChanges(sectorBefore, player->sector, &outSector, &inSector);

        // 아웃 섹터 유저들에게 현재 플레이어 삭제 메시지 전송
        SerializedBuffer *playerDeleteFromOtherPacket = packetPool.Alloc();
        playerDeleteFromOtherPacket->Clear();
        Make_PACKET_SC_DELETE_CHARACTER(playerDeleteFromOtherPacket, player->playerID);
        SendPacketBroadcastSector(player, &outSector, playerDeleteFromOtherPacket);
        playerDeleteFromOtherPacket->Clear();
        packetPool.Free(playerDeleteFromOtherPacket);

        int sectorIndex = 0;
        // 아웃 섹터 유저들 삭제 메시지를 현재 플레이어에게 전송
        SerializedBuffer *playerDeleteOthersPacket = packetPool.Alloc();
        for (sectorIndex = 0; sectorIndex < outSector.sectorCount; sectorIndex++)
        {
            MapSector currentSector = outSector.sectors[sectorIndex];
            auto playerSectorMapCurrent = playerSectorMap[currentSector.y][currentSector.x];
            auto playerSectorMapCurrentIterator = playerSectorMapCurrent.begin();
            Player *currentPlayer;
            while (playerSectorMapCurrentIterator != playerSectorMapCurrent.end())
            {
                currentPlayer = playerSectorMapCurrentIterator->second;
                ++playerSectorMapCurrentIterator;
                if (currentPlayer == player) continue;
                playerDeleteOthersPacket->Clear();
                Make_PACKET_SC_DELETE_CHARACTER(playerDeleteOthersPacket, currentPlayer->playerID);
                SendPacketUnicast(player, playerDeleteOthersPacket);
            }
        }
        playerDeleteOthersPacket->Clear();
        packetPool.Free(playerDeleteOthersPacket);

        // 인 섹터 유저들에게 현재 플레이어 생성 및 이동 메시지 전송
        SerializedBuffer *playerCreatePacket = packetPool.Alloc();
        SerializedBuffer *playerMovePacket = packetPool.Alloc();
        playerCreatePacket->Clear();
        playerMovePacket->Clear();
        Make_PACKET_SC_CREATE_CHARACTER_OTHERS(playerCreatePacket, player->playerID, player->direction, player->posX, player->posY, player->hp);
        SendPacketBroadcastSector(player, &inSector, playerCreatePacket);
        Make_PACKET_SC_MOVE_START(playerMovePacket, player->playerID, player->direction, player->posX, player->posY);
        SendPacketBroadcastSector(player, &inSector, playerMovePacket);
        playerCreatePacket->Clear();
        playerMovePacket->Clear();
        packetPool.Free(playerMovePacket);
        packetPool.Free(playerCreatePacket);

        // 인 섹터 유저들의 생성 및 이동 메시지를 플레이어에게 전송
        SerializedBuffer *playerCreateOtherPacket = packetPool.Alloc();
        SerializedBuffer *playerMoveOtherPacket = packetPool.Alloc();
        for (sectorIndex = 0; sectorIndex < inSector.sectorCount; sectorIndex++) 
        {
            MapSector currentSector = inSector.sectors[sectorIndex];
            auto playerSectorMapCurrent = playerSectorMap[currentSector.y][currentSector.x];
            auto playerSectorMapCurrentIterator = playerSectorMapCurrent.begin();
            Player *currentPlayer;
            while (playerSectorMapCurrentIterator != playerSectorMapCurrent.end())
            {
                currentPlayer = playerSectorMapCurrentIterator->second;
                ++playerSectorMapCurrentIterator;
                if (currentPlayer == player) continue;
                playerCreateOtherPacket->Clear();
                Make_PACKET_SC_CREATE_CHARACTER_OTHERS(playerCreateOtherPacket, currentPlayer->playerID, currentPlayer->direction, currentPlayer->posX, currentPlayer->posY, currentPlayer->hp);
                SendPacketUnicast(player, playerCreateOtherPacket);
                if (currentPlayer->status == STATUS_MOVE) {
                    playerMoveOtherPacket->Clear();
                    Make_PACKET_SC_MOVE_START(playerMoveOtherPacket, currentPlayer->playerID, currentPlayer->direction, currentPlayer->posX, currentPlayer->posY);
                    SendPacketUnicast(player, playerMoveOtherPacket);
                }
            }
        }
        playerCreateOtherPacket->Clear();
        playerMoveOtherPacket->Clear();
        packetPool.Free(playerCreateOtherPacket);
        packetPool.Free(playerMoveOtherPacket);

    }
}

BOOL ProcessControl() 
{

    return true;
}

VOID ProcessBySecond() 
{
    DWORD timeFrameCheck = timeGetTime();
    if (timeFrameCheck < timeLastSecondStart + 1000) return;

    // 타임아웃 클라이언트 접속종료
    auto playerIter = playerMap.begin();
    Player *currentPlayer;
    while (playerIter != playerMap.end()) 
    {
        currentPlayer = playerIter->second;
        ++playerIter;

        if (timeFrameCheck - currentPlayer->session->lastRecvTime > NETWORK_PACKET_RECV_TIMEOUT_)
        {
            currentPlayer->status = STATUS_DELETE;
            currentPlayer->session->disconnectFlag = true;
#ifdef DEBUG_LOGGING
            logger->SaveLogDebug(L"ProcessBySecond", L"timeout client cleared");
            {
                constexpr SIZE_T messageSize = 60;
                WCHAR message[messageSize] = { 0, };
                _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PLAYER ID [%d] <DELETE> TIMEOUT>", currentPlayer->playerID);
                ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
                logger->SaveLogDebug(__FUNCTIONW__, message);
            }
#endif
            continue;
        }
    }
    
    timeLastSecondStart = timeFrameCheck;
    framePerSecondNet = framePerSecondNetCnt;
    framePerSecondLogic = framePerSecondLogicCnt;
    framePerSecondRecv = framePerSecondRecvCnt;
    framePerSecondSend = framePerSecondSendCnt;
    framePerSecondAccept = framePerSecondAcceptCnt;
    framePerSecondEcho = framePerSecondEchoCnt;
    byteRecvPerSecond = byteRecvPerSecondCnt;
    byteSendPerSecond = byteSendPerSecondCnt;
    byteRecvProcPerSecond = byteRecvProcPerSecondCnt;
    byteSendReqPerSecond = byteSendReqPerSecondCnt;
    framePerSecondNetCnt = 0;
    framePerSecondLogicCnt = 0;
    framePerSecondRecvCnt = 0;
    framePerSecondSendCnt = 0;
    framePerSecondAcceptCnt = 0;
    framePerSecondEchoCnt = 0;
    byteRecvPerSecondCnt = 0;
    byteSendPerSecondCnt = 0;
    byteRecvProcPerSecondCnt = 0;
    byteSendReqPerSecondCnt = 0;

    ProcessMonitor();
}

VOID ProcessMonitor() 
{
    {
        FILETIME idleTime, kernelTime, userTime;
        if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
            ULONGLONG totalTime = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime
                + ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;
            ULONGLONG deltaTime = totalTime - monitorCPUPrevTotal;
            ULONGLONG deltaIdle = ((ULONGLONG)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime - monitorCPUPrevIdle;
            double cpuUsage = (double)(deltaTime - deltaIdle) * 100.0 / deltaTime;
            monitorCPUT = static_cast<USHORT>(min(round(cpuUsage), 100.0));
            monitorCPUPrevTotal = totalTime;
            monitorCPUPrevIdle = ((ULONGLONG)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
        }
    }

    {
        FILETIME creationTime, exitTime, kernelTime, userTime;
        if (GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime)) {
            ULONGLONG totalTime = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime
                + ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;
            ULONGLONG totalElapsedTime = GetTickCount64() - GetProcessId(GetCurrentProcess());
            double cpuUsage = (double)totalTime / (double)totalElapsedTime * 100.0;
            monitorCPUP = static_cast<USHORT>(min(round(cpuUsage), 100.0));
        }
    }
    
    {
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc))) {
            monitorMemoryW = pmc.WorkingSetSize / 1024;
            monitorMemoryP = pmc.PrivateUsage / 1024;
        }
    }

    {
        PERFORMANCE_INFORMATION pi;
        if (GetPerformanceInfo(&pi, sizeof(pi))) {
            monitorMemoryN = pi.KernelNonpaged / 1024;
        }
    }
    /*
    {
		system("cls");
        printf("%-30s %10d\n", "PLAYER COUNT", playerMap.size());
        printf("%-30s %10lu\n", "timeFramePerSecond", timeFramePerSecond);
        printf("%-30s %10lu\n", "framePerSecondNet", framePerSecondNet);
        printf("%-30s %10lu\n", "framePerSecondLogic", framePerSecondLogic);
        printf("%-30s %10lu\n", "framePerSecondRecv", framePerSecondRecv);
        printf("%-30s %10lu\n", "framePerSecondSend", framePerSecondSend);
        printf("%-30s %10lu\n", "framePerSecondAccept", framePerSecondAccept);
        printf("%-30s %10lu\n", "framePerSecondEcho", framePerSecondEcho);
        printf("%-30s %10hu\n", "monitorCPUT", monitorCPUT);
        printf("%-30s %10hu\n", "monitorCPUP", monitorCPUP);
        printf("%-30s %10lu\n", "monitorMemoryW", monitorMemoryW);
        printf("%-30s %10lu\n", "monitorMemoryP", monitorMemoryP);
        printf("%-30s %10lu\n", "monitorMemoryN", monitorMemoryN);
        printf("%-30s %10lu\n", "byteRecvPerSecond", byteRecvPerSecond);
        printf("%-30s %10lu\n", "byteSendPerSecond", byteSendPerSecond);
        printf("%-30s %10lu\n", "byteRecvProcPerSecond", byteRecvProcPerSecond);
        printf("%-30s %10lu\n", "byteSendReqPerSecond", byteSendReqPerSecond);
        printf("%-30s %10hu\n", "wrongSyncTotal", wrongSyncTotal);
        printf("%-30s %10hu\n", "wrongCodeTotal", wrongCodeTotal);
        printf("%-30s %10hu\n", "wrongErrorTotal", wrongErrorTotal);
    }
    */
    constexpr SIZE_T messageSize = 256;
    WCHAR message[messageSize] = { 0, };
    _snwprintf_s(message, messageSize * sizeof(WCHAR), L"USERS [%d] \t FrameNetPS [%d] \t FrameLogicPS [%d] \t PacketRecvPS [%d] \t PacketSendPS [%d] \t AcceptPS [%d] \t EchoPS [%d]", playerMap.size(), framePerSecondNet, framePerSecondLogic, framePerSecondRecv, framePerSecondSend, framePerSecondAccept, framePerSecondEcho);
    ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
    logger->SaveLogInfo(__FUNCTIONW__, message);
}

VOID ProcessShutDown() 
{

}

VOID NetworkAccept(SOCKET listenSocket)
{
    if (listenSocket == INVALID_SOCKET) return;

    INT32 acceptedSockets = 0;
    while (true)
    {
        SOCKADDR_IN clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET acceptedSocket = accept(listenSocket, reinterpret_cast<LPSOCKADDR>(&clientAddr), &clientAddrSize);
        if (acceptedSocket == INVALID_SOCKET)
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                logger->SaveLogError(__FUNCTIONW__, L"ACCEPT Socket Invalid", WSAGetLastError());
            }
            break;
        }

        acceptedSockets++;
        framePerSecondAcceptCnt++;

        Session *newSession = CreateSession(acceptedSocket);
        Player *newPlayer = CreatePlayer(newSession);

#ifdef DEBUG_LOGGING
        constexpr SIZE_T messageSize = 60;
        WCHAR message[messageSize] = { 0, };
        _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<NEW CLIENT [%d] X[%d] Y[%d]>", newPlayer->playerID, newPlayer->posX, newPlayer->posY);
        ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
        logger->SaveLogDebug(__FUNCTIONW__, message);
#endif

        NetworkAcceptPlayer(acceptedSocket, newPlayer);

        if (acceptedSockets >= 5) {
            break;
        }
    }
}

VOID NetworkAcceptPlayer(SOCKET socket, Player *newPlayer)
{
    // 접속한 사람에게 자신 캐릭터 생성 메시지 보내기
    SerializedBuffer *packetCreateCharacterOwn = packetPool.Alloc();
    packetCreateCharacterOwn->Clear();
    Make_PACKET_SC_CREATE_CHARACTER_MINE(packetCreateCharacterOwn, newPlayer->playerID, newPlayer->direction, newPlayer->posX, newPlayer->posY, newPlayer->hp);
    SendPacketUnicast(newPlayer, packetCreateCharacterOwn);
    packetCreateCharacterOwn->Clear();
    packetPool.Free(packetCreateCharacterOwn);

    /*  OK
    *   1. 접속한 사람 기준 주변 섹터로만 보내도록 변경
    */

    // 접속한 플레이어의 주변 섹터 구하기
    MapSectorGroup newPlayerSectorAround;
    GetSectorNear(newPlayer->sector, &newPlayerSectorAround);

    // 접속한 플레이어에게 다른 캐릭터의 생성을 알리는 패킷
    SerializedBuffer *packetCreateCharacterOthers = packetPool.Alloc();
    // 접속한 플레이어에게 다른 캐릭터의 이동을 알리는 패킷
    SerializedBuffer *packetMoveCharacterOthers = packetPool.Alloc();

    // 접속한 플레이어의 주변 섹터로만
    int sectorIndex = 0;
    for (sectorIndex = 0; sectorIndex < newPlayerSectorAround.sectorCount; sectorIndex++)
    {
        MapSector currentSector = newPlayerSectorAround.sectors[sectorIndex];
        auto playerSectorMapCurrent = playerSectorMap[currentSector.y][currentSector.x];
        auto playerIterator = playerSectorMapCurrent.begin();
        Player *currentPlayer;
        // 섹터 당 섹터 내 플레이어 순회
        while (playerIterator != playerSectorMapCurrent.end()) {
            packetCreateCharacterOthers->Clear();
            currentPlayer = playerIterator->second;
            ++playerIterator;
            if (currentPlayer == newPlayer) continue;
            // 해당 플레이어의 생성 패킷 보내기
            Make_PACKET_SC_CREATE_CHARACTER_OTHERS(packetCreateCharacterOthers, currentPlayer->playerID, currentPlayer->direction, currentPlayer->posX, currentPlayer->posY, currentPlayer->hp);
            SendPacketUnicast(newPlayer, packetCreateCharacterOthers);
            // 만일 해당 플레이어가 이동상태였다면, 이동 패킷도 보냄
            if (currentPlayer->status == STATUS_MOVE) {
                packetMoveCharacterOthers->Clear();
                Make_PACKET_SC_MOVE_START(packetMoveCharacterOthers, currentPlayer->playerID, currentPlayer->direction, currentPlayer->posX, currentPlayer->posY);
                SendPacketUnicast(newPlayer, packetMoveCharacterOthers);
            }
        }
        packetMoveCharacterOthers->Clear();
        packetCreateCharacterOthers->Clear();
    }

    packetPool.Free(packetMoveCharacterOthers);
    packetPool.Free(packetCreateCharacterOthers);

    // 다른 사람들에게 접속한 사람 캐릭터 생성 메시지 보내기
    /*  OK
    *   1. 접속한 사람 기준 주변 섹터의 사람들의 생성만 되도록 변경
    */

    // 다른 플레이어에게 접속한 플레이어의 생성을 알리는 패킷
    SerializedBuffer *packetCreateCharacterBroadcast = packetPool.Alloc();

    packetCreateCharacterBroadcast->Clear();
    Make_PACKET_SC_CREATE_CHARACTER_OTHERS(packetCreateCharacterBroadcast, newPlayer->playerID, newPlayer->direction, newPlayer->posX, newPlayer->posY, newPlayer->hp);
    SendPacketBroadcastSector(newPlayer, &newPlayerSectorAround, packetCreateCharacterBroadcast);
    packetCreateCharacterBroadcast->Clear();

    packetPool.Free(packetCreateCharacterBroadcast);
}

VOID NetworkAcceptMonitor(SOCKET socket)
{

}


//VOID NetworkSend(SOCKET socket)
VOID NetworkSend(Session *session)
{
    //if (socket == INVALID_SOCKET) return;
    if (session == nullptr) return;

    /*
    if (session == nullptr) {
        logger->SaveLogError(__FUNCTIONW__, L"NetworkRecv session not found");
        return;
    }
    */

    if (session->disconnectFlag) return;

    while (true)
    {
        if (session->sendQueue.GetSizeUsed() == 0) break;
        int sendResult = send(session->socket, session->sendQueue.GetFrontBuffer(), session->sendQueue.GetSizeDirectDequeueAble(), 0);
        if (sendResult == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                break;
            }
            Player *player = FindPlayer(session->socket);
            if (player == nullptr) {
                logger->SaveLogError(__FUNCTIONW__, L"NetworkRecv player not found");
                return;
            }
            player->status = STATUS_DELETE;
            session->disconnectFlag = true;
#ifdef DEBUG_LOGGING
            {
                constexpr SIZE_T messageSize = 60;
                WCHAR message[messageSize] = { 0, };
                _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PLAYER ID [%d] <DELETE> SEND ERROR>", player->playerID);
                ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
                logger->SaveLogDebug(__FUNCTIONW__, message);
            }
#endif
            if (WSAGetLastError() != 10054) {
                logger->SaveLogWarning(__FUNCTIONW__, L"send result is SOCKET_ERROR", WSAGetLastError());
            }
            break;
        }
        BOOL moveBufferResult = session->sendQueue.MoveFrontBufferRear(sendResult);
        if (!moveBufferResult)
        {
            logger->SaveLogError(__FUNCTIONW__, L"sendQueue send bufferMove Error");
            break;
        }

        byteSendPerSecondCnt += sendResult;
    }

    if (session->sendQueue.GetSizeUsed() > 0)
    {
        // L4 송신버퍼가 다 찼다
        // L7 송신버퍼가 다 찰때까지 유예해준다
        /*
        player->status = STATUS_DELETE;
        session->disconnectFlag = true;
        logger->SaveLogWarning(__FUNCTIONW__, L"sendQueue has remainings");
#ifdef DEBUG_LOGGING
        {
            constexpr SIZE_T messageSize = 60;
            WCHAR message[messageSize] = { 0, };
            _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PLAYER ID [%d] <DELETE> SEND QUEUE REMAINING>", player->playerID);
            ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
            logger->SaveLogDebug(__FUNCTIONW__, message);
        }
#endif
        */
        return;
    }
}

//VOID NetworkRecv(SOCKET socket)
VOID NetworkRecv(Session *session)
{
    //if (socket == INVALID_SOCKET) return;

    //Session *session = FindSession(socket);
    Player *player = FindPlayer(session->socket);

    /*
    if (session == nullptr) {
        logger->SaveLogError(__FUNCTIONW__, L"NetworkRecv session not found");
        return;
    }
    */

    if (player == nullptr) {
        logger->SaveLogError(__FUNCTIONW__, L"NetworkRecv player not found");
        return;
    }

    // GET EVERY PACKETS
    if (session->recvQueue.GetSizeFree() == 0)
    {
        wrongErrorTotal++;
        player->status = STATUS_DELETE;
        session->disconnectFlag = true;
        logger->SaveLogWarning(__FUNCTIONW__, L"recvQueue is full");
#ifdef DEBUG_LOGGING
        {
            constexpr SIZE_T messageSize = 60;
            WCHAR message[messageSize] = { 0, };
            _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PLAYER ID [%d] <DELETE> RECVQUEUE FULL>", player->playerID);
            ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
            logger->SaveLogDebug(__FUNCTIONW__, message);
        }
#endif
        return;
    }
    int recvResult = recv(session->socket, session->recvQueue.GetRearBuffer(), session->recvQueue.GetSizeDirectEnqueueAble(), 0);
    if (recvResult == 0)
    {
        player->status = STATUS_DELETE;
        session->disconnectFlag = true;
#ifdef DEBUG_LOGGING
        logger->SaveLogDebug(__FUNCTIONW__, L"recv result is zero");
        {
            constexpr SIZE_T messageSize = 60;
            WCHAR message[messageSize] = { 0, };
            _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PLAYER ID [%d] <DELETE> RECV ZERO>", player->playerID);
            ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
            logger->SaveLogDebug(__FUNCTIONW__, message);
        }
#endif
        return;
    }
    if (recvResult == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSAEWOULDBLOCK && WSAGetLastError() != 10054) {
            player->status = STATUS_DELETE;
            session->disconnectFlag = true;
            logger->SaveLogWarning(__FUNCTIONW__, L"recv result is SOCKET_ERROR", WSAGetLastError());
            return;
#ifdef DEBUG_LOGGING
            {
                constexpr SIZE_T messageSize = 60;
                WCHAR message[messageSize] = { 0, };
                _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PLAYER ID [%d] <DELETE> RECV ERROR>", player->playerID);
                ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
                logger->SaveLogDebug(__FUNCTIONW__, message);
            }
#endif
        }
        return;
    }

    session->lastRecvTime = timeCurrentFrame;
    byteRecvPerSecondCnt += recvResult;

    BOOL moveBufferResult = session->recvQueue.MoveRearBufferRear(recvResult);
    if (!moveBufferResult)
    {
        wrongErrorTotal++;
        player->status = STATUS_DELETE;
        session->disconnectFlag = true;
#ifdef DEBUG_LOGGING
        {
            constexpr SIZE_T messageSize = 60;
            WCHAR message[messageSize] = { 0, };
            _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PLAYER ID [%d] <DELETE> RECV BUFFER ERROR>", player->playerID);
            ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
            logger->SaveLogDebug(__FUNCTIONW__, message);
        }
#endif
        logger->SaveLogError(__FUNCTIONW__, L"recvQueue recv bufferMove Error");
        return;
    }

    if (session->disconnectFlag) return;

    // HANDLE EVERY PACKETS
    SerializedBuffer *serializedBuffer = packetPool.Alloc();
    serializedBuffer->Clear();
    while (true)
    {
        if (session->recvQueue.GetSizeUsed() < sizeof(PACKET_HEADER)) break;

        PACKET_HEADER netHeader;
        ZeroMemory(&netHeader, sizeof(PACKET_HEADER));
        INT32 peekHeadOutSize = 0;
        BOOL peekHeadResult = session->recvQueue.Peek(reinterpret_cast<PCHAR>(&netHeader), sizeof(PACKET_HEADER), &peekHeadOutSize, false);
        if (!peekHeadResult || peekHeadOutSize != sizeof(PACKET_HEADER))
        {
            wrongErrorTotal++;
            player->status = STATUS_DELETE;
            session->disconnectFlag = true;
            logger->SaveLogError(__FUNCTIONW__, L"recvQueue header peek Error");
            break;
        }

        if (netHeader.byteCode != PACKET_CODE)
        {
            wrongCodeTotal++;
            player->status = STATUS_DELETE;
            session->disconnectFlag = true;
#ifdef DEBUG_LOGGING
            {
                constexpr SIZE_T messageSize = 60;
                WCHAR message[messageSize] = { 0, };
                _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PLAYER ID [%d] <DELETE> BYTECODE ERROR>", player->playerID);
                ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
                logger->SaveLogDebug(__FUNCTIONW__, message);
            }
#endif
            logger->SaveLogWarning(__FUNCTIONW__, L"byteCode Error");
            return;
        }

        if (session->recvQueue.GetSizeUsed() < netHeader.byteSize + sizeof(PACKET_HEADER)) 
        {
            // 헤더 길이보다는 많이 버퍼에 있지만, 실제 메시지만큼은 없음
            break;
        }

        framePerSecondRecvCnt++;

        INT32 peekOutSize = 0;
        BOOL peekResult = session->recvQueue.Peek(serializedBuffer->GetBufferWritePos(), netHeader.byteSize + sizeof(PACKET_HEADER), &peekOutSize, false);
        if (!peekResult || peekOutSize != netHeader.byteSize + sizeof(PACKET_HEADER))
        {
            wrongErrorTotal++;
            player->status = STATUS_DELETE;
            session->disconnectFlag = true;
            logger->SaveLogError(__FUNCTIONW__, L"recvQueue body peek Error");
            break;
        }

        BOOL moveBufferResult = session->recvQueue.MoveFrontBufferRear(peekOutSize);
        if (!moveBufferResult)
        {
            wrongErrorTotal++;
            player->status = STATUS_DELETE;
            session->disconnectFlag = true;
#ifdef DEBUG_LOGGING
            {
                constexpr SIZE_T messageSize = 60;
                WCHAR message[messageSize] = { 0, };
                _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PLAYER ID [%d] <DELETE> RECV BODY BUFFER ERROR>", player->playerID);
                ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
                logger->SaveLogDebug(__FUNCTIONW__, message);
            }
#endif
            logger->SaveLogError(__FUNCTIONW__, L"recvQueue body bufferMove Error");
            break;
        }

        INT32 movePacketBufferSize = 0;
        BOOL movePacketBufferResult = serializedBuffer->MoveWritePosRear(peekOutSize, &movePacketBufferSize);
        if (!movePacketBufferResult || movePacketBufferSize != peekOutSize) 
        {
            wrongErrorTotal++;
            player->status = STATUS_DELETE;
            session->disconnectFlag = true;
#ifdef DEBUG_LOGGING
            {
                constexpr SIZE_T messageSize = 60;
                WCHAR message[messageSize] = { 0, };
                _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PLAYER ID [%d] <DELETE> RECV PACKET BUFFER ERROR>", player->playerID);
                ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
                logger->SaveLogDebug(__FUNCTIONW__, message);
            }
#endif
            logger->SaveLogError(__FUNCTIONW__, L"recvQueue packet bufferMove Error");
            break;
        }

        NetworkPacket(player, &netHeader, serializedBuffer);
        serializedBuffer->Clear();
    }
    serializedBuffer->Clear();
    packetPool.Free(serializedBuffer);
}

VOID NetworkPacket(Player *player, PACKET_HEADER *netHeader, SerializedBuffer *packet)
{
    if (player == nullptr || player->session == nullptr || player->session->socket == INVALID_SOCKET)
    {
        logger->SaveLogError(L"NetworkPacket", L"player session socket is invalid");
        return;
    }

    // TODO CHECK SAFETY

    BYTE byteCode, byteSize, byteType;
    *packet >> byteCode >> byteSize >> byteType;

    byteRecvProcPerSecondCnt += (sizeof(PACKET_HEADER) + byteSize);

    switch (byteType)
    {
    case PACKET_CS_MOVE_START_:
        {
            BYTE direction;
            USHORT posX;
            USHORT posY;
            *packet >> direction >> posX >> posY;
            On_PACKET_CS_MOVE_START(player, direction, posX, posY);
        }
        break;
    case PACKET_CS_MOVE_STOP_:
        {
            BYTE direction;
            USHORT posX;
            USHORT posY;
            *packet >> direction >> posX >> posY;
            On_PACKET_CS_MOVE_STOP(player, direction, posX, posY);
        }
        break;
    case PACKET_CS_ATTACK1_:
        {
            BYTE direction;
            USHORT posX;
            USHORT posY;
            *packet >> direction >> posX >> posY;
            On_PACKET_CS_ATTACK1(player, direction, posX, posY);
        }
        break;
    case PACKET_CS_ATTACK2_:
        {
            BYTE direction;
            USHORT posX;
            USHORT posY;
            *packet >> direction >> posX >> posY;
            On_PACKET_CS_ATTACK2(player, direction, posX, posY);
        }
        break;
    case PACKET_CS_ATTACK3_:
        {
            BYTE direction;
            USHORT posX;
            USHORT posY;
            *packet >> direction >> posX >> posY;
            On_PACKET_CS_ATTACK3(player, direction, posX, posY);
        }
        break;
    case PACKET_CS_ECHO_:
        {
            DWORD time;
            *packet >> time;
            On_PACKET_CS_ECHO(player, time);
        }
        break;
    default :
        {
            logger->SaveLogError(L"NetworkPacket", L"header type not found");
        }
        break;
    }
}

VOID On_PACKET_CS_MOVE_START(Player *player, BYTE direction, USHORT posX, USHORT posY)
{
#ifdef DEBUG_LOGGING
    {
        constexpr SIZE_T messageSize = 60;
        WCHAR message[messageSize] = { 0, };
        _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PACKET FROM [%d] <MOVE START> X[%d] Y[%d]>", player->playerID, posX, posY);
        ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
        logger->SaveLogDebug(__FUNCTIONW__, message);
    }
#endif

    if (abs(player->posX - posX) > RANGE_ERROR_ ||
        abs(player->posY - posY) > RANGE_ERROR_)
    {
        wrongSyncTotal++;
        USHORT clientX = posX;
        USHORT clientY = posY;
        SerializedBuffer *packet = packetPool.Alloc();
        packet->Clear();
        posX = player->posX;
        posY = player->posY;
        Make_PACKET_SC_SYNC(packet, player->playerID, player->posX, player->posY);
        SendPacketUnicast(player, packet);
        packet->Clear();
        packetPool.Free(packet);
#ifdef DEBUG_LOGGING
        {
            constexpr SIZE_T messageSize = 60;
            WCHAR message[messageSize] = { 0, };
            _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<SYNC! [%d] <MOVE_START> X[%d->%d] Y[%d->%d]>", player->playerID, clientX, posX, clientY, posY);
            ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
            logger->SaveLogDebug(__FUNCTIONW__, message);
        }
#endif
        /*
        player->status = STATUS_DELETE;
        player->session->disconnectFlag = true;
        logger->SaveLogWarning(__FUNCTIONW__, L"out of tolerance");
        return;
        */
    }

    player->posX = posX;
    player->posY = posY;
    player->direction = static_cast<PlayerDirection>(direction);
    player->status = STATUS_MOVE;
#ifdef DEBUG_LOGGING
    {
        constexpr SIZE_T messageSize = 60;
        WCHAR message[messageSize] = { 0, };
        _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PACKET FROM [%d] IS MOVING TO %d>", player->playerID, player->direction);
        ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
        logger->SaveLogDebug(__FUNCTIONW__, message);
    }
#endif

    /** TODO
     *  1. 주변 섹터에만 패킷을 쏘도록 한다
     */
    // 주변 섹터 구하기
    MapSectorGroup sectorAround;
    GetSectorNear(player->sector, &sectorAround);

    SerializedBuffer *packet = packetPool.Alloc();
    packet->Clear();
    Make_PACKET_SC_MOVE_START(packet, player->playerID, player->direction, player->posX, player->posY);
    SendPacketBroadcastSector(player, &sectorAround, packet);
    packet->Clear();
    packetPool.Free(packet);
}

VOID On_PACKET_CS_MOVE_STOP(Player *player, BYTE direction, USHORT posX, USHORT posY)
{
#ifdef DEBUG_LOGGING
    constexpr SIZE_T messageSize = 60;
    WCHAR message[messageSize] = { 0, };
    _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PACKET FROM [%d] <MOVE STOP> X[%d] Y[%d]>", player->playerID, posX, posY);
    ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
    logger->SaveLogDebug(__FUNCTIONW__, message);
#endif

    if (abs(player->posX - posX) > RANGE_ERROR_ ||
        abs(player->posY - posY) > RANGE_ERROR_)
    {
        wrongSyncTotal++;
        USHORT clientX = posX;
        USHORT clientY = posY;
        SerializedBuffer *packet = packetPool.Alloc();
        packet->Clear();
        posX = player->posX;
        posY = player->posY;
        Make_PACKET_SC_SYNC(packet, player->playerID, player->posX, player->posY);
        SendPacketUnicast(player, packet);
        packet->Clear();
        packetPool.Free(packet);
#ifdef DEBUG_LOGGING
        {
            constexpr SIZE_T messageSize = 60;
            WCHAR message[messageSize] = { 0, };
            _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<SYNC! [%d] <MOVE_STOP> X[%d->%d] Y[%d->%d]>", player->playerID, clientX, posX, clientY, posY);
            ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
            logger->SaveLogDebug(__FUNCTIONW__, message);
        }
#endif
        /*
        player->status = STATUS_DELETE;
        player->session->disconnectFlag = true;
        logger->SaveLogWarning(__FUNCTIONW__, L"out of tolerance");
        return;
        */
    }

    player->posX = posX;
    player->posY = posY;
    player->direction = static_cast<PlayerDirection>(direction);
    player->status = STATUS_STOP;
#ifdef DEBUG_LOGGING
    {
        constexpr SIZE_T messageSize = 60;
        WCHAR message[messageSize] = { 0, };
        _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PACKET FROM [%d] IS STOP TO %d>", player->playerID, player->direction);
        ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
        logger->SaveLogDebug(__FUNCTIONW__, message);
    }
#endif

    /** TODO
     *  1. 주변 섹터에만 패킷을 쏘도록 한다
     */
     // 주변 섹터 구하기
    MapSectorGroup sectorAround;
    GetSectorNear(player->sector, &sectorAround);

    SerializedBuffer *packet = packetPool.Alloc();
    packet->Clear();
    Make_PACKET_SC_MOVE_STOP(packet, player->playerID, player->direction, player->posX, player->posY);
    SendPacketBroadcastSector(player, &sectorAround, packet);
    packet->Clear();
    packetPool.Free(packet);
}

VOID On_PACKET_CS_ATTACK1(Player *player, BYTE direction, USHORT posX, USHORT posY)
{
#ifdef DEBUG_LOGGING
    constexpr SIZE_T messageSize = 60;
    WCHAR message[messageSize] = { 0, };
    _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PACKET FROM [%d] <ATTACK1> X[%d] Y[%d]>", player->playerID, posX, posY);
    ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
    logger->SaveLogDebug(__FUNCTIONW__, message);
#endif

    if (abs(player->posX - posX) > RANGE_ERROR_ ||
        abs(player->posY - posY) > RANGE_ERROR_)
    {
        wrongSyncTotal++;
        USHORT clientX = posX;
        USHORT clientY = posY;
        SerializedBuffer *packet = packetPool.Alloc();
        packet->Clear();
        posX = player->posX;
        posY = player->posY;
        Make_PACKET_SC_SYNC(packet, player->playerID, player->posX, player->posY);
        SendPacketUnicast(player, packet);
        packet->Clear();
        packetPool.Free(packet);
#ifdef DEBUG_LOGGING
        {
            constexpr SIZE_T messageSize = 60;
            WCHAR message[messageSize] = { 0, };
            _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<SYNC! [%d] <ATTACK1> X[%d->%d] Y[%d->%d]>", player->playerID, clientX, posX, clientY, posY);
            ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
            logger->SaveLogDebug(__FUNCTIONW__, message);
        }
#endif
        /*
        player->status = STATUS_DELETE;
        player->session->disconnectFlag = true;
        logger->SaveLogWarning(__FUNCTIONW__, L"out of tolerance");
        return;
        */
    }

    player->posX = posX;
    player->posY = posY;
    player->direction = static_cast<PlayerDirection>(direction);
    player->status = STATUS_ATTACK1;
#ifdef DEBUG_LOGGING
    {
        constexpr SIZE_T messageSize = 60;
        WCHAR message[messageSize] = { 0, };
        _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PACKET FROM [%d] IS ATTACK1 TO %d>", player->playerID, player->direction);
        ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
        logger->SaveLogDebug(__FUNCTIONW__, message);
    }
#endif

    // 주변 섹터 구하기
    MapSectorGroup sectorAround;
    GetSectorNear(player->sector, &sectorAround);

    SerializedBuffer *packet = packetPool.Alloc();
    packet->Clear();
    Make_PACKET_SC_ATTACK1(packet, player->playerID, player->direction, player->posX, player->posY);
    SendPacketBroadcastSector(player, &sectorAround, packet);
    packet->Clear();
    packetPool.Free(packet);

    // 섹터별로 유저를 순회하여 충돌판정
    int sectorIndex = 0;
    for (sectorIndex = 0; sectorIndex < sectorAround.sectorCount; sectorIndex++)
    {
        MapSector mapSector = sectorAround.sectors[sectorIndex];
        auto playerSectorMapCurrent = playerSectorMap[mapSector.y][mapSector.x];
        auto playerSectorMapIterator = playerSectorMapCurrent.begin();
        Player *currentPlayer;
        while (playerSectorMapIterator != playerSectorMapCurrent.end())
        {
            currentPlayer = playerSectorMapIterator->second;
            ++playerSectorMapIterator;
            if (currentPlayer == player) continue;
            if (currentPlayer->status == STATUS_DELETE) continue;
            if (GetAttackResult(player, currentPlayer, Player::ATTACK1_RANGE_X, Player::ATTACK1_RANGE_Y))
            {
                currentPlayer->hp -= min(currentPlayer->hp, Player::ATTACK1_DAMAGE);

                SerializedBuffer *damagePacket = packetPool.Alloc();
                damagePacket->Clear();
                Make_PACKET_SC_DAMAGE(damagePacket, player->playerID, currentPlayer->playerID, currentPlayer->hp);

                MapSectorGroup damagerSectorAround;
                GetSectorNear(currentPlayer->sector, &damagerSectorAround);
                SendPacketBroadcastSector(nullptr, &damagerSectorAround, damagePacket);

                damagePacket->Clear();
                packetPool.Free(damagePacket);

                if (currentPlayer->hp <= 0)
                {
                    currentPlayer->status = STATUS_DELETE;
                    currentPlayer->session->disconnectFlag = true;
                }
            }
        }
    }
}

VOID On_PACKET_CS_ATTACK2(Player *player, BYTE direction, USHORT posX, USHORT posY)
{
#ifdef DEBUG_LOGGING
    constexpr SIZE_T messageSize = 60;
    WCHAR message[messageSize] = { 0, };
    _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PACKET FROM [%d] <ATTACK2> X[%d] Y[%d]>", player->playerID, posX, posY);
    ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
    logger->SaveLogDebug(__FUNCTIONW__, message);
#endif

    if (abs(player->posX - posX) > RANGE_ERROR_ ||
        abs(player->posY - posY) > RANGE_ERROR_)
    {
        wrongSyncTotal++;
        USHORT clientX = posX;
        USHORT clientY = posY;
        SerializedBuffer *packet = packetPool.Alloc();
        packet->Clear();
        posX = player->posX;
        posY = player->posY;
        Make_PACKET_SC_SYNC(packet, player->playerID, player->posX, player->posY);
        SendPacketUnicast(player, packet);
        packet->Clear();
        packetPool.Free(packet);
#ifdef DEBUG_LOGGING
        {
            constexpr SIZE_T messageSize = 60;
            WCHAR message[messageSize] = { 0, };
            _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<SYNC! [%d] <ATTACK2> X[%d->%d] Y[%d->%d]>", player->playerID, clientX, posX, clientY, posY);
            ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
            logger->SaveLogDebug(__FUNCTIONW__, message);
        }
#endif
        /*
        player->status = STATUS_DELETE;
        player->session->disconnectFlag = true;
        logger->SaveLogWarning(__FUNCTIONW__, L"out of tolerance");
        return;
        */
    }

    player->posX = posX;
    player->posY = posY;
    player->direction = static_cast<PlayerDirection>(direction);
    player->status = STATUS_ATTACK2;
#ifdef DEBUG_LOGGING
    {
        constexpr SIZE_T messageSize = 60;
        WCHAR message[messageSize] = { 0, };
        _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PACKET FROM [%d] IS ATTACK2 TO %d>", player->playerID, player->direction);
        ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
        logger->SaveLogDebug(__FUNCTIONW__, message);
    }
#endif

    // 주변 섹터 구하기
    MapSectorGroup sectorAround;
    GetSectorNear(player->sector, &sectorAround);

    SerializedBuffer *packet = packetPool.Alloc();
    packet->Clear();
    Make_PACKET_SC_ATTACK2(packet, player->playerID, player->direction, player->posX, player->posY);
    SendPacketBroadcastSector(player, &sectorAround, packet);
    packet->Clear();
    packetPool.Free(packet);

    // 섹터별로 유저를 순회하여 충돌판정
    int sectorIndex = 0;
    for (sectorIndex = 0; sectorIndex < sectorAround.sectorCount; sectorIndex++)
    {
        MapSector mapSector = sectorAround.sectors[sectorIndex];
        auto playerSectorMapCurrent = playerSectorMap[mapSector.y][mapSector.x];
        auto playerSectorMapIterator = playerSectorMapCurrent.begin();
        Player *currentPlayer;
        while (playerSectorMapIterator != playerSectorMapCurrent.end())
        {
            currentPlayer = playerSectorMapIterator->second;
            ++playerSectorMapIterator;
            if (currentPlayer == player) continue;
            if (currentPlayer->status == STATUS_DELETE) continue;
            if (GetAttackResult(player, currentPlayer, Player::ATTACK2_RANGE_X, Player::ATTACK2_RANGE_Y))
            {
                currentPlayer->hp -= min(currentPlayer->hp, Player::ATTACK2_DAMAGE);

                SerializedBuffer *damagePacket = packetPool.Alloc();
                damagePacket->Clear();
                Make_PACKET_SC_DAMAGE(damagePacket, player->playerID, currentPlayer->playerID, currentPlayer->hp);

                MapSectorGroup damagerSectorAround;
                GetSectorNear(currentPlayer->sector, &damagerSectorAround);
                SendPacketBroadcastSector(nullptr, &damagerSectorAround, damagePacket);

                damagePacket->Clear();
                packetPool.Free(damagePacket);

                if (currentPlayer->hp <= 0)
                {
                    currentPlayer->status = STATUS_DELETE;
                    currentPlayer->session->disconnectFlag = true;
                }
            }
        }
    }
}

VOID On_PACKET_CS_ATTACK3(Player *player, BYTE direction, USHORT posX, USHORT posY)
{
#ifdef DEBUG_LOGGING
    constexpr SIZE_T messageSize = 60;
    WCHAR message[messageSize] = { 0, };
    _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PACKET FROM [%d] <ATTACK3> X[%d] Y[%d]>", player->playerID, posX, posY);
    ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
    logger->SaveLogDebug(__FUNCTIONW__, message);
#endif

    if (abs(player->posX - posX) > RANGE_ERROR_ ||
        abs(player->posY - posY) > RANGE_ERROR_)
    {
        wrongSyncTotal++;
        USHORT clientX = posX;
        USHORT clientY = posY;
        SerializedBuffer *packet = packetPool.Alloc();
        packet->Clear();
        posX = player->posX;
        posY = player->posY;
        Make_PACKET_SC_SYNC(packet, player->playerID, player->posX, player->posY);
        SendPacketUnicast(player, packet);
        packet->Clear();
        packetPool.Free(packet);
#ifdef DEBUG_LOGGING
        {
            constexpr SIZE_T messageSize = 60;
            WCHAR message[messageSize] = { 0, };
            _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<SYNC! [%d] <ATTACK3> X[%d->%d] Y[%d->%d]>", player->playerID, clientX, posX, clientY, posY);
            ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
            logger->SaveLogDebug(__FUNCTIONW__, message);
        }
#endif
        /*
        player->status = STATUS_DELETE;
        player->session->disconnectFlag = true;
        logger->SaveLogWarning(__FUNCTIONW__, L"out of tolerance");
        return;
        */
    }

    player->posX = posX;
    player->posY = posY;
    player->direction = static_cast<PlayerDirection>(direction);
    player->status = STATUS_ATTACK3;
#ifdef DEBUG_LOGGING
    {
        constexpr SIZE_T messageSize = 60;
        WCHAR message[messageSize] = { 0, };
        _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PACKET FROM [%d] IS ATTACK3 TO %d>", player->playerID, player->direction);
        ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
        logger->SaveLogDebug(__FUNCTIONW__, message);
    }
#endif

    /** TODO
     *  1. 주변 섹터에만 패킷을 쏘도록 한다
     */
     // 주변 섹터 구하기
    MapSectorGroup sectorAround;
    GetSectorNear(player->sector, &sectorAround);

    SerializedBuffer *packet = packetPool.Alloc();
    packet->Clear();
    Make_PACKET_SC_ATTACK3(packet, player->playerID, player->direction, player->posX, player->posY);
    SendPacketBroadcastSector(player, &sectorAround, packet);
    packet->Clear();
    packetPool.Free(packet);

    // 섹터별로 유저를 순회하여 충돌판정
    int sectorIndex = 0;
    for (sectorIndex = 0; sectorIndex < sectorAround.sectorCount; sectorIndex++)
    {
        MapSector mapSector = sectorAround.sectors[sectorIndex];
        auto playerSectorMapCurrent = playerSectorMap[mapSector.y][mapSector.x];
        auto playerSectorMapIterator = playerSectorMapCurrent.begin();
        Player *currentPlayer;
        while (playerSectorMapIterator != playerSectorMapCurrent.end())
        {
            currentPlayer = playerSectorMapIterator->second;
            ++playerSectorMapIterator;
            if (currentPlayer == player) continue;
            if (currentPlayer->status == STATUS_DELETE) continue;
            if (GetAttackResult(player, currentPlayer, Player::ATTACK3_RANGE_X, Player::ATTACK3_RANGE_Y))
            {
                currentPlayer->hp -= min(currentPlayer->hp, Player::ATTACK3_DAMAGE);

                SerializedBuffer *damagePacket = packetPool.Alloc();
                damagePacket->Clear();
                Make_PACKET_SC_DAMAGE(damagePacket, player->playerID, currentPlayer->playerID, currentPlayer->hp);

                MapSectorGroup damagerSectorAround;
                GetSectorNear(currentPlayer->sector, &damagerSectorAround);
                SendPacketBroadcastSector(nullptr, &damagerSectorAround, damagePacket);

                damagePacket->Clear();
                packetPool.Free(damagePacket);

                if (currentPlayer->hp <= 0)
                {
                    currentPlayer->status = STATUS_DELETE;
                    currentPlayer->session->disconnectFlag = true;
                }
            }
        }
    }
}

VOID On_PACKET_CS_ECHO(Player *player, DWORD time)
{
    /*
    constexpr SIZE_T messageSize = 60;
    WCHAR message[messageSize] = { 0, };
    _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PACKET FROM [%d] <ECHO> %d>", player->playerID, time);
    ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
    logger->SaveLogDebug(__FUNCTIONW__, message);
    */

    framePerSecondEchoCnt++;

    SerializedBuffer *packet = packetPool.Alloc();
    packet->Clear();
    Make_PACKET_SC_ECHO(packet, time);
    SendPacketUnicast(player, packet);
    packet->Clear();
    packetPool.Free(packet);
}

VOID SendPacketUnicast(Player *receiver, SerializedBuffer *packet)
{
    if (receiver == nullptr) {
        logger->SaveLogError(L"SendPacketUnicast", L"receiver is nullptr");
        return;
    }

    if (packet == nullptr) {
        logger->SaveLogError(L"SendPacketUnicast", L"packet is nullptr");
        return;
    }

    if (receiver->session->sendQueue.GetSizeFree() < packet->GetBufferSizeUsed()) 
    {
        wrongErrorTotal++;
        receiver->status = STATUS_DELETE;
        receiver->session->disconnectFlag = true;
#ifdef DEBUG_LOGGING
        {
            constexpr SIZE_T messageSize = 60;
            WCHAR message[messageSize] = { 0, };
            _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PLAYER ID [%d] <DELETE> SENDQUEUE SMALL ERROR>", receiver->playerID);
            ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
            logger->SaveLogDebug(__FUNCTIONW__, message);
        }
#endif
        logger->SaveLogWarning(L"SendPacketUnicast", L"sendQueue is full, cannot add packet");
        return;
    }

    INT32 enqueueOutSize = 0;
    BOOL enqueueResult = receiver->session->sendQueue.Enqueue(packet->GetBufferReadPos(), packet->GetBufferSizeUsed(), &enqueueOutSize);
    if (!enqueueResult || enqueueOutSize != packet->GetBufferSizeUsed())
    {
        wrongErrorTotal++;
        receiver->status = STATUS_DELETE;
        receiver->session->disconnectFlag = true;
#ifdef DEBUG_LOGGING
        {
            constexpr SIZE_T messageSize = 60;
            WCHAR message[messageSize] = { 0, };
            _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PLAYER ID [%d] <DELETE> SENDQUEUE ENQUEUE ERROR>", receiver->playerID);
            ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
            logger->SaveLogDebug(__FUNCTIONW__, message);
        }
#endif
        logger->SaveLogError(L"SendPacketUnicast", L"sendQueue enqueue Error");
        return;
    }
    //sessionSendSet.insert(receiver->session);

    framePerSecondSendCnt++;
    byteSendReqPerSecondCnt += enqueueOutSize;
}

VOID SendPacketBroadcast(const Player *except, SerializedBuffer *packet)
{
    if (packet == nullptr) {
        logger->SaveLogError(L"SendPacketBroadcast", L"packet is nullptr");
        return;
    }

    auto sendIterator = playerMap.begin();
    Player *currentPlayer;
    while (sendIterator != playerMap.end())
    {
        currentPlayer = sendIterator->second;
        ++sendIterator;
        if (except != nullptr && except == currentPlayer) continue;
        SendPacketUnicast(currentPlayer, packet);
    }
}

VOID SendPacketBroadcastSector(const Player *except, MapSectorGroup *sectors, SerializedBuffer *packet)
{
    int sectorIndex = 0;
    for (sectorIndex = 0; sectorIndex < sectors->sectorCount; sectorIndex++)
    {
        MapSector currentSector = sectors->sectors[sectorIndex];
        auto playerSectorMapCurrent = playerSectorMap[currentSector.y][currentSector.x];
        auto playerIterator = playerSectorMapCurrent.begin();
        Player *currentPlayer;
        // 섹터 당 섹터 내 플레이어 순회
        while (playerIterator != playerSectorMapCurrent.end()) {
            currentPlayer = playerIterator->second;
            ++playerIterator;
            if (currentPlayer == except) continue;
            SendPacketUnicast(currentPlayer, packet);
        }
    }
}

VOID GetSectorNear(MapSector sector, MapSectorGroup *outSectorAround)
{
    outSectorAround->sectorCount = 0;
    int currentSectorX = 0;
    int currentSectorY = 0;

    int sectorCheckStartX = max(sector.x - 1, 0);
    int sectorCheckStartY = max(sector.y - 1, 0);
    int sectorCheckEndX = min(sector.x + 1, SECTOR_INDEX_SIZE_X - 1);
    int sectorCheckEndY = min(sector.y + 1, SECTOR_INDEX_SIZE_Y - 1);
    
    
    for (currentSectorY = sectorCheckStartY; currentSectorY <= sectorCheckEndY; currentSectorY++) 
    {
        for (currentSectorX = sectorCheckStartX; currentSectorX <= sectorCheckEndX; currentSectorX++) 
        {
            outSectorAround->sectors[outSectorAround->sectorCount++] = { currentSectorX, currentSectorY };
        }
    }
}

VOID GetSectorChanges(MapSector sectorBefore, MapSector sectorAfter, MapSectorGroup *outScopeSectors, MapSectorGroup *inScopeSectors)
{
    outScopeSectors->sectorCount = 0;
    inScopeSectors->sectorCount = 0;

    MapSectorGroup sectorAroundBefore;
    MapSectorGroup sectorAroundAfter;

    GetSectorNear(sectorBefore, &sectorAroundBefore);
    GetSectorNear(sectorAfter, &sectorAroundAfter);

    int sectorBeforeCounter = 0;
    int sectorAfterCounter = 0;
    for (sectorBeforeCounter = 0; sectorBeforeCounter < sectorAroundBefore.sectorCount; sectorBeforeCounter++) 
    {
        // 이전 주변 섹터들 대상으로 순회하면서
        BOOL isSectorAlsoExists = false;
        for (sectorAfterCounter = 0; sectorAfterCounter < sectorAroundAfter.sectorCount; sectorAfterCounter++)
        {
            // 현재 주변 섹터들 하나하나 체크하며 같은 섹터가 있는지 확인
            if (sectorAroundBefore.sectors[sectorBeforeCounter].x == sectorAroundAfter.sectors[sectorAfterCounter].x
                && sectorAroundBefore.sectors[sectorBeforeCounter].y == sectorAroundAfter.sectors[sectorAfterCounter].y) {
                isSectorAlsoExists = true;
            }
        }
        // 만일 현재 주변 섹터에 포함되지 않은 영역이라면 outScopeSector
        if (!isSectorAlsoExists) {
            outScopeSectors->sectors[outScopeSectors->sectorCount++] = sectorAroundBefore.sectors[sectorBeforeCounter];
        }
    }

    sectorAfterCounter = 0;
    sectorBeforeCounter = 0;
    for (sectorAfterCounter = 0; sectorAfterCounter < sectorAroundAfter.sectorCount; sectorAfterCounter++) 
    {
        // 현재 주변 섹터들 대상으로 순회하면서
        BOOL isSectorAlsoExists = false;
        for (sectorBeforeCounter = 0; sectorBeforeCounter < sectorAroundBefore.sectorCount; sectorBeforeCounter++) 
        {
            // 이전 주변 섹터들 하나하나 체크하며 같은 섹터가 있는지 확인
            if (sectorAroundBefore.sectors[sectorBeforeCounter].x == sectorAroundAfter.sectors[sectorAfterCounter].x
                && sectorAroundBefore.sectors[sectorBeforeCounter].y == sectorAroundAfter.sectors[sectorAfterCounter].y) {
                isSectorAlsoExists = true;
            }
        }
        // 만일 이전 주변 섹터에 포함되지 않은 영역이라면 inScopeSector
        if (!isSectorAlsoExists) {
            inScopeSectors->sectors[inScopeSectors->sectorCount++] = sectorAroundAfter.sectors[sectorAfterCounter];
        }
    }
}

VOID DisconnectClient(Player *player)
{
#ifdef DEBUG_LOGGING
    {
        constexpr SIZE_T messageSize = 60;
        WCHAR message[messageSize] = { 0, };
        _snwprintf_s(message, messageSize * sizeof(WCHAR), L"<PLAYER ID [%d] <DISCONNECT>>", player->playerID);
        ZeroMemory(message + messageSize - 1, sizeof(WCHAR));
        logger->SaveLogDebug(__FUNCTIONW__, message);
    }
#endif

    /** TODO
     *  1. 주변 섹터에만 패킷을 쏘도록 한다
     */
     // 주변 섹터 구하기
    MapSectorGroup sectorAround;
    GetSectorNear(player->sector, &sectorAround);

    SerializedBuffer *packet = packetPool.Alloc();
    packet->Clear();
    Make_PACKET_SC_DELETE_CHARACTER(packet, player->playerID);
    SendPacketBroadcastSector(player, &sectorAround, packet);
    packet->Clear();
    packetPool.Free(packet);
    RemovePlayer(player);
    DisconnectSession(player->session);
}
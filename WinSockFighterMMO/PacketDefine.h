#ifndef __PROTOCOL__
#define __PROTOCOL__

#include <WinSock2.h>
#include <Windows.h>

#define NETWORK_PORT			20000
#define NETWORK_PORT_MONITOR	20001

#define PACKET_CODE		0x89

#pragma pack(push, 1)
struct PACKET_HEADER
{
	BYTE	byteCode;			// 패킷코드 0x89 고정.
	BYTE	byteSize;			// 패킷 사이즈.
	BYTE	byteType;			// 패킷타입.
};

#define	PACKET_SC_CREATE_CHARACTER_MINE_			0
//---------------------------------------------------------------
// 클라이언트 자신의 캐릭터 할당		Server -> Client
//
// 서버에 접속시 최초로 받게되는 패킷으로 자신이 할당받은 ID 와
// 자신의 최초 위치, HP 를 받게 된다. (처음에 한번 받게 됨)
// 
// 이 패킷을 받으면 자신의 ID,X,Y,HP 를 저장하고 캐릭터를 생성시켜야 한다.
//
//	4	-	ID
//	1	-	Direction
//	2	-	X
//	2	-	Y
//	1	-	HP
//
//---------------------------------------------------------------
struct PACKET_SC_CREATE_CHARACTER_MINE : PACKET_HEADER {
	INT32	id;
	BYTE	direction;
	USHORT	posX;
	USHORT	posY;
	BYTE	hp;
};

#define	PACKET_SC_CREATE_CHARACTER_OTHERS_		1
//---------------------------------------------------------------
// 다른 클라이언트의 캐릭터 생성 패킷		Server -> Client
//
// 처음 서버에 접속시 이미 접속되어 있던 캐릭터들의 정보
// 또는 게임중에 접속된 클라이언트들의 생성 용 정보.
//
//
//	4	-	ID
//	1	-	Direction
//	2	-	X
//	2	-	Y
//	1	-	HP
//
//---------------------------------------------------------------
struct PACKET_SC_CREATE_CHARACTER_OTHERS : PACKET_HEADER {
	INT32	id;
	BYTE	direction;
	USHORT	posX;
	USHORT	posY;
	BYTE	hp;
};

#define	PACKET_SC_DELETE_CHARACTER_			2
//---------------------------------------------------------------
// 캐릭터 삭제 패킷						Server -> Client
//
// 캐릭터의 접속해제 또는 캐릭터가 죽었을때 전송됨.
//
//	4	-	ID
//
//---------------------------------------------------------------
struct PACKET_SC_DELETE_CHARACTER : PACKET_HEADER {
	INT32	id;
};


#define	PACKET_CS_MOVE_START_					10
#define PACKET_MOVE_DIR_LL_					0
#define PACKET_MOVE_DIR_LU_					1
#define PACKET_MOVE_DIR_UU_					2
#define PACKET_MOVE_DIR_RU_					3
#define PACKET_MOVE_DIR_RR_					4
#define PACKET_MOVE_DIR_RD_					5
#define PACKET_MOVE_DIR_DD_					6
#define PACKET_MOVE_DIR_LD_					7
//---------------------------------------------------------------
// 캐릭터 이동시작 패킷						Client -> Server
//
// 자신의 캐릭터 이동시작시 이 패킷을 보낸다.
// 이동 중에는 본 패킷을 보내지 않으며, 키 입력이 변경되었을 경우에만
// 보내줘야 한다.
//
// (왼쪽 이동중 위로 이동 / 왼쪽 이동중 왼쪽 위로 이동... 등등)
//
//	1	-	Direction	( 방향 디파인 값 8방향 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct PACKET_CS_MOVE_START : PACKET_HEADER {
	BYTE	direction;
	USHORT	posX;
	USHORT	posY;
};

#define	PACKET_SC_MOVE_START_					11
//---------------------------------------------------------------
// 캐릭터 이동시작 패킷						Server -> Client
//
// 다른 유저의 캐릭터 이동시 본 패킷을 받는다.
// 패킷 수신시 해당 캐릭터를 찾아 이동처리를 해주도록 한다.
// 
// 패킷 수신 시 해당 키가 계속해서 눌린것으로 생각하고
// 해당 방향으로 계속 이동을 하고 있어야만 한다.
//
//	4	-	ID
//	1	-	Direction	( 방향 디파인 값 8방향 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct PACKET_SC_MOVE_START : PACKET_HEADER {
	INT32	id;
	BYTE	direction;
	USHORT	posX;
	USHORT	posY;
};


#define	PACKET_CS_MOVE_STOP_					12
//---------------------------------------------------------------
// 캐릭터 이동중지 패킷						Client -> Server
//
// 이동중 키보드 입력이 없어서 정지되었을 때, 이 패킷을 서버에 보내준다.
//
//	1	-	Direction	( 방향 디파인 값 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct PACKET_CS_MOVE_STOP : PACKET_HEADER {
	BYTE	direction;
	USHORT	posX;
	USHORT	posY;
};

#define	PACKET_SC_MOVE_STOP_					13
//---------------------------------------------------------------
// 캐릭터 이동중지 패킷						Server -> Client
//
// ID 에 해당하는 캐릭터가 이동을 멈춘것이므로 
// 캐릭터를 찾아서 방향과, 좌표를 입력해주고 멈추도록 처리한다.
//
//	4	-	ID
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct PACKET_SC_MOVE_STOP : PACKET_HEADER {
	INT32	id;
	BYTE	direction;
	USHORT	posX;
	USHORT	posY;
};


#define	PACKET_CS_ATTACK1_						20
//---------------------------------------------------------------
// 캐릭터 공격 패킷							Client -> Server
//
// 공격 키 입력시 본 패킷을 서버에게 보낸다.
// 충돌 및 데미지에 대한 결과는 서버에서 알려 줄 것이다.
//
// 공격 동작 시작시 한번만 서버에게 보내줘야 한다.
//
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y	
//
//---------------------------------------------------------------
struct PACKET_CS_ATTACK1 : PACKET_HEADER {
	BYTE	direction;
	USHORT	posX;
	USHORT	posY;
};

#define	PACKET_SC_ATTACK1_						21
//---------------------------------------------------------------
// 캐릭터 공격 패킷							Server -> Client
//
// 패킷 수신시 해당 캐릭터를 찾아서 공격1번 동작으로 액션을 취해준다.
// 방향이 다를 경우에는 해당 방향으로 바꾼 후 해준다.
//
//	4	-	ID
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct PACKET_SC_ATTACK1 : PACKET_HEADER {
	INT32	id;
	BYTE	direction;
	USHORT	posX;
	USHORT	posY;
};


#define	PACKET_CS_ATTACK2_						22
//---------------------------------------------------------------
// 캐릭터 공격 패킷							Client -> Server
//
// 공격 키 입력시 본 패킷을 서버에게 보낸다.
// 충돌 및 데미지에 대한 결과는 서버에서 알려 줄 것이다.
//
// 공격 동작 시작시 한번만 서버에게 보내줘야 한다.
//
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct PACKET_CS_ATTACK2 : PACKET_HEADER {
	BYTE	direction;
	USHORT	posX;
	USHORT	posY;
};

#define	PACKET_SC_ATTACK2_						23
//---------------------------------------------------------------
// 캐릭터 공격 패킷							Server -> Client
//
// 패킷 수신시 해당 캐릭터를 찾아서 공격2번 동작으로 액션을 취해준다.
// 방향이 다를 경우에는 해당 방향으로 바꾼 후 해준다.
//
//	4	-	ID
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct PACKET_SC_ATTACK2 : PACKET_HEADER {
	INT32	id;
	BYTE	direction;
	USHORT	posX;
	USHORT	posY;
};

#define	PACKET_CS_ATTACK3_						24
//---------------------------------------------------------------
// 캐릭터 공격 패킷							Client -> Server
//
// 공격 키 입력시 본 패킷을 서버에게 보낸다.
// 충돌 및 데미지에 대한 결과는 서버에서 알려 줄 것이다.
//
// 공격 동작 시작시 한번만 서버에게 보내줘야 한다.
//
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct PACKET_CS_ATTACK3 : PACKET_HEADER {
	BYTE	direction;
	USHORT	posX;
	USHORT	posY;
};

#define	PACKET_SC_ATTACK3_						25
//---------------------------------------------------------------
// 캐릭터 공격 패킷							Server -> Client
//
// 패킷 수신시 해당 캐릭터를 찾아서 공격3번 동작으로 액션을 취해준다.
// 방향이 다를 경우에는 해당 방향으로 바꾼 후 해준다.
//
//	4	-	ID
//	1	-	Direction	( 방향 디파인 값. 좌/우만 사용 )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct PACKET_SC_ATTACK3 : PACKET_HEADER {
	INT32	id;
	BYTE	direction;
	USHORT	posX;
	USHORT	posY;
};

#define	PACKET_SC_DAMAGE_						30
//---------------------------------------------------------------
// 캐릭터 데미지 패킷							Server -> Client
//
// 공격에 맞은 캐릭터의 정보를 보냄.
//
//	4	-	AttackID	( 공격자 ID )
//	4	-	DamageID	( 피해자 ID )
//	1	-	DamageHP	( 피해자 HP )
//
//---------------------------------------------------------------
struct PACKET_SC_DAMAGE : PACKET_HEADER {
	INT32	idAttacker;
	INT32	idDamager;
	BYTE	hpDamagerRemain;
};

#define	PACKET_SC_SYNC_						251
//---------------------------------------------------------------
// 동기화를 위한 패킷					Server -> Client
//
// 서버로부터 동기화 패킷을 받으면 해당 캐릭터를 찾아서
// 캐릭터 좌표를 보정해준다.
//
//	4	-	ID
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct PACKET_SC_SYNC : PACKET_HEADER {
	INT32	id;
	USHORT	posX;
	USHORT	posY;
};

#define	PACKET_CS_ECHO_						252
//---------------------------------------------------------------
// Echo 용 패킷					Client -> Server
//
//	4	-	Time
//
//---------------------------------------------------------------
struct PACKET_CS_ECHO : PACKET_HEADER {
	DWORD	time;
};

#define	PACKET_SC_ECHO_						253
//---------------------------------------------------------------
// Echo 응답 패킷				Server -> Client
//
//	4	-	Time
//
//---------------------------------------------------------------
struct PACKET_SC_ECHO : PACKET_HEADER {
	DWORD	time;
};


#define PACKET_SS_MONITOR_				255
//---------------------------------------------------------------
// 모니터링 데이터 전송				Server -> Monitoring Server
//
//	4	-	ServerTime
//	4	-	ServerStart
//	2	-	CPU percentage
//	4	-	Memory Usage KB 
//	4	-	FPS Network
//	4	-	FPS Logic
//	2	-	SessionCount
//	2	-	PlayerCount
//	2	-	AcceptPS
//	2	-	EchoPS
//	4	-	SendPS
//	4	-	RecvPS
//	4	-	SendBytePS
//	4	-	RecvBytePS
//	2	-	SyncWrongTotal
//	2	-	CodeWrongTotal
//	2	-	ErrorWrongTotal
//	4	-	SessionPoolTotal
//	4	-	PlayerPoolTotal
//	4	-	PacketPoolTotal
//
//---------------------------------------------------------------
struct PACKET_SS_MONITOR : PACKET_HEADER
{
	DWORD	serverTime;
	DWORD	serverRunningTime;
	USHORT	serverCPU;
	DWORD	serverMemory;
	DWORD	fpsNetwork;
	DWORD	fpsLogic;
	USHORT	countSession;
	USHORT	countPlayer;
	USHORT	acceptPerSecond;
	USHORT	echoPerSecond;
	DWORD	packetSendPerSecond;
	DWORD	packetRecvPerSecond;
	DWORD	byteSendPerSecond;
	DWORD	byteRecvPerSecond;
	USHORT	wrongSyncTotal;
	USHORT	wrongCodeTotal;
	USHORT	wrongErrorTotal;
	DWORD	poolSessionTotal;
	DWORD	poolPlayerTotal;
	DWORD	poolPacketTotal;
};

#pragma pack(pop)

#endif
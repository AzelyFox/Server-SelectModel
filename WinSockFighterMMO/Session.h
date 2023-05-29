#pragma once

#include <WinSock2.h>
#include <map>
#include <unordered_map>

#include "SimpleLogger.h"
#include "MemoryPool.h"
#include "RingBuffer.h"

using namespace std;
using namespace azely;

namespace azely {

	struct Session
	{
		SOCKET		socket;
		DWORD		sessionID;
		RingBuffer	sendQueue;
		RingBuffer	recvQueue;
		DWORD		lastRecvTime;
		BOOL		disconnectFlag;
	};

	Session		*FindSession(SOCKET socket);
	Session		*CreateSession(SOCKET socket);
	VOID		DisconnectSession(SOCKET socket);
	VOID		DisconnectSession(Session *session);

}

extern SimpleLogger							*logger;
extern INT32								sessionNextID;
extern MemoryPool<Session>					sessionPool;
extern unordered_map<SOCKET, Session *>		sessionMap;
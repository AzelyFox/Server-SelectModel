#include "stdafx.h"
#include "Session.h"

namespace azely {

    Session *FindSession(SOCKET socket)
    {
        auto findSessionResult = sessionMap.find(socket);
        if (findSessionResult == sessionMap.end()) {
            WCHAR buffer[32] = { 0, };
            _snwprintf_s(buffer, 32 * 2, L"FindSession %lld not found", socket);
            logger->SaveLogWarning(L"FindSession", buffer);
            return nullptr;
        }
        return findSessionResult->second;
    }

    Session *CreateSession(SOCKET socket)
    {
        Session *newSession = sessionPool.Alloc();

        newSession->socket = socket;
        newSession->sessionID = sessionNextID++;
        newSession->lastRecvTime = timeGetTime();
        newSession->sendQueue.ClearBuffer();
        newSession->recvQueue.ClearBuffer();
        newSession->disconnectFlag = false;

        sessionMap.insert(pair<SOCKET, Session *>(socket, newSession));

        return newSession;
    }

    VOID DisconnectSession(SOCKET socket)
    {
        auto findSessionResult = sessionMap.find(socket);
        if (findSessionResult == sessionMap.end()) {
            WCHAR buffer[32] = { 0, };
            _snwprintf_s(buffer, 32 * 2, L"DisconnectSession %lld not found", socket);
            logger->SaveLogWarning(L"DisconnectSession", buffer);
            return;
        }
        Session *disconnectingSession = findSessionResult->second;

        DisconnectSession(disconnectingSession);
    }

    VOID DisconnectSession(Session *session)
    {
        closesocket(session->socket);

        session->recvQueue.ClearBuffer();
        session->sendQueue.ClearBuffer();
        session->lastRecvTime = 0;

        sessionMap.erase(session->socket);
        sessionPool.Free(session);
    }

}
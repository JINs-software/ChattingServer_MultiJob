#include "ChattingServer.h"

using namespace std;

bool ChattingServer::OnWorkerThreadCreate(HANDLE thHnd)
{
	return true;
}

void ChattingServer::OnAllWorkerThreadCreate()
{
}

void ChattingServer::OnWorkerThreadStart()
{
	m_MessagePoolMgr.AllocTlsMemPool();
}

void ChattingServer::OnClientJoin(UINT64 sessionID, const SOCKADDR_IN& clientSockAddr)
{
	lock_guard<mutex> lockGuard(m_LoginWaitSessionSetMtx);
	m_LoginWaitSessionSet.insert(sessionID);
}

void ChattingServer::OnClientLeave(UINT64 sessionID)
{
	{
		lock_guard<mutex> lockGuard(m_LoginWaitSessionSetMtx);
		if (m_LoginWaitSessionSet.find(sessionID) != m_LoginWaitSessionSet.end()) {
			m_LoginWaitSessionSet.erase(sessionID);
			return;
		}
	}

	// 플레이어 정리
}

void ChattingServer::OnRecv(UINT64 sessionID, JBuffer& recvBuff)
{
	WORD type;
	//recvBuff.Peek(&type);
	recvBuff >> type;

	switch (type)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN: 
	{
		bool releaseBeforeLogin = false;
		{
			lock_guard<mutex> lockGuard(m_LoginWaitSessionSetMtx);
			if (m_LoginWaitSessionSet.find(sessionID) != m_LoginWaitSessionSet.end()) {
				m_LoginWaitSessionSet.erase(sessionID);
			}
			else {
				releaseBeforeLogin = true;
			}
		}
		if (!releaseBeforeLogin)	break;

		INT64	AccountNo;
		WCHAR	ID[20];				// null 포함
		WCHAR	Nickname[20];		// null 포함
		char	SessionKey[64];		// 인증토큰

		recvBuff >> AccountNo;
		recvBuff >> ID;
		recvBuff >> Nickname;
		recvBuff >> SessionKey;

		// ... 인증 절차


		// 인증 성공 이 후
		Player* newPlayer = new Player(this, sessionID, AccountNo, ID, Nickname);

		// SessionPlayer
		AcquireSRWLockExclusive(&m_SessionPlayerMapSrwLock);
		m_SessionPlayerMap.insert({ sessionID, newPlayer });
		ReleaseSRWLockExclusive(&m_SessionPlayerMapSrwLock);

		JBuffer* reply = AllocSerialSendBuff(sizeof(MSG_PACKET_CS_CHAT_RES_LOGIN));
		*reply << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << (BYTE)1 << AccountNo;
		if (!SendPacket(sessionID, reply)) {
			FreeSerialBuff(reply);
		}
	}
	break;
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
	{
		INT64	AccountNo;			// 사용 X ?
		WORD	SectorX;
		WORD	SectorY;

		recvBuff >> AccountNo;
		recvBuff >> SectorX;
		recvBuff >> SectorY;

		AcquireSRWLockShared(&m_SessionPlayerMapSrwLock);
		m_SessionPlayerMap[sessionID]->HandleJob_Async(&Player::Move, (WORD)10, (WORD)20);
		ReleaseSRWLockShared(&m_SessionPlayerMapSrwLock);
	}
	break;
	case en_PACKET_CS_CHAT_REQ_MESSAGE:
	{
		INT64	AccountNo;
		WORD	MessageLen;
		recvBuff >> AccountNo;
		recvBuff >> MessageLen;

		stMessage* chatMsg = m_MessagePoolMgr.GetTlsMemPool().AllocMem();
		recvBuff >> chatMsg->messageLen;
		recvBuff.Dequeue(chatMsg->message, chatMsg->messageLen);

		AcquireSRWLockShared(&m_SessionPlayerMapSrwLock);
		m_SessionPlayerMap[sessionID]->HandleJob_Async(&Player::Chat, chatMsg);
		ReleaseSRWLockShared(&m_SessionPlayerMapSrwLock);
	}
	break;
	default:
		DebugBreak();
	}
}

void ChattingServer::OnRecv(UINT64 sessionID, JSerialBuffer& recvBuff)
{
}

void ChattingServer::OnPrintLogOnConsole()
{
}

void ChattingServer::Player::Move(WORD NY, WORD NX)
{
	if (Y == NY && X == NX)	return;
	if (NY < 0 || NY >= dfSECTOR_Y_MAX || NX < 0 || NX >= dfSECTOR_X_MAX)	return;

	if (X == (WORD)-1) {	// 초기 MOVE 메시지
		AcquireSRWLockExclusive(&Serv->m_SectorMapSrwLocks[NY][NX]);
		Serv->m_SessionPlayerMap.insert({ SessionID, this });
		ReleaseSRWLockExclusive(&Serv->m_SectorMapSrwLocks[NY][NX]);
		return;
	}

	pair<WORD, WORD> ulPos, drPos;
	if (Y < NY) { ulPos = { Y, X }; drPos = { NY, NX }; }
	else if (Y == NY) {
		if (X < NX) { ulPos = { Y, X }; drPos = { NY, NX }; }
		else { ulPos = { NY, NX }; drPos = { Y, X }; };
	}
	else { ulPos = { NY, NX }; drPos = { Y, X }; }

	AcquireSRWLockExclusive(&Serv->m_SectorMapSrwLocks[ulPos.first][ulPos.second]);
	AcquireSRWLockExclusive(&Serv->m_SectorMapSrwLocks[drPos.first][drPos.second]);
	Serv->m_SessionPlayerMap.erase(SessionID);
	Serv->m_SessionPlayerMap.insert({ SessionID, this });
	ReleaseSRWLockExclusive(&Serv->m_SectorMapSrwLocks[drPos.first][drPos.second]);
	ReleaseSRWLockExclusive(&Serv->m_SectorMapSrwLocks[ulPos.first][ulPos.second]);
}

void ChattingServer::Player::Chat(stMessage* chatMessage)
{
	JBuffer* chat = Serv->AllocSerialSendBuff(sizeof(MSG_PACKET_CS_CHAT_RES_MESSAGE) + chatMessage->messageLen);
	*chat << (WORD)en_PACKET_CS_CHAT_RES_MESSAGE;
	*chat << AccountNo << ID << Nickname << chatMessage->messageLen;
	chat->Enqueue(chatMessage->message, chatMessage->messageLen);
	Serv->m_MessagePoolMgr.GetTlsMemPool().FreeMem(chatMessage);

	for (int y = Y - dfRangeHalfY; y <= Y + dfRangeHalfY; y++) {
		for (int x = X - dfRangeHalfX; x <= X + dfRangeHalfY; x++) {
			if (y < 0 || y > dfSECTOR_Y_MAX || x < 0 || x > dfSECTOR_X_MAX) continue;

			AcquireSRWLockShared(&Serv->m_SectorMapSrwLocks[y][x]);
			const auto sector = Serv->m_SectorMap[y][x];
			for (const auto p : sector) {
				Serv->AddRefSerialBuff(chat);
				if (!Serv->SendPacket(p.second->SessionID, chat)) {
					Serv->FreeSerialBuff(chat);
				}
			}
			ReleaseSRWLockShared(&Serv->m_SectorMapSrwLocks[y][x]);
		}
	}
}

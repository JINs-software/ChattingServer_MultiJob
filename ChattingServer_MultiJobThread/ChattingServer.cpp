#include "ChattingServer.h"
#include "CRedisConn.h"

using namespace std;

bool ChattingServer::Start()
{
	if (m_ConnToRedis) {
		// Connect to Redis
		bool firstConn = true;
		for (uint16 i = 0; i < m_NumOfIocpWorkers; i++) {
			RedisCpp::CRedisConn* redisConn = new RedisCpp::CRedisConn();	// 형식 지정자가 필요합니다.
			if (redisConn == NULL) {
				cout << "[LoginServer::Start] new RedisCpp::CRedisConn() return NULL" << endl;
				return false;
			}

			if (!redisConn->connect(TOKEN_AUTH_REDIS_IP, TOKEN_AUTH_REDIS_PORT)) {
				cout << "[ChattingServer::Start] new RedisCpp::CRedisConn(); return NULL" << endl;
				return false;
			}

			if (!redisConn->ping()) {
				cout << "[ChattingServer::Start] m_RedisConn->connect(..) return FALSE" << endl;
				return false;
			}

			m_RedisConnPool.Enqueue(redisConn);
		}
	}

	//m_ChatServerMont = new ChattingServerMont(this,
	//	MONT_SERVER_IP, MONT_SERVER_PORT,
	//	MONT_SERVER_PROTOCOL_CODE,
	//	MONT_CLIENT_IOCP_CONCURRENT_THRD, MONT_CLIENT_IOCP_WORKER_THRD_CNT,
	//	MONT_CLIENT_MEM_POOL_UNIT_CNT, MONT_CLIENT_MEM_POOL_UNIT_CAPACITY,
	//	MONT_CLIENT_MEM_POOL_BUFF_ALLOC_SIZE,
	//	MONT_CLIENT_RECV_BUFF_SIZE
	//);

	return JNetServer::Start();
}

void ChattingServer::Stop()
{
	m_StopFlag = true;

	if (m_ConnToRedis) {
		while (m_RedisConnPool.GetSize() > 0) {
			RedisCpp::CRedisConn* redisConn = NULL;
			m_RedisConnPool.Dequeue(redisConn);
			if (redisConn != NULL) {
				delete redisConn;
			}
		}
	}

	JNetServer::Stop();
}

//bool ChattingServer::OnWorkerThreadCreate(HANDLE thHnd) {}
//void ChattingServer::OnAllWorkerThreadCreate() {}

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
		if (releaseBeforeLogin)	break;

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
		//Player* newPlayer = new Player(this, sessionID, AccountNo, ID, Nickname);

		// SessionPlayer
		AcquireSRWLockExclusive(&m_SessionPlayerMapSrwLock);
		//m_SessionPlayerMap.insert({ sessionID, newPlayer });
		m_SessionPlayerMap.insert({ sessionID, make_shared<Player>(this, sessionID, AccountNo, ID, Nickname) });
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
		m_SessionPlayerMap[sessionID]->HandleJob_Async(&Player::Move, (WORD)SectorY, (WORD)SectorX);
		ReleaseSRWLockShared(&m_SessionPlayerMapSrwLock);
	}
	break;
	case en_PACKET_CS_CHAT_REQ_MESSAGE:
	{
		INT64	AccountNo;
		WORD	MessageLen;
		stMessage* chatMsg = m_MessagePoolMgr.GetTlsMemPool().AllocMem();

		recvBuff >> AccountNo;
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
	//while (recvBuff.GetUseSize() > 0) {
	//	OnRecv(sessionID, reinterpret_cast<JBuffer&>(recvBuff));
	//}
	DebugBreak();
}

void ChattingServer::OnPrintLogOnConsole()
{
}

void ChattingServer::Player::Move(WORD NY, WORD NX)
{
	if (NY < 0 || NY >= dfSECTOR_Y_MAX || NX < 0 || NX >= dfSECTOR_X_MAX) {
		Serv->Disconnect(SessionID);
		return;
	}

	if (X == (WORD)-1) {	// 초기 MOVE 메시지
		AcquireSRWLockExclusive(&Serv->m_SectorMapSrwLocks[NY][NX]);
		//Serv->m_SessionPlayerMap.insert({ SessionID, this });
		Serv->m_SectorMap[NY][NX].insert({ SessionID, get_shared_from_this() });
		ReleaseSRWLockExclusive(&Serv->m_SectorMapSrwLocks[NY][NX]);
	}
	else if (X == NX && Y == NY) {
		// skip..
	}
	else {
		pair<WORD, WORD> ulPos, drPos;
		if (Y < NY) { ulPos = { Y, X }; drPos = { NY, NX }; }
		else if (Y == NY) {
			if (X < NX) { ulPos = { Y, X }; drPos = { NY, NX }; }
			else { ulPos = { NY, NX }; drPos = { Y, X }; };
		}
		else { ulPos = { NY, NX }; drPos = { Y, X }; }

		AcquireSRWLockExclusive(&Serv->m_SectorMapSrwLocks[ulPos.first][ulPos.second]);
		AcquireSRWLockExclusive(&Serv->m_SectorMapSrwLocks[drPos.first][drPos.second]);
		Serv->m_SectorMap[Y][X].erase(SessionID);
		Serv->m_SectorMap[NY][NX].insert({ SessionID, get_shared_from_this() });
		ReleaseSRWLockExclusive(&Serv->m_SectorMapSrwLocks[drPos.first][drPos.second]);
		ReleaseSRWLockExclusive(&Serv->m_SectorMapSrwLocks[ulPos.first][ulPos.second]);
	}

	Y = NY;
	X = NX;
	JBuffer* reply = Serv->AllocSerialSendBuff(sizeof(MSG_PACKET_CS_CHAT_RES_SECTOR_MOVE));
	*reply << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE << AccountNo << X << Y;
	if (!Serv->SendPacket(SessionID, reply)) {
		Serv->FreeSerialBuff(reply);
	}
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

	Serv->FreeSerialBuff(chat);
}

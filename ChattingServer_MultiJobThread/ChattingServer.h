#pragma once
#include "JNetCore.h"
#include "JobHandler.h"
#include "Protocol.h"
#include "Configuration.h"

using namespace jnet;

//#define dfSECTOR_X_MAX								50
//#define dfSECTOR_Y_MAX								50

#define dfRangeHalfX								1
#define dfRangeHalfY								1

#define dfMaximumMsgLen								100

#define dfMessagePoolCapacity						1000

class ChattingServerMont;
namespace RedisCpp {
	class CRedisConn;
}

class ChattingServer : public JNetServer
{
	struct stMessage
	{
		WORD	messageLen;
		BYTE	message[dfMaximumMsgLen];
	};

	class Player : public JobHandler
	{
	public:
		Player(ChattingServer* serv, SessionID64 sessionID, INT64 accountNo, WCHAR id[20], WCHAR nickName[20])
			: Serv(serv), SessionID(sessionID), AccountNo(accountNo), X((WORD)-1)
		{
			memcpy(ID, id, sizeof(id));
			memcpy(Nickname, nickName, sizeof(Nickname));
		}

		std::shared_ptr<Player> get_shared_from_this() {
			return std::static_pointer_cast<Player>(shared_from_this());
		}
		void Move(WORD NY, WORD NX);
		void Chat(stMessage* chatMessage);

	private:
		ChattingServer* Serv;
		SessionID64	SessionID;
		INT64 AccountNo;
		WCHAR ID[20];			// null 포함
		WCHAR Nickname[20];		// null 포함
		//char sessinKey[64];		// 인증 토큰

		WORD Y;
		WORD X;
	};

	bool										m_StopFlag;

	std::mutex									m_LoginWaitSessionSetMtx;
	std::set<SessionID64>						m_LoginWaitSessionSet;

	SRWLOCK												m_SessionPlayerMapSrwLock;
	std::unordered_map<SessionID64, shared_ptr<Player>>	m_SessionPlayerMap;

	SRWLOCK												m_SectorMapSrwLocks[dfSECTOR_Y_MAX + 1][dfSECTOR_X_MAX + 1];
	std::unordered_map<SessionID64, shared_ptr<Player>>	m_SectorMap[dfSECTOR_Y_MAX + 1][dfSECTOR_X_MAX + 1];

	TlsMemPoolManager<stMessage, false, false>  m_MessagePoolMgr;

	bool m_ConnToRedis;
	LockFreeQueue<RedisCpp::CRedisConn*>		m_RedisConnPool;
	uint32										m_NumOfIocpWorkers;

public:
	ChattingServer(
		bool	connToRedis,
		const char* serverIP, uint16	serverPort, uint16	maximumOfConnections,
		BYTE	packetCode_LAN, BYTE	packetCode, BYTE	packetSymmetricKey,
		bool	recvBufferingMode,
		uint16	maximumOfSessions,
		uint32	numOfIocpConcurrentThrd, uint16	numOfIocpWorkerThrd,
		size_t	tlsMemPoolUnitCnt, size_t	tlsMemPoolUnitCapacity,
		uint32	memPoolBuffAllocSize,
		uint32	sessionRecvBuffSize,
		bool	calcTpsThread
	) : JNetServer(
		serverIP, serverPort, maximumOfConnections,
		packetCode_LAN, packetCode, packetSymmetricKey,
		recvBufferingMode,
		maximumOfSessions,
		numOfIocpConcurrentThrd, numOfIocpWorkerThrd,
		tlsMemPoolUnitCnt, tlsMemPoolUnitCapacity,
		memPoolBuffAllocSize,
		sessionRecvBuffSize,
		calcTpsThread
	),
		m_StopFlag(false), m_MessagePoolMgr(0, dfMessagePoolCapacity), 
		m_ConnToRedis(connToRedis), m_NumOfIocpWorkers(numOfIocpWorkerThrd)
	{}

	bool Start();
	void Stop();

private:
	//virtual bool OnWorkerThreadCreate(HANDLE thHnd) override;
	//virtual void OnAllWorkerThreadCreate() override;	// -> 프로세싱(업데이트) 스레드 생성
	virtual void OnWorkerThreadStart() override;
	virtual void OnClientJoin(UINT64 sessionID, const SOCKADDR_IN& clientSockAddr) override;	// -> 세션 접속 메시지 업데이트 메시지 큐에 큐잉
	virtual void OnClientLeave(UINT64 sessionID) override;	// -> 세션 해제 메시지 업데이트 메시지 큐에 큐잉 
	virtual void OnRecv(UINT64 sessionID, JBuffer& recvBuff) override;
	virtual void OnRecv(UINT64 sessionID, JSerialBuffer& recvBuff) override;
	virtual void OnPrintLogOnConsole() override;

};

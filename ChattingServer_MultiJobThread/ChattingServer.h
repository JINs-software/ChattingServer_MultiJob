#pragma once
#include "JNetCore.h"
#include "JobHandler.h"
#include "Protocol.h"

using namespace jnet;

#define dfSECTOR_X_MAX								50
#define dfSECTOR_Y_MAX								50

#define dfRangeHalfX								1
#define dfRangeHalfY								1

#define dfMaximumMsgLen								100

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

		void Move(WORD NY, WORD NX);
		void Chat(stMessage* chatMessage);

	private:
		ChattingServer* Serv;
		SessionID64	SessionID;
		INT64 AccountNo;
		WCHAR ID[20];			// null ����
		WCHAR Nickname[20];		// null ����
		//char sessinKey[64];		// ���� ��ū

		WORD Y;
		WORD X;
	};

	std::mutex									m_LoginWaitSessionSetMtx;
	std::set<SessionID64>						m_LoginWaitSessionSet;

	SRWLOCK										m_SessionPlayerMapSrwLock;
	std::unordered_map<SessionID64, Player*>	m_SessionPlayerMap;

	SRWLOCK										m_SectorMapSrwLocks[dfSECTOR_Y_MAX + 1][dfSECTOR_X_MAX + 1];
	std::unordered_map<SessionID64, Player*>	m_SectorMap[dfSECTOR_Y_MAX + 1][dfSECTOR_X_MAX + 1];

	TlsMemPoolManager<stMessage, false, false>  m_MessagePoolMgr;

private:
	virtual bool OnWorkerThreadCreate(HANDLE thHnd) override;
	virtual void OnAllWorkerThreadCreate() override;	// -> ���μ���(������Ʈ) ������ ����
	virtual void OnWorkerThreadStart() override;
	virtual void OnClientJoin(UINT64 sessionID, const SOCKADDR_IN& clientSockAddr) override;	// -> ���� ���� �޽��� ������Ʈ �޽��� ť�� ť��
	virtual void OnClientLeave(UINT64 sessionID) override;	// -> ���� ���� �޽��� ������Ʈ �޽��� ť�� ť�� 
	virtual void OnRecv(UINT64 sessionID, JBuffer& recvBuff) override;
	virtual void OnRecv(UINT64 sessionID, JSerialBuffer& recvBuff) override;
	virtual void OnPrintLogOnConsole() override;

};

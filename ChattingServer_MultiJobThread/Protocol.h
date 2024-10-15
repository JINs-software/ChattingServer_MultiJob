#pragma once

#include <minwindef.h>

enum en_PACKET_TYPE
{
	////////////////////////////////////////////////////////
	//
	//	Client & Server Protocol
	//
	////////////////////////////////////////////////////////

	//------------------------------------------------------
	// Chatting Server
	//------------------------------------------------------
	en_PACKET_CS_CHAT_SERVER = 0,

	//------------------------------------------------------------
	// ä�ü��� �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]				// null ����
	//		WCHAR	Nickname[20]		// null ����
	//		char	SessionKey[64];		// ������ū
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_LOGIN,

	//------------------------------------------------------------
	// ä�ü��� �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status				// 0:����	1:����
	//		INT64	AccountNo
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_LOGIN,

	//------------------------------------------------------------
	// ä�ü��� ���� �̵� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_SECTOR_MOVE,

	//------------------------------------------------------------
	// ä�ü��� ���� �̵� ���
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_SECTOR_MOVE,

	//------------------------------------------------------------
	// ä�ü��� ä�ú����� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null ������
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_REQ_MESSAGE,

	//------------------------------------------------------------
	// ä�ü��� ä�ú����� ����  (�ٸ� Ŭ�� ���� ä�õ� �̰ɷ� ����)
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]						// null ����
	//		WCHAR	Nickname[20]				// null ����
	//		
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null ������
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_CHAT_RES_MESSAGE,

	//------------------------------------------------------------
	// ��Ʈ��Ʈ
	//
	//	{
	//		WORD		Type
	//	}
	//
	//
	// Ŭ���̾�Ʈ�� �̸� 30�ʸ��� ������.
	// ������ 40�� �̻󵿾� �޽��� ������ ���� Ŭ���̾�Ʈ�� ������ ������� ��.
	//------------------------------------------------------------	
	en_PACKET_CS_CHAT_REQ_HEARTBEAT,

	en_SESSION_JOIN,
	en_SESSION_RELEASE,
	//en_SESSION_RELEASE_BEFORE_LOGIN
};

//#endif

#pragma pack(push, 1)
struct MSG_PACKET_CS_CHAT_REQ_LOGIN {
	WORD Type;
	INT64 AccountNo;
	WCHAR ID[20];			// null ����
	WCHAR Nickname[20];		// null ����
	char sessionKey[64];		// ���� ��ū
};
struct MSG_PACKET_CS_CHAT_RES_LOGIN {
	WORD	Type;
	BYTE	Status;				// 0:����	1:����
	INT64	AccountNo;
};
struct MSG_PACKET_CS_CHAT_REQ_SECTOR_MOVE {
	WORD	Type;
	INT64	AccountNo;
	WORD	SectorX;
	WORD	SectorY;
};
struct MSG_PACKET_CS_CHAT_RES_SECTOR_MOVE {
	WORD	Type;
	INT64	AccountNo;
	WORD	SectorX;
	WORD	SectorY;
};
struct MSG_PACKET_CS_CHAT_REQ_MESSAGE {
	WORD	Type;
	INT64	AccountNo;
	WORD	MessageLen;

	//WCHAR	Message[MessageLen / 2];		// null ������
	//WCHAR* Message;
};
struct MSG_PACKET_CS_CHAT_RES_MESSAGE {
	WORD	Type;
	INT64	AccountNo;
	WCHAR	ID[20];						// null ����
	WCHAR	Nickname[20];				// null ����

	WORD	MessageLen;

	//WCHAR	Message[MessageLen / 2];		// null ������
	//WCHAR* Message;
};
struct MSG_PACKET_CS_CHAT_REQ_HEARTBEAT {
	WORD		Type;
};

struct stMSG_MONITOR_DATA_UPDATE {
	WORD	Type;
	BYTE	DataType;
	int		DataValue;
	int		TimeStamp;

};

#pragma pack(pop)

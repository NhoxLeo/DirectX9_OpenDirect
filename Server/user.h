#pragma once


// Include files required to compile this header
#include <winsock2.h>

enum EndWaitResult
{
	EWR_RECVPACKET,
	EWR_DISCONNECT,
	EWR_TIMEOUT,
};

class User
{
public:

	User();
	~User();
	HRESULT Create(DWORD dwId, WORD wPort);
	VOID Destroy();

	DWORD GetId();

	HRESULT Connect(const SOCKADDR_IN * pAddress, LPTHREAD_START_ROUTINE lpProcAddress);
	VOID Disconnect();
	BOOL IsConnected();
	VOID ThreadFinished();

	EndWaitResult WaitForPackets(DWORD dwTimeout);
	int SendPacket(const CHAR * pBuffer, int length);
	int RecvPacket(char * pBuffer, int length);


protected:

	BOOL m_bConnected;
	DWORD m_dwId;
	HANDLE m_hThread;
	WSAEVENT m_hRecvEvent;
	SOCKET m_sSocket;
	HANDLE m_hDisconnectEvent;
};
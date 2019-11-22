#pragma once
#include "stdafx.h"
#include "Data.h"


class NetworkClient : public Object
{
public:
	NetworkClient();
	~NetworkClient();

	// Networking structures
	SOCKET sSocket;
	HANDLE hRecvEvent;
	OtherPlayer players[MAX_USERS];
	Player player;
	//ZeroMemory(players, sizeof(players));
	std::vector<NetworkEntity*>* entities;

	void Update(float deltaTime) override;

	bool InitializeWinsock(SOCKET * pSocket, HANDLE * pRecvEvent);
	bool ConnectToServer(SOCKET sSocket, HANDLE hRecvEvent);
	bool ProcessNetworkMessages(OtherPlayer * pPlayers, SOCKET sSocket, HANDLE hRecvEvent);
	bool ProcessPacket(OtherPlayer * pPlayers, const CHAR * pBuffer, DWORD dwSize);
	bool UpdateOtherPlayer(int _id, OtherPlayer * pPlayer, UpdatePlayerMessage * pUpm);
};


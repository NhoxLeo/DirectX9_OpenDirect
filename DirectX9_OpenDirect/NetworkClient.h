#pragma once
#include "stdafx.h"
#include "Data.h"
//#pragma comment (lib,"Ws2_32.lib") // For Release only


class NetworkClient : public Object
{
public:
	NetworkClient();
	NetworkClient(std::string _ipAddress);
	~NetworkClient();

	// Networking structures
	int id;
	std::string ipAddress;
	SOCKET sSocket;
	HANDLE hRecvEvent;
	OtherPlayer players[MAX_USERS];
	//ZeroMemory(players, sizeof(players));
	std::vector<NetworkEntity*>* entities;
	void Update(float deltaTime) override;

	bool InitializeWinsock(SOCKET * pSocket, HANDLE * pRecvEvent);
	bool ConnectToServer(std::string _ipAddress, SOCKET sSocket, HANDLE hRecvEvent);
	bool ProcessNetworkMessages(OtherPlayer * pPlayers, SOCKET sSocket, HANDLE hRecvEvent);
	bool ProcessPacket(OtherPlayer * pPlayers, const CHAR * pBuffer, DWORD dwSize);
	bool UpdateOtherPlayer(int _id, OtherPlayer * pPlayer, UpdatePlayerMessage * pUpm);
	void DisconnectFromServer(SOCKET sSocket);
};


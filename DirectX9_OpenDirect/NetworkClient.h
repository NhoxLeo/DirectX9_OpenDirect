#pragma once
#include "stdafx.h"
#include "Data.h"
#include "MemoryBitStream.h"
#include<thread>
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
	std::vector<NetworkEntity*>* entities;
	std::vector<Object*>* bullets;
	NetworkEntity* thisEntity;
	bool isConnected;
	ID3DXFont* my_font;
	bool isAlive;
	Vector2 currentPosition;
	Vector2 normalVec;

	Messenger* serverCurrentInput = NULL;
	std::vector<Messenger> pendingInputs;
	int input_sequence_number = 0;

	float fireRate,fireTime;

	std::thread clientThread;
	bool allThreadRunning;

	float deltatime;
	void Update(float deltaTime) override;
	void Render() override;

	bool InitializeWinsock(SOCKET * pSocket, HANDLE * pRecvEvent);
	bool ConnectToServer(std::string _ipAddress, SOCKET sSocket, HANDLE hRecvEvent);
	bool ProcessNetworkMessages(SOCKET sSocket, HANDLE hRecvEvent);
	bool ProcessPacket(const CHAR * pBuffer, DWORD dwSize);
	bool UpdateOtherPlayer(int _id, UpdateMessage * pUpm);
	void DisconnectFromServer(SOCKET sSocket);
	void NetworkThread(SOCKET _socket, HANDLE _recvEvent);
};


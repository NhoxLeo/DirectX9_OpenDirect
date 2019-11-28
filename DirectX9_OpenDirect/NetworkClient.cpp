#include "NetworkClient.h"



NetworkClient::NetworkClient()
{
	ipAddress = "127.0.0.1";
	if (InitializeWinsock(&sSocket, &hRecvEvent) && ConnectToServer(ipAddress, sSocket, hRecvEvent));
	else cout << "failed";
	entities = new vector<NetworkEntity*>();
}
NetworkClient::NetworkClient(std::string _ipAddress)
{
	entities = new vector<NetworkEntity*>();
	ipAddress = _ipAddress;
	if (InitializeWinsock(&sSocket, &hRecvEvent) && ConnectToServer(_ipAddress, sSocket, hRecvEvent));
	else cout << "failed";
}
NetworkClient::~NetworkClient()
{
	DisconnectFromServer(sSocket);
}
void NetworkClient::Update(float deltaTime)
{
	int x = 0, y = 0;
	if (Input::GetInstance()->GetKeyState('A') == KeyState::Pressed) x = -1;
	if (Input::GetInstance()->GetKeyState('D') == KeyState::Pressed) x = 1;
	if (Input::GetInstance()->GetKeyState('W') == KeyState::Pressed) y = -1;
	if (Input::GetInstance()->GetKeyState('S') == KeyState::Pressed) y = 1;

	Object::Update(deltaTime);
	UpdatePlayerMessage upm;
	upm.dwPlayerID = 0;
	upm.fVelocity[0] = x;
	upm.fVelocity[1] = y;
	upm.fVelocity[2] = 0;
	upm.fSize[0] = this->GetSize().x;
	upm.fSize[1] = this->GetSize().y;

	// Send off the packet
	int a = send(sSocket, (CHAR*)&upm, sizeof(upm), 0);

	// Update the messages from the server
	ProcessNetworkMessages(players, sSocket, hRecvEvent);
}

bool NetworkClient::InitializeWinsock(SOCKET * pSocket, HANDLE * pRecvEvent)
{
	// Start up Winsock
	{
		// Stores information about Winsock
		WSADATA wsaData;
		// Call the DLL initialization function
		if (SOCKET_ERROR == WSAStartup(WINSOCK_VERSION, &wsaData) || wsaData.wVersion != WINSOCK_VERSION) return false;
	}

	// Set up the main socket 
	SOCKET sSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// Create the event
	HANDLE hRecvEvent = WSACreateEvent();
	if (SOCKET_ERROR == WSAEventSelect(sSocket, hRecvEvent, FD_READ))
	{
		closesocket(sSocket);
		return false;
	}

	// Store data
	*pSocket = sSocket;
	*pRecvEvent = hRecvEvent;

	// Success
	return true;
}
bool NetworkClient::ConnectToServer(std::string _ipAddress, SOCKET sSocket, HANDLE hRecvEvent)
{
	SOCKADDR_IN addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_COMM_PORT);
	addr.sin_addr.s_addr = inet_addr(_ipAddress.c_str());

	// Send the log on message
	LogOnMessage packet;
	if (SOCKET_ERROR == sendto(sSocket, (CHAR*)&packet, sizeof(packet), 0, (LPSOCKADDR)&addr, sizeof(SOCKADDR_IN)))		return false;

	// Wait for a reply or for a few seconds to pass
	if (WAIT_TIMEOUT == WSAWaitForMultipleEvents(1, &hRecvEvent, TRUE, 3000, FALSE)) return false;

	// Recieve the result
	CHAR buffer[MAX_PACKET_SIZE];
	int length;
	SOCKADDR_IN src;
	ZeroMemory(&src, sizeof(src));
	int fromlen = sizeof(SOCKADDR_IN);
	if (SOCKET_ERROR == (length = recvfrom(sSocket, buffer, sizeof(buffer), 0, (LPSOCKADDR)&src, &fromlen))) return false;

	// Connect to this address
	connect(sSocket, (LPSOCKADDR)&src, sizeof(SOCKADDR_IN));

	// Success
	return true;
}
bool NetworkClient::ProcessNetworkMessages(OtherPlayer * pPlayers, SOCKET sSocket, HANDLE hRecvEvent)
{
	if (WAIT_OBJECT_0 == WaitForSingleObject(hRecvEvent, 0))
	{
		// Buffers used to recieve data
		CHAR buffer[MAX_PACKET_SIZE];
		int size;
		SOCKADDR_IN addr;
		int fromlen = sizeof(SOCKADDR_IN);

		// Stores return codes
		bool hr;

		// Get the packet
		while (SOCKET_ERROR != (size = recvfrom(sSocket, buffer, sizeof(buffer), 0, (LPSOCKADDR)&addr, &fromlen)))
		{
			// Process information from the packet
			hr = ProcessPacket(pPlayers, buffer, size);
			if (hr == false)
				return hr;
		}
	}

	// Success
	return true;
}
bool NetworkClient::ProcessPacket(OtherPlayer * pPlayers, const CHAR * pBuffer, DWORD dwSize)
{
	MessageHeader * pMh = (MessageHeader*)pBuffer;

	switch (pMh->MsgID)
	{
	case MSG_UPDATEPLAYER:
	{
		UpdatePlayerMessage * pUpm = (UpdatePlayerMessage*)pBuffer;
		return UpdateOtherPlayer(pUpm->dwPlayerID, &pPlayers[pUpm->dwPlayerID], pUpm);
	}

	case MSG_PLAYERLOGGEDOFF:
	{
		PlayerLoggedOffMessage * pPlom = (PlayerLoggedOffMessage*)pBuffer;
		pPlayers[pPlom->dwPlayerID].bActive = FALSE;
	} break;
	}

	// Success
	return true;
}
bool NetworkClient::UpdateOtherPlayer(int _id, OtherPlayer * pPlayer, UpdatePlayerMessage * pUpm)
{
	bool initPlayer = false;
	for (size_t i = 0; i < entities->size(); i++)
	{
		if (entities->at(i)->id == _id)
		{
			initPlayer = true;
			entities->at(i)->SetPosition(pUpm->fPosition[0], pUpm->fPosition[1]);
		}
	}
	if (!initPlayer)
	{
		NetworkEntity* newNet = new NetworkEntity(_id, Sprite::Create(L"Resources\\tank.png"));
		this->SetSize(newNet->GetSize().x, newNet->GetSize().y);
		this->AddChild(newNet);
		entities->push_back(newNet);
	}

	//// If this player is inactive, activate it
	//if (!pPlayer->bActive)
	//{
	//	pPlayer->fNewTime = GetTickCount() / 1000.0f;
	//	pPlayer->vNewPos = D3DXVECTOR3(pUpm->fPosition[0], pUpm->fPosition[1], pUpm->fPosition[2]);
	//	pPlayer->vNewVel = D3DXVECTOR3(pUpm->fVelocity[0], pUpm->fVelocity[1], pUpm->fVelocity[2]);
	//	pPlayer->bActive = TRUE;
	//	pPlayer->vRenderPos = pPlayer->vNewPos;
	//	pPlayer->fRenderYaw = pPlayer->fYaw;
	//	pPlayer->entity = Sprite::Create(L"Resources\\player.png");
	//	pPlayer->entity->SetAnchorPoint(0.f, 0.f);
	//	pPlayer->entity->SetPosition(pUpm->dwPlayerID * 30, 0);
	//	pPlayer->vNewPos;
	//}
	//// Move old stuff backward
	//pPlayer->vOldPos = pPlayer->vNewPos;
	//pPlayer->vOldVel = pPlayer->vNewVel;
	//pPlayer->fOldTime = pPlayer->fNewTime;
	//// Update
	//pPlayer->fNewTime = GetTickCount() / 1000.0f;
	//pPlayer->vNewPos = D3DXVECTOR3(pUpm->fPosition[0], pUpm->fPosition[1], pUpm->fPosition[2]);
	//pPlayer->vNewVel = D3DXVECTOR3(pUpm->fVelocity[0], pUpm->fVelocity[1], pUpm->fVelocity[2]);

	// Success
	return true;
}
void NetworkClient::DisconnectFromServer(SOCKET sSocket)
{
	LogOffMessage lom;
	send(sSocket, (char*)&lom, sizeof(lom), 0);
}
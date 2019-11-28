#include "Data.h"
#include "user.h"
#include "CollisionManager.h"

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

//#pragma comment (lib,"Ws2_32.lib") // For Release only
float _totalTime = (float)clock() / CLOCKS_PER_SEC;
float _elapsedTime = 0;
clock_t _totalTicks;
float _deltaTime = 0;
float _lastTime = 0;
const float tickPerFrame = 1.0f / 60;

// Global variables used in the server program.  These variables are global because they are used
// by the server thread and initialized in the main thread.  It would be inefficient and
// duplicative to find an object-oriented workaround.
HANDLE g_hExitEvent;    // Set when the server shuts down
HANDLE g_hCommunicationThread;   // Thread processing object
WSAEVENT g_hRecvEvent;  // Event triggered when data is received
SOCKET g_sSocket;       // Socket to send and receive on
// Global variables for client management
//HANDLE g_hUserSemaphore;    // Allows only a certain number of users to process simultaneously
User g_Users[MAX_USERS];    // List of all of the users
UpdatePlayerMessage UPMs[MAX_USERS];
int UPMSizes[MAX_USERS];
std::vector<int>* dataMap;
std::vector<Object*>* dataObj;

void LoadWorld(const wchar_t * _txtPath)
{
	dataMap = new std::vector<int>();
	dataObj = new std::vector<Object*>();
	std::ifstream file(_txtPath);
	if (file.good())
	{
		std::string curData = "";
		int i = 0;
		while (file >> curData)
		{
			i++;
			int x = atoi(curData.c_str());
			dataMap->push_back(x);
		}
		file.close();
	}
	for (size_t i = 0; i < dataMap->size(); i++)
	{
		int _index = dataMap->at(i);
		if (_index == 4)
		{
			int _tileRow = (int)(i / 26);
			int _tileColumn = i % 26;

			Object* obj = new Object();
			obj->size.x = obj->size.y = 16;
			obj->position.x = _tileColumn * 16 + 8;
			obj->position.y = _tileRow * 16 + 8;
			dataObj->push_back(obj);
		}
	}
}
BOOL WaitForPackets()
{
	WSAEVENT hEvents[] = { g_hRecvEvent, g_hExitEvent };
	return WAIT_OBJECT_0 == WSAWaitForMultipleEvents(2, hEvents, FALSE, INFINITE, FALSE);
}
int RecvPacket(char * pBuffer, int length, SOCKADDR_IN * pAddress)
{
	// CHEAP TRICK!!  Force the receive event to reset.  Find out why this doesn't usually work!!
	ResetEvent(g_hRecvEvent);

	// Size of the address structure
	int iFromLen = sizeof(SOCKADDR_IN);

	// Perform the operation
	return recvfrom(g_sSocket, pBuffer, length, 0, (LPSOCKADDR)pAddress, &iFromLen);
}
int SendPacket(const LPSOCKADDR_IN pAddress, const CHAR * pBuffer, int length)
{
	return sendto(g_sSocket, pBuffer, length, 0, (LPSOCKADDR)&pAddress, sizeof(SOCKADDR_IN));
}
HRESULT ProcessUserPacket(User * pUser, const CHAR * pBuffer, DWORD dwSize)
{
	// Get the message header so we can determine the type of the packet
	MessageHeader * mh = (MessageHeader*)pBuffer;
	// Process the message
	switch (mh->MsgID)
	{
	case MSG_LOGOFF:
	{
		// Get this user's ID number
		DWORD dwId = pUser->GetId();

		// Build a disconnect message
		PlayerLoggedOffMessage packet;
		packet.dwPlayerID = dwId;

		// Tell all of the other users that this player disconnected
		for (DWORD i = 0; i < MAX_USERS; ++i)
		{
			// Send to all connected users except the source
			if (/*(i != dwId) &&*/ (g_Users[i].IsConnected()))
				g_Users[i].SendPacket((CHAR*)&packet, sizeof(packet));
		}
		// Disconnect the user
		pUser->Disconnect();

		// Return a success message, but the recieve loop can't continue
		return S_FALSE;
	}
	case MSG_UPDATEPLAYER:
	{
		// Get this user's ID number
		DWORD dwId = pUser->GetId();

		// Set the message's ID component
		UpdatePlayerMessage upm;
		memcpy(&upm, pBuffer, dwSize);
		upm.dwPlayerID = dwId;
		/*pUser->velocity.x = upm.fVelocity[0];
		pUser->velocity.y = upm.fVelocity[1];
		pUser->position.x += pUser->velocity.x;
		pUser->position.y += pUser->velocity.y;
		upm.fPosition[0] = pUser->position.x;
		upm.fPosition[1] = pUser->position.y;*/

		memcpy(&UPMs[upm.dwPlayerID], &upm, dwSize);
		UPMSizes[upm.dwPlayerID] = dwSize;
		//// Re-broadcast this message
		//for (DWORD i = 0; i < MAX_USERS; ++i)
		//{
		//	// Send to all connected users except the source
		//	if (/*(i != dwId) && */(g_Users[i].IsConnected())) g_Users[i].SendPacket((CHAR*)&UPMs[upm.dwPlayerID], dwSize);
		//}
	}
	break;
	default:
	{
		// This message type couldn't be processed
		return E_FAIL;
	}
	}
	// Success
	return S_OK;
}
DWORD WINAPI UserProcessor(LPVOID pParam)
{
	// Get a pointer to the user structure
	User * pUser = (User*)pParam;
	// Send a message to the user telling them that they have successfully logged on
	// Build a packet
	ConfirmLogOnMessage packet;
	// Send the packet
	int a = pUser->SendPacket((CHAR*)&packet, sizeof(packet));

	//// Activity flag
	//BOOL bUserActive = TRUE;
	//// Enter the processing loop
	//while (bUserActive)
	//{
	//	// Wait for something to happen or for 5 seconds to expire
	//	switch (pUser->WaitForPackets(5000))
	//	{
	//		// When a packet is recieved, process it
	//	case EWR_RECVPACKET:
	//	{
	//		OutputDebugString(".");
	//		// Buffers used to recieve data
	//		CHAR buffer[MAX_PACKET_SIZE];
	//		int size;
	//		// Get the packet
	//		while (SOCKET_ERROR != (size = pUser->RecvPacket(buffer, sizeof(buffer))))
	//		{
	//			// Process information from the packet
	//			if (S_FALSE == ProcessUserPacket(pUser, buffer, size))
	//				break;
	//		}
	//	} break;
	//	// If the user hasn't sent a message in a while, determine if the program wants to
	//	// exit.  If it does, break the loop.
	//	case EWR_TIMEOUT:
	//	{
	//		// Output message
	//		printf("\n[%u] lagged out", pUser->GetId());
	//		// If the global termination event is set, exit
	//		if (WAIT_OBJECT_0 == WaitForSingleObject(g_hExitEvent, 0))
	//			break;
	//		// Log this player out
	//		//pUser->Disconnect();
	//	} break;
	//	// This user has been forcibly disconnected by the server, usually by shutting down.
	//	case EWR_DISCONNECT:
	//	{
	//		// Output message
	//		printf("\n[%u] disconnected", pUser->GetId());
	//		// No longer active
	//		bUserActive = FALSE;
	//	} break;
	//	}
	//}
	//pUser->ThreadFinished();

	// Success
	return S_OK;
}
HRESULT LogOnNewPlayer(const SOCKADDR_IN * pAddr)
{
	// Look through the list
	for (int i = 0; i < MAX_USERS; ++i)
	{
		// If this user structure isn't already handling someone, set up the connection
		if (g_Users[i].IsConnected() == FALSE)
		{
			printf("\nLogged on user %i", i);
			return g_Users[i].Connect(pAddr, UserProcessor);
		}
	}
	// Couldn't find a place for the player
	return E_FAIL;
}
DWORD WINAPI CommunicationThread(LPVOID pParam)
{
	// Wait for things to happen
	while (WaitForPackets())
	{
		// Holds incoming data values
		char buffer[MAX_PACKET_SIZE];
		SOCKADDR_IN address;
		int len;

		ZeroMemory(buffer, sizeof(buffer));

		// Get data until the operation would block
		while (SOCKET_ERROR != (len = RecvPacket(buffer, sizeof(buffer), &address)))
		{
			MessageHeader * pMh = (MessageHeader*)buffer;
			if (pMh->MsgID == MSG_LOGON) LogOnNewPlayer(&address);
		}
	}

	// Success
	return S_OK;
}
void Update(float _deltaTime)
{
	Vector2 normalVector;
	normalVector.x = normalVector.y = 0;
	for (int i = 0; i < MAX_USERS; ++i)
	{
		if (g_Users[i].IsConnected())
		{
			// Wait for something to happen or for 5 seconds to expire
			switch (g_Users[i].WaitForPackets(5000))
			{
				// When a packet is recieved, process it
			case EWR_RECVPACKET:
			{
				OutputDebugString(".");
				// Buffers used to recieve data
				CHAR buffer[MAX_PACKET_SIZE];
				int size;
				// Get the packet
				while (SOCKET_ERROR != (size = g_Users[i].RecvPacket(buffer, sizeof(buffer))))
				{
					// Process information from the packet
					if (S_FALSE == ProcessUserPacket(&g_Users[i], buffer, size)) break;
				}
			} break;
			// If the user hasn't sent a message in a while, determine if the program wants to
			// exit.  If it does, break the loop.
			case EWR_TIMEOUT:
			{
				// Output message
				printf("\n[%u] lagged out", g_Users[i].GetId());
				// If the global termination event is set, exit
				if (WAIT_OBJECT_0 == WaitForSingleObject(g_hExitEvent, 0))
					break;
				// Log this player out
				//pUser->Disconnect();
			} break;
			// This user has been forcibly disconnected by the server, usually by shutting down.
			case EWR_DISCONNECT:
			{
				// Output message
				printf("\n[%u] disconnected", g_Users[i].GetId());
			} break;
			}
			g_Users[i].velocity.x = UPMs[i].fVelocity[0];
			g_Users[i].velocity.y = UPMs[i].fVelocity[1];
			g_Users[i].size.x = UPMs[i].fSize[0];
			g_Users[i].size.y = UPMs[i].fSize[1];
			//Physics
			for (DWORD j = 0; j < MAX_USERS; j++)
			{
				if (i != j && g_Users[j].IsConnected())
				{
					float normalX, normalY;
					int checkAABB = CollisionManager::GetInstance()->CheckSweptAABB(&g_Users[i], &g_Users[j], normalX, normalY);
					if (checkAABB < 1)
					{
						normalVector.x = normalX;
						normalVector.y = normalY;
					}
					else if (checkAABB == 1)
					{
						if (normalX > 0 || normalY > 0)
						{
							normalVector.x = normalX;
							normalVector.y = normalY;
						}
					}
				}
			}
			/*for (size_t j = 0; j < dataObj->size(); j++)
			{
				float normalX, normalY;
				int checkAABB = CollisionManager::GetInstance()->CheckSweptAABB(&g_Users[i], dataObj->at(j), normalX, normalY);
				if (checkAABB < 1)
				{
					normalVector.x = normalX;
					normalVector.y = normalY;
				}
				else if (checkAABB == 1)
				{
					if (normalX > 0 || normalY > 0)
					{
						normalVector.x = normalX;
						normalVector.y = normalY;
					}
				}
			}*/
			// Apply Physics to players
			if (g_Users[i].velocity.x != 0 || g_Users[i].velocity.y != 0)
			{
				g_Users[i].position.x += g_Users[i].velocity.x + normalVector.x;
				g_Users[i].position.y += g_Users[i].velocity.y + normalVector.y;
			}
			else
			{
				g_Users[i].position.x += g_Users[i].velocity.x;
				g_Users[i].position.y += g_Users[i].velocity.y;
			}

			UPMs[i].fPosition[0] = g_Users[i].position.x;
			UPMs[i].fPosition[1] = g_Users[i].position.y;
			//Broadcast to all players
			for (DWORD k = 0; k < MAX_USERS; ++k) if ((g_Users[k].IsConnected())) g_Users[k].SendPacket((CHAR*)&UPMs[i], UPMSizes[i]);
		}
	}
}
int main()
{
	// Start up Winsock
	{
		// Stores information about Winsock
		WSADATA wsaData;

		// Call the DLL initialization function
		if (SOCKET_ERROR == WSAStartup(WINSOCK_VERSION, &wsaData) ||
			wsaData.wVersion != WINSOCK_VERSION)
			return -1;
	}
	// Tell the user the local IP address and host information
	{
		CHAR strHostName[80];
		if (SOCKET_ERROR == gethostname(strHostName, sizeof(strHostName)))
			return -1;

		// Get the entry in the host table
		LPHOSTENT pHostEnt = gethostbyname(strHostName);
		if (!pHostEnt)
			return -1;

		// Get the primary address from the host entry table
		in_addr addr;
		memcpy(&addr, pHostEnt->h_addr_list[0], sizeof(addr));

		// Tell the user information about the server
		printf("Server '%s' is operating at %s\n", strHostName, inet_ntoa(addr));
	}
	// Set up server data
	{
		// Initialize the exit event
		g_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		// Set up the main socket to accept and send UDP packets
		g_sSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		// Generate the address to bind to
		SOCKADDR_IN addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(SERVER_COMM_PORT);
		addr.sin_addr.S_un.S_addr = INADDR_ANY;

		// Bind it to a port
		if (SOCKET_ERROR == bind(g_sSocket, (LPSOCKADDR)&addr, sizeof(SOCKADDR_IN)))
			return -1;

		// Create the event
		g_hRecvEvent = WSACreateEvent();
		if (SOCKET_ERROR == WSAEventSelect(g_sSocket, g_hRecvEvent, FD_READ))
			return -1;

		// Create the processor thread
		if (NULL == (g_hCommunicationThread = CreateThread(NULL, 0, CommunicationThread, NULL, 0, NULL)))
			return -1;
	}
	// Initialize all of the clients
	{
		WORD wBasePort = SERVER_COMM_PORT + 1;
		for (int i = 0; i < MAX_USERS; ++i)
			while (!g_Users[i].Create(i, wBasePort + i)) { wBasePort++; }
	}
	// Tell the user that the server has been initialized
	printf("Server successfully initialized.  Press any key to exit...");
	LoadWorld(L"Resources\\Level1.txt");

	while (true)
	{
		float curTime = (float)clock() / CLOCKS_PER_SEC;
		_elapsedTime = curTime - _totalTime;
		_totalTime = curTime;
		_deltaTime = _totalTime - _lastTime;
		if (_deltaTime >= tickPerFrame)
		{
			_lastTime += tickPerFrame;
			// Update
			Update(_deltaTime);
		}
		else Sleep((tickPerFrame - _deltaTime) * 1000.0f);
	}

	 //Repeat until the termination event is set
	while (WAIT_OBJECT_0 != WaitForSingleObject(g_hExitEvent, 0))
	{
		// Wait for 10 seconds to exit
		_getch();
		SetEvent(g_hExitEvent);
	}
	// Wait for the main thread to terminate
	WaitForSingleObject(g_hCommunicationThread, INFINITE);
	// Shut down all of the clients
	{
		for (int i = 0; i < MAX_USERS; ++i)
		{
			g_Users[i].ThreadFinished();
			g_Users[i].Destroy();
		}
	}
	// Delete the receive event
	WSACloseEvent(g_hRecvEvent);
	// Close the socket
	closesocket(g_sSocket);
	// Shut down Winsock
	WSACleanup();
	// Success
	return 0;
}
#include "Data.h"
#include "user.h"
#include "CollisionManager.h"

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <deque>

//#pragma comment (lib,"Ws2_32.lib") // For Release only
float _totalTime = (float)clock() / CLOCKS_PER_SEC;
float _elapsedTime = 0;
clock_t _totalTicks;
float _deltaTime = 0;
float _lastTime = 0;
int FPS = 60;
const float tickPerFrame = 1.0f / FPS;

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
UpdateMessage UPMs[MAX_USERS];
Messenger msgs[MAX_USERS];
int UPMSizes[MAX_USERS];
std::vector<int>* dataMap;
std::vector<Object*>* staticObjs;
std::vector<Object*>* dynamicObjs;
std::deque<std::vector<Object*>*>* lastFrameObjectsInfo;
std::vector<Object> lateFrameObjects[MAX_LATE_FRAMES];

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
	Messenger* msg = (Messenger*)pBuffer;
	switch (msg->action)
	{
	case Action::UpdateObj:
	{
		pUser->velocity.x = msg->velocity[0];
		pUser->velocity.y = msg->velocity[1];
		pUser->size.x = msg->size[0];
		pUser->size.y = msg->size[1];
		pUser->tick = msg->tick;
		pUser->inputSequenceNumber = msg->inputSequenceNumber;
		int lateFramesCount = 0;
		//std::vector<Object>* thisFrameUserData = new std::vector<Object>();
		//if (pUser->IsConnected())
		//{
		//	lateFramesCount = (int)(((int)GetTickCount() - msg->tick) / 16.67f);
		//	if (lateFramesCount > 0)
		//	{
		//		for (int k = MAX_USERS - lateFramesCount - 1; k < lastFrameObjectsInfo->size(); ++k);
		//		//lastFrameObjectsInfo->at(k)->push_back(*pUser);
		//		//lateFrameObjects[lateFramesCount - 1].push_back(g_Users[i]);
		//	}
		//	thisFrameUserData->push_back(*pUser);
		//	if (lastFrameObjectsInfo->size() > MAX_LATE_FRAMES - 1) lastFrameObjectsInfo->pop_front();
		//	lastFrameObjectsInfo->push_back(thisFrameUserData);
		//}
	}
	break;
	case Action::Create:
	{
		//Object* bullet = new Object();
		int id = msg->id;
		for (DWORD k = 0; k < MAX_USERS; ++k) if ((g_Users[k].IsConnected())) g_Users[k].SendPacket((CHAR*)msg, sizeof(*msg));
	}
	break;
	case Action::LogOff:
	{
		// Disconnect the user
		pUser->Disconnect();
		//pUser->ThreadFinished();
		for (int i = 0; i < dynamicObjs->size(); i++)
		{
			if (dynamicObjs->at(i)->objType == ObjectType::Player)
			{
				if (dynamicObjs->at(i)->id == msg->id) dynamicObjs->erase(dynamicObjs->begin() + i);
			}
		}
		// Tell all of the other users that this player disconnected
		for (DWORD i = 0; i < MAX_USERS; ++i)
		{
			// Send to all connected users except the source
			if (/*i != msg->id && */g_Users[i].IsConnected()) g_Users[i].SendPacket((CHAR*)msg, sizeof(*msg));
		}
	}
	break;
	default:
		break;
	}

	return S_OK;
}
DWORD WINAPI UserProcessor(LPVOID pParam)
{
	// Get a pointer to the user structure
	User * pUser = (User*)pParam;
	// Send a message to the user telling them that they have successfully logged on
	// Build a packet

	LogOnMessage packet;
	//packet.Header.MsgID = Message::MSG_LOGON;
	packet.id = pUser->GetId();
	// Send the packet
	int a = pUser->SendPacket((CHAR*)&packet, sizeof(packet));
	//for (DWORD k = 0; k < MAX_USERS; ++k) if (k != packet.id && g_Users[k].IsConnected()) g_Users[k].SendPacket((CHAR*)&packet, sizeof(packet));

	// Activity flag
	BOOL bUserActive = TRUE;

	// Enter the processing loop
	while (bUserActive)
	{
		// Wait for something to happen or for 5 seconds to expire
		switch (pUser->WaitForPackets(5000))
		{
		case EWR_RECVPACKET:
		{
			OutputDebugString(".");
			// Buffers used to recieve data
			CHAR buffer[MAX_PACKET_SIZE];
			int size;
			// Get the packet
			while (SOCKET_ERROR != (size = pUser->RecvPacket(buffer, sizeof(buffer))))
			{
				// Process information from the packet
				if (S_FALSE == ProcessUserPacket(pUser, buffer, size)) break;
			}
		} break;
		case EWR_TIMEOUT:
		{
			// Output message
			printf("\n[%u] lagged out", pUser->GetId());

			// If the global termination event is set, exit
			if (WAIT_OBJECT_0 == WaitForSingleObject(g_hExitEvent, 0)) break;

			// Log this player out
			//pUser->Disconnect();

		} break;
		case EWR_DISCONNECT:
		{
			// Output message
			printf("\n[%u] disconnected", pUser->GetId());

			// No longer active
			bUserActive = FALSE;

		} break;
		}
	}

	pUser->ThreadFinished();

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
			g_Users[i].objType = ObjectType::Player;
			g_Users[i].id = i;
			dynamicObjs->push_back(&g_Users[i]);
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
			MessageHeader* pMh = (MessageHeader*)buffer;
			if (pMh->MsgID == Message::MSG_LOGON) LogOnNewPlayer(&address);
		}
	}
	// Success
	return S_OK;
}
void LoadWorld(const wchar_t * _txtPath)
{
	dataMap = new std::vector<int>();
	staticObjs = new std::vector<Object*>();
	dynamicObjs = new std::vector<Object*>();
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
	for (int i = 0; i < dataMap->size(); i++)
	{
		int _index = dataMap->at(i);
		if (_index != 0)
		{
			int _tileRow = (int)(i / 26);
			int _tileColumn = i % 26;
			Object* obj = new Object();
			obj->velocity.x = obj->velocity.y = 0;
			obj->size.x = obj->size.y = 16;
			obj->position.x = _tileColumn * 16 + obj->size.x / 2;
			obj->position.y = _tileRow * 16 + obj->size.x / 2;
			obj->id = i;
			if (_index == 4) obj->tag = "Brick";
			else if (_index == 13) obj->tag = "Wall";
			else if (_index == 49 || _index == 50 || _index == 53 || _index == 54) obj->tag = "Eagle";
			staticObjs->push_back(obj);
		}
	}
}
void Update(float _deltaTime)
{
	//std::vector<Object*>* thisFrameUserData = new std::vector<Object*>();
	Vector2 normalVector;
	normalVector.x = normalVector.y = 0;
	//Physics
	for (int i = 0; i < dynamicObjs->size(); ++i)
	{
		//thisFrameUserData->push_back(dynamicObjs->at(i));
		if (dynamicObjs->at(i)->objType == ObjectType::Player)
		{
			for (int j = 0; j < dynamicObjs->size(); ++j)
			{
				if (dynamicObjs->at(j)->objType == ObjectType::Player && j == i);
				else
				{
					float normalX, normalY;
					int checkAABB = CollisionManager::GetInstance()->CheckSweptAABB(dynamicObjs->at(i), dynamicObjs->at(j), normalX, normalY);
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
			for (int j = 0; j < staticObjs->size(); j++)
			{
				float normalX, normalY;
				int checkAABB = CollisionManager::GetInstance()->CheckSweptAABB(dynamicObjs->at(i), staticObjs->at(j), normalX, normalY);
				if (checkAABB < 1)
				{
					Object*a = staticObjs->at(j);
					normalVector.x = normalX;
					normalVector.y = normalY;
				}
				else if (checkAABB == 1)
				{
					if (normalX > 0 || normalY > 0)
					{
						Object*a = staticObjs->at(j);
						normalVector.x = normalX;
						normalVector.y = normalY;
					}
				}
			}
			// Apply Velocity to players after physics
			//for (int k = 0; k < MAX_USERS; ++k) if (g_Users[k].id == dynamicObjs->at(i)->id) g_Users[k].OnCollisionEnter(normalVector);
			dynamicObjs->at(i)->OnCollisionEnter(normalVector);
		}
	}

	//if (lastFrameObjectsInfo->size() > MAX_LATE_FRAMES - 1) lastFrameObjectsInfo->pop_front();
	//lastFrameObjectsInfo->push_back(thisFrameUserData);

	//Create the sending Message
	for (int i = 0; i < MAX_USERS; ++i)
	{
		if (g_Users[i].IsConnected())
		{
			msgs[i].id = g_Users[i].GetId();
			msgs[i].action = Action::UpdateObj;
			msgs[i].type = ObjectType::Player;
			msgs[i].inputSequenceNumber = g_Users[i].inputSequenceNumber;
			msgs[i].tick = g_Users[i].tick;
			msgs[i].position[0] = g_Users[i].position.x;
			msgs[i].position[1] = g_Users[i].position.y;
			msgs[i].rotation[0] = g_Users[i].floatRotation *3.14f / 180;
			msgs[i].size[0] = g_Users[i].size.x;
			msgs[i].size[1] = g_Users[i].size.y;
		}
	}
	//Broadcast to all players
	for (int i = 0; i < MAX_USERS; ++i)
	{
		for (DWORD k = 0; k < MAX_USERS; ++k)
		{
			if (g_Users[i].IsConnected() && g_Users[k].IsConnected())
			{
				g_Users[k].SendPacket((CHAR*)&msgs[i], sizeof(msgs[i]));
			}
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
	//Initiate game world
	LoadWorld(L"Resources\\Level1.txt");
	lastFrameObjectsInfo = new std::deque<std::vector<Object*>*>();
	int index = 0;
	while (true)
	{
		float curTime = (float)clock() / CLOCKS_PER_SEC;
		_elapsedTime = curTime - _totalTime;
		_totalTime = curTime;
		_deltaTime = _totalTime - _lastTime;
		if (_deltaTime >= tickPerFrame)
		{
			index = 0;
			_lastTime += tickPerFrame;
			//// Update

			////Late Frame Physics
			//if (index < MAX_LATE_FRAMES && lastFrameObjectsInfo->size()>0)
			//{
			//	if (lateFrameObjects[index].size() > 0)
			//	{
			//		for (int o = 0; o < lateFrameObjects[index].size(); o++)
			//		{
			//			for (int p = 0; p < lastFrameObjectsInfo->at(index)->size(); p++)
			//			{
			//				//AABB
			//			}
			//		}
			//	}
			//	index++;
			//}

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
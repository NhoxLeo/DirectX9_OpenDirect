#include "NetworkClient.h"
#include "Tilemap.h"

NetworkClient::NetworkClient()
{
	ipAddress = "127.0.0.1";
	if (InitializeWinsock(&sSocket, &hRecvEvent) && ConnectToServer(ipAddress, sSocket, hRecvEvent));
	else cout << "failed";
	entities = new vector<NetworkEntity*>();

	for (size_t i = 0; i < MAX_USERS; i++) players[i] = new NetworkEntity(i, Sprite::Create(L"Resources\\tank.png"));
}
NetworkClient::NetworkClient(std::string _ipAddress)
{
	entities = new vector<NetworkEntity*>();
	ipAddress = _ipAddress;
	if (InitializeWinsock(&sSocket, &hRecvEvent) && ConnectToServer(_ipAddress, sSocket, hRecvEvent));
	else cout << "failed";
	for (size_t i = 0; i < MAX_USERS; i++) players[i] = new NetworkEntity(i, Sprite::Create(L"Resources\\tank.png"));
	allThreadRunning = true;

	fireRate = 0.5f;
	fireTime = 0;

	clientThread = std::thread(&NetworkClient::NetworkThread, this, sSocket, hRecvEvent);
}
NetworkClient::~NetworkClient()
{
	allThreadRunning = false;
	DisconnectFromServer(sSocket);
	clientThread.join();
	Object::~Object();
}
void NetworkClient::Update(float deltaTime)
{
	int x = 0, y = 0;
	if (Input::GetInstance()->GetKeyState('W') == KeyState::Pressed) y = -1;
	if (Input::GetInstance()->GetKeyState('A') == KeyState::Pressed) x = -1;
	if (Input::GetInstance()->GetKeyState('S') == KeyState::Pressed) y = 1;
	if (Input::GetInstance()->GetKeyState('D') == KeyState::Pressed) x = 1;

	Object::Update(deltaTime);
	//for (size_t i = 0; i < entities->size(); i++) entities->at(i)->Update(deltaTime);
	//for (size_t i = 0; i < MAX_USERS; i++) if (players[i]->isActive) players[i]->Update(deltaTime);
	//players[id]->SetPosition(players[id]->GetPosition() + Vector2(x, y));

	Messenger msg;
	msg.action = Action::UpdateObj;
	msg.id = id;
	msg.tick = GetTickCount();
	msg.velocity[0] = x;
	msg.velocity[1] = y;
	if (msg.velocity[0] != 0) msg.rotation[0] = (msg.velocity[0] == 1) ? 90 : 270;
	else if (msg.velocity[1] != 0) msg.rotation[0] = (msg.velocity[1] == 1) ? 180 : 0;
	msg.rotation[0] = msg.rotation[0] * 3.14f / 180;
	msg.size[0] = this->GetSize().x;
	msg.size[1] = this->GetSize().y;
	msg.inputSequenceNumber = input_sequence_number++;
	pendingInputs.push_back(msg);
	int sendInt = send(sSocket, (CHAR*)&msg, sizeof(msg), 0);

	if (Input::GetInstance()->GetKeyState('Q') == KeyState::Pressed && fireTime > fireRate)
	{
		fireTime = 0;
		Messenger shootMessage;
		shootMessage.type = ObjectType::Bullet;
		shootMessage.action = Action::Create;
		shootMessage.id = id;
		shootMessage.tick = GetTickCount();
		for (size_t i = 0; i < entities->size(); i++)
		{
			if (entities->at(i)->id == id)
			{
				shootMessage.position[0] = entities->at(i)->GetPosition().x;
				shootMessage.position[1] = entities->at(i)->GetPosition().y;
			}
		}
		shootMessage.velocity[0] = 1;
		shootMessage.velocity[1] = 0;
		int a = sizeof(shootMessage);
		send(sSocket, (CHAR*)&shootMessage, sizeof(shootMessage), 0);
	}
	if (fireTime <= fireRate) fireTime += deltaTime;

	//// Update the messages from the server
	//ProcessNetworkMessages(sSocket, hRecvEvent);
	////cout << pendingInputs.size() << "\n";

}
void NetworkClient::Render()
{
	Object::Render();
	for (size_t i = 0; i < entities->size(); i++) entities->at(i)->Render();
	//for (size_t i = 0; i < MAX_USERS; i++) if (players[i]->isActive) players[i]->Render();
}

bool NetworkClient::InitializeWinsock(SOCKET * pSocket, HANDLE * pRecvEvent)
{
	// Start up Winsock
	{
		// Stores information about Winsock
		WSADATA wsaData;
		// Call the DLL initialization function
		if (SOCKET_ERROR == WSAStartup(WINSOCK_VERSION, &wsaData) || wsaData.wVersion != WINSOCK_VERSION)
		{
			cout << "cannot init winsock";
			return false;
		}
	}

	// Set up the main socket 
	SOCKET sSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// Create the event
	HANDLE hRecvEvent = WSACreateEvent();
	if (SOCKET_ERROR == WSAEventSelect(sSocket, hRecvEvent, FD_READ))
	{
		cout << "cannot connect";
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
	packet.Header.MsgID = Message::MSG_LOGON;
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

	MessageHeader* pMh = (MessageHeader*)buffer;
	if (pMh->MsgID == Message::MSG_LOGON)
	{
		LogOnMessage * msg = (LogOnMessage*)buffer;
		id = msg->id;

		NetworkEntity* newNet = new NetworkEntity(msg->id, Sprite::Create(L"Resources\\tank.png"));
		this->SetSize(newNet->GetSize().x, newNet->GetSize().y);
		entities->push_back(newNet);
	}
	// Connect to this address
	connect(sSocket, (LPSOCKADDR)&src, sizeof(SOCKADDR_IN));

	// Success
	return true;
}
bool NetworkClient::ProcessNetworkMessages(SOCKET sSocket, HANDLE hRecvEvent)
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
			hr = ProcessPacket(buffer, size);
			if (hr == false) return hr;
		}
	}
	// Success
	return true;
}
bool NetworkClient::ProcessPacket(const CHAR * pBuffer, DWORD dwSize)
{
	Messenger* msg = (Messenger*)pBuffer;
	switch (msg->action)
	{
	case Action::UpdateObj:
	{
		if (msg->type == ObjectType::Player)
		{
			bool initPlayer = false;
			for (size_t i = 0; i < entities->size(); i++)
			{
				if (msg->id == entities->at(i)->id)
				{
					initPlayer = true;
					entities->at(i)->SetPosition(msg->position[0], msg->position[1]);
					entities->at(i)->SetRotation(msg->rotation[0]);

					if (msg->id == id)
					{
						int j = 0;
						while (j < pendingInputs.size())
						{
							if (pendingInputs.at(j).inputSequenceNumber <= msg->inputSequenceNumber)
							{
								pendingInputs.erase(pendingInputs.begin() + j);
								entities->at(i)->SetPosition(msg->position[0], msg->position[1]);
								entities->at(i)->SetRotation(msg->rotation[0]);
							}
							else
							{
								if (pendingInputs.at(j).velocity[0] + pendingInputs.at(j).velocity[1] != 0)
								{
									entities->at(i)->SetPosition(entities->at(i)->GetPosition() + Vector2(pendingInputs.at(j).velocity[0], pendingInputs.at(j).velocity[1]));
									entities->at(i)->SetRotation(pendingInputs.at(j).rotation[0]);
								}
								j++;
							}
						}
					}
				}
			}
			if (!initPlayer && msg->id != id)
			{
				NetworkEntity* newNet = new NetworkEntity(msg->id, Sprite::Create(L"Resources\\tank.png"));
				this->SetSize(newNet->GetSize().x, newNet->GetSize().y);
				entities->push_back(newNet);
			}
		}
	}
	break;
	case Action::Create:
	{
		if (msg->type == ObjectType::Bullet)
		{
			Sprite* bullet = Sprite::Create(L"Resources\\Tank\\testbullet.bmp");
			bullet->SetTag("Bullet");
			bullet->AddComponent<Rigidbody>(new Rigidbody());
			bullet->SetPosition(msg->position[0], msg->position[1]);
			bullet->GetComponent<Rigidbody>()->SetVelocity(msg->velocity[0], msg->velocity[1]);
			Director::GetInstance()->GetScene()->AddChild(bullet);
		}
	}
	break;
	case Action::LogOff:
	{
		for (size_t i = 0; i < entities->size(); i++)
		{
			if (entities->at(i)->id == msg->id)
				entities->erase(entities->begin() + i);
		}
		//if (players[msg->id]->isActive) players[msg->id]->isActive = false;
	}
	break;
	case Action::Destroy:
	{
		std::vector<Object*> objList = Director::GetInstance()->GetScene()->GetComponent<Tilemap>()->GetChildren();
		for (size_t i = 0; i < objList.size(); i++)
		{
			Object* obj = objList.at(i);
			if (obj->GetPosition().x == msg->position[0] && obj->GetPosition().y == msg->position[1])
			{
				int a = 1;
				//Director::GetInstance()->GetScene()->RemoveChild(obj);
				Director::GetInstance()->GetScene()->GetComponent<Tilemap>()->RemoveChild(obj);
				for (size_t i = 0; i < Director::GetInstance()->GetScene()->GetChildren().size(); i++)
				{
					Object* obj = Director::GetInstance()->GetScene()->GetChildren().at(i);
					if (obj->GetTag() == "Bullet")  Director::GetInstance()->GetScene()->RemoveChild(obj);
				}
			}
		}
	}
	break;
	default:
		break;
	}
	// Success
	return true;
}
bool NetworkClient::UpdateOtherPlayer(int _id, UpdateMessage * pUpm)
{
	bool initPlayer = false;
	for (size_t i = 0; i < entities->size(); i++)
	{
		if (entities->at(i)->id == _id)
		{
			initPlayer = true;
			/*entities->at(i)->SetPosition(pUpm->fPosition[0], pUpm->fPosition[1]);
			entities->at(i)->SetRotation(pUpm->fRotation[0]);*/
		}
	}
	if (!initPlayer)
	{
		NetworkEntity* newNet = new NetworkEntity(_id, Sprite::Create(L"Resources\\tank.png"));
		this->SetSize(newNet->GetSize().x, newNet->GetSize().y);
		this->AddChild(newNet);
		entities->push_back(newNet);
	}
	// Success
	return true;
}
void NetworkClient::DisconnectFromServer(SOCKET sSocket)
{
	Messenger logoffMessage;
	logoffMessage.id = id;
	logoffMessage.action = Action::LogOff;
	logoffMessage.type = ObjectType::Player;
	send(sSocket, (char*)&logoffMessage, sizeof(logoffMessage), 0);
}

void NetworkClient::NetworkThread(SOCKET _socket, HANDLE _recvEvent)
{
	while (allThreadRunning)
	{
		// Update the messages from the server
		ProcessNetworkMessages(sSocket, hRecvEvent);
	}
}

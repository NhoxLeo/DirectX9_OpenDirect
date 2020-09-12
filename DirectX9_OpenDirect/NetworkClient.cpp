#include "NetworkClient.h"
#include "Tilemap.h"
#include "MemoryBitStream.h"
#include <sstream>

NetworkClient::NetworkClient()
{
	ipAddress = "127.0.0.1";
	if (InitializeWinsock(&sSocket, &hRecvEvent) && ConnectToServer(ipAddress, sSocket, hRecvEvent));
	else cout << "failed";
	entities = new vector<NetworkEntity*>();

}
NetworkClient::NetworkClient(std::string _ipAddress)
{
	isAlive = true;
	entities = new vector<NetworkEntity*>();
	bullets = new vector<Object*>();
	ipAddress = _ipAddress;
	if (InitializeWinsock(&sSocket, &hRecvEvent) && ConnectToServer(ipAddress, sSocket, hRecvEvent));
	else cout << "failed";
	allThreadRunning = true;

	fireRate = 0.5f;
	fireTime = 0;

	//clientThread = std::thread(&NetworkClient::NetworkThread, this, sSocket, hRecvEvent);


	//D3DXCreateFont(Renderer::GetInstance()->GetDevice()
	//	, 416, 416
	//	, FW_NORMAL, 1
	//	, false, DEFAULT_CHARSET
	//	, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, (LPCWSTR) "Arial", &my_font);
}
NetworkClient::~NetworkClient()
{
	allThreadRunning = false;
	DisconnectFromServer(sSocket);
	//clientThread.join();
	Object::~Object();
}
void NetworkClient::Update(float deltaTime)
{
	int x = 0, y = 0;
	if (isAlive)
	{
		if (Input::GetInstance()->GetKeyState('W') == KeyState::Pressed) y = -1;
		if (Input::GetInstance()->GetKeyState('A') == KeyState::Pressed) x = -1;
		if (Input::GetInstance()->GetKeyState('S') == KeyState::Pressed) y = 1;
		if (Input::GetInstance()->GetKeyState('D') == KeyState::Pressed) x = 1;
	}

	Object::Update(deltaTime);

	Messenger msg;
	msg.room = 1;
	msg.action = Action::UpdateObj;
	msg.id = id;
	msg.tick = GetTickCount();
	msg.velocity[0] = x;
	msg.velocity[1] = y;
	if (msg.velocity[0] != 0) msg.rotation[0] = (msg.velocity[0] == 1) ? 90 : 270;
	else if (msg.velocity[1] != 0) msg.rotation[0] = (msg.velocity[1] == 1) ? 180 : 0;
	msg.rotation[0] *= 3.14f / 180;
	msg.size[0] = this->GetSize().x;
	msg.size[1] = this->GetSize().y;
	msg.inputSequenceNumber = (x + y != 0) ? input_sequence_number++ : 0;
	pendingInputs.push_back(msg);

	int sendInt = send(sSocket, (CHAR*)&msg, sizeof(msg), 0);
	//int sendInt = send(sSocket, oss.str().c_str(), sizeof(oss.str().c_str()), 0);

	if (isAlive && Input::GetInstance()->GetKeyState('P') == KeyState::Pressed && fireTime > fireRate)
	{
		fireTime = 0;
		Messenger shootMessage;
		shootMessage.type = ObjectType::Bullet;
		shootMessage.action = Action::Create;
		shootMessage.id = id;
		shootMessage.tick = GetTickCount();
		shootMessage.position[0] = thisEntity->GetPosition().x;
		shootMessage.position[1] = thisEntity->GetPosition().y;
		int tempRot = thisEntity->GetRotation() * 180 / 3.14f;
		if ((int)tempRot == 90 || (int)tempRot == 270)shootMessage.velocity[0] = ((int)tempRot == 90) ? 3 : -3;
		else shootMessage.velocity[1] = ((int)tempRot == 180) ? 3 : -3;
		send(sSocket, (CHAR*)&shootMessage, sizeof(shootMessage), 0);

		Sprite* bullet = Sprite::Create(L"Resources\\Tank\\bullet.png");
		bullet->SetTag("Bullet");
		bullet->AddComponent<Rigidbody>(new Rigidbody());
		bullet->SetPosition(shootMessage.position[0], shootMessage.position[1]);
		bullet->GetComponent<Rigidbody>()->SetVelocity(shootMessage.velocity[0], shootMessage.velocity[1]);
		bullet->SetSize(8, 8);
		//Director::GetInstance()->GetScene()->AddChild(bullet);
		AddChild(bullet);
	}
	if (fireTime <= fireRate) fireTime += deltaTime;


	//Local Physics
	if (x + y != 0) thisEntity->GetComponent<Rigidbody>()->SetVelocity(x, y);
	Vector2 normalVector = Vector2(0, 0);
	float normalX = 0, normalY = 0;
	std::vector<Object*> objList = Director::GetInstance()->GetScene()->GetComponent<Tilemap>()->GetChildren();
	for (size_t k = 0; k < objList.size(); k++)
	{
		int checkAABB = CollisionManager::GetInstance()->CheckSweptAABB(thisEntity, objList.at(k), normalX, normalY);
		if (checkAABB < 1) normalVector = Vector2(normalX, normalY);
		else if (checkAABB == 1)
			if (normalX > 0 || normalY > 0)
				normalVector = Vector2(normalX, normalY);
	}
	normalVec = normalVector;
	thisEntity->SetPosition(thisEntity->GetPosition() + Vector2(x, y) + normalVector);

	for (int i = 0; i < this->GetChildren().size(); i++)
	{
		if (this->GetChildren().at(i)->GetTag() == "Bullet")
		{
			bool collided = false;
			Object* bullet = this->GetChildren().at(i);
			for (size_t j = 0; j < entities->size(); j++)
			{
				if (entities->at(j)->id != id && entities->at(j)->GetVisible())
				{
					Object* obj = entities->at(j);
					if (bullet->GetPosition().x < obj->GetPosition().x + obj->GetSize().x
						&& bullet->GetPosition().x + bullet->GetSize().x > obj->GetPosition().x
						&& bullet->GetPosition().y < obj->GetPosition().y + obj->GetSize().y
						&& bullet->GetPosition().y + bullet->GetSize().y > obj->GetPosition().y)
					{
						collided = true;
						RemoveChild(bullet);
						break;
					}
				}
			}

			if (!collided)
			{
				std::vector<Object*> objList = Director::GetInstance()->GetScene()->GetComponent<Tilemap>()->GetChildren();
				for (size_t j = 0; j < objList.size(); j++)
				{
					Object* obj = objList.at(j);
					if (bullet->GetPosition().x < obj->GetPosition().x + obj->GetSize().x
						&& bullet->GetPosition().x + bullet->GetSize().x > obj->GetPosition().x
						&& bullet->GetPosition().y < obj->GetPosition().y + obj->GetSize().y
						&& bullet->GetPosition().y + bullet->GetSize().y > obj->GetPosition().y)
					{
						RemoveChild(bullet);
						break;
					}
				}
			}
		}
	}

	ProcessNetworkMessages(sSocket, hRecvEvent);
}
void NetworkClient::Render()
{
	Object::Render();
	for (size_t i = 0; i < entities->size(); i++) entities->at(i)->Render();

	//std::string text = "ADSSDAD";
	//RECT rect;
	//rect.top = rect.left = 0;
	//rect.right = rect.bottom = 200;
	//my_font->DrawTextA(NULL, text.c_str(), -1, &rect, DT_LEFT, D3DCOLOR_XRGB(255, 242, 0));
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
	Messenger packet;
	packet.action = Action::LogOn;
	packet.position[0] = 100;
	OutputMemoryBitStream os;
	os.Write((int)packet.action, 4);
	/*std::ostringstream oss;
	oss << (int)packet.action;*/
	/*oss << packet.room << " " << packet.id << " " << packet.inputSequenceNumber << " " << packet.tick << " " << action << " " << (int)packet.type << " " <<
		packet.position[0] << " " << packet.position[1] << " " << packet.rotation[0] << " " << packet.size[0] << " " << packet.size[1] << " " << packet.velocity[0] << " " << packet.velocity[1];*/
		//if (SOCKET_ERROR == sendto(sSocket, oss.str().c_str(), sizeof(oss.str().c_str()), 0, (LPSOCKADDR)&addr, sizeof(SOCKADDR_IN)))		return false;
		//if (SOCKET_ERROR == sendto(sSocket, (CHAR*)&packet, sizeof(packet), 0, (LPSOCKADDR)&addr, sizeof(SOCKADDR_IN)))		return false;
	if (SOCKET_ERROR == sendto(sSocket, os.GetBufferPtr(), sizeof(os.GetByteLength()), 0, (LPSOCKADDR)&addr, sizeof(SOCKADDR_IN)))		return false;
	// Wait for a reply or for a few seconds to pass
	if (WAIT_TIMEOUT == WSAWaitForMultipleEvents(1, &hRecvEvent, TRUE, 3000, FALSE)) return false;

	// Recieve the result
	CHAR buffer[MAX_PACKET_SIZE];
	int length;
	SOCKADDR_IN src;
	ZeroMemory(&src, sizeof(src));
	int fromlen = sizeof(SOCKADDR_IN);
	if (SOCKET_ERROR == (length = recvfrom(sSocket, buffer, sizeof(buffer), 0, (LPSOCKADDR)&src, &fromlen))) return false;

	/*LogOnMessage* pMh = (LogOnMessage*)buffer;
	Messenger* msg = (Messenger*)buffer;
	if (pMh->Header.MsgID == Message::MSG_LOGON)
	{
		LogOnMessage * msg = (LogOnMessage*)buffer;
		id = msg->id;

		thisEntity = new NetworkEntity(msg->id, Sprite::Create(L"Resources\\tank.png"));
		this->SetSize(thisEntity->GetSize().x, thisEntity->GetSize().y);
		entities->push_back(thisEntity);
	}*/
	Messenger* msg = (Messenger*)buffer;
	if (msg->action == Action::LogOn)
	{
		id = msg->id;

		thisEntity = new NetworkEntity(msg->id, Sprite::Create(L"Resources\\tank.png"));
		thisEntity->SetPosition(msg->position[0], msg->position[1]);
		thisEntity->AddComponent<Rigidbody>(new Rigidbody());
		this->SetSize(thisEntity->GetSize().x, thisEntity->GetSize().y);
		entities->push_back(thisEntity);
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
					if (msg->id != id)
					{
						entities->at(i)->SetPosition(msg->position[0], msg->position[1]);
						entities->at(i)->SetRotation(msg->rotation[0]);
					}

					if (msg->id == id)
					{
						int j = 0;
						int checkX = abs(entities->at(i)->GetPosition().x - msg->position[0]);
						int checkY = abs(entities->at(i)->GetPosition().y - msg->position[1]);
						if (checkX > 50 || checkY > 50)
						{
							entities->at(i)->SetPosition(msg->position[0], msg->position[1]);
							//entities->at(i)->SetRotation(msg->rotation[0]);
						}
						while (j < pendingInputs.size())
						{
							if (msg->inputSequenceNumber != 0 && pendingInputs.at(j).inputSequenceNumber <= msg->inputSequenceNumber)
							{
								pendingInputs.erase(pendingInputs.begin() + j);
								/*entities->at(i)->SetPosition(msg->position[0], msg->position[1]);
								entities->at(i)->SetRotation(msg->rotation[0]);*/
							}
							else
							{
								if (pendingInputs.at(j).velocity[0] + pendingInputs.at(j).velocity[1] != 0)
								{
									//entities->at(i)->SetPosition(entities->at(i)->GetPosition() + Vector2(pendingInputs.at(j).velocity[0], pendingInputs.at(j).velocity[1]));
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
				newNet->AddComponent<Rigidbody>(new Rigidbody());
				entities->push_back(newNet);
			}
		}
	}
	break;
	case Action::Create:
	{
		if (msg->type == ObjectType::Bullet && msg->id!=id)
		{
				Sprite* bullet = Sprite::Create(L"Resources\\Tank\\bullet.png");
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
			if (entities->at(i)->id == msg->id) entities->erase(entities->begin() + i);
		}
		if (msg->id == id)
		{
			DisconnectFromServer(sSocket);
		}
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
	case Action::Dead:
	{
		for (size_t i = 0; i < entities->size(); i++)
		{
			if (entities->at(i)->id == msg->id) entities->at(i)->SetVisible(false);
		}
		if (msg->id == id)
		{
			isAlive = false;
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

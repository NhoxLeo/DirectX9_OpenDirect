#include "Client.h"
#include "Server.h"
#include <iostream>


Client::Client()
{
	networkEntities = new vector<NetworkEntity*>();
	Init();
	/*while (true)
	{
		Update(1);
	}*/
}


Client::~Client()
{
}

void Client::Update(float deltaTime)
{
	Object::Update(deltaTime);
	if (clientSharedData.isWorkerThreadRunning && clientSharedData.doRun == true)
	{

		ServerCommand command = DoNothing;
		ClientMessage message = None;

		// check if there is something to receive from the server
		DrawingData rcvData, sendData;
		sendData.id = id;
		sendData.moveX = sendData.moveY = 0;
		if (Input::GetInstance()->GetKeyState('A') == KeyState::Pressed) sendData.moveX = -1;
		if (Input::GetInstance()->GetKeyState('D') == KeyState::Pressed) sendData.moveX = 1;
		if (Input::GetInstance()->GetKeyState('W') == KeyState::Pressed) sendData.moveY = -1;
		if (Input::GetInstance()->GetKeyState('S') == KeyState::Pressed) sendData.moveY = 1;

		// wait for the server to send the object that should be drawn
		NetworkerReturnCode selectResult = networker.selectRead(TIMEOUT_INTERVAL);
		if (selectResult == NW_OK)
		{
			//NetworkerReturnCode recvResult = networker.receive<DrawingData>(&rcvData, sizeof(DrawingData));
			NetworkerReturnCode recvResult = networker.receive<ServerCommand>(&command, sizeof(ServerCommand));
			if (recvResult == NW_OK)
			{
				NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
				if (command == RequestInput) do { sendMessageResult = networker.send<DrawingData>(&sendData, sizeof(DrawingData)); } while (sendMessageResult == NW_NOTHING_TO_SEND);
				else if (command == Draw)
				{
					/*NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
					message = ClientAck;
					do { sendMessageResult = networker.send<ClientMessage>(&message, sizeof(ClientMessage)); } while (sendMessageResult == NW_NOTHING_TO_SEND);*/
					/*DrawingData recvData;
					NetworkerReturnCode recvMsgResult = NW_NOTHING_TO_RECEIVE;
					do { recvMsgResult = networker.receive<DrawingData>(&recvData, sizeof(DrawingData)); } while (recvMsgResult == NW_NOTHING_TO_RECEIVE);
					for (int i = 0; i < networkEntities->size(); i++) if (networkEntities->at(i)->id == rcvData.id) networkEntities->at(i)->SetPosition(Vector2(rcvData.posX, rcvData.posY));*/
				}
				
			}
		}
		NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
		if (sendData.moveY != 0 || sendData.moveX != 0) message = Input;
		else message = None;
		do { sendMessageResult = networker.send<ClientMessage>(&message, sizeof(ClientMessage)); } while (sendMessageResult == NW_NOTHING_TO_SEND);

		/*message = None;
		NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
		do { sendMessageResult = networker.send<ClientMessage>(&message, sizeof(ClientMessage)); } while (sendMessageResult == NW_NOTHING_TO_SEND);

		command = DoNothing;
		NetworkerReturnCode receiveCommandResult = NW_NOTHING_TO_SEND;
		do { sendMessageResult = networker.receive<ServerCommand>(&command, sizeof(ServerCommand)); } while (receiveCommandResult == NW_NOTHING_TO_SEND);*/
	}
}

bool Client::Init()
{
	if ((networker.setup() == NW_OK) && (networker.connectToServer(ipAddress.c_str(), 8888) == NW_OK) && (networker.setNonBlockingMode(true) == NW_OK))
	{
		// notify main thread that the connection has been successfully established
		clientSharedData.doRun = true;
		clientSharedData.isWorkerThreadRunning = true;
		clientSharedData.connectionEstablished = true;
		clientSharedData.connectionEstablishedCondition.notify_one();
		//-----------------------------------------------------------------------------------
		// receive an ID from the server from the server, if anything goes wrong, terminate the client

		// wait for the server to send the id or for the function call to timeout
		NetworkerReturnCode selectResult = networker.selectRead(TIMEOUT_INTERVAL);
		switch (selectResult)
		{
		case NW_OK:
		{
			int newId = -1;
			NetworkerReturnCode recvResult = networker.receive<int>(&newId, sizeof(int));
			if (recvResult == NW_OK)
			{
				// acknowledge the receipt of the id
				NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
				ClientMessage message = ClientAck;
				do { sendMessageResult = networker.send<ClientMessage>(&message, sizeof(ClientMessage)); } while (sendMessageResult == NW_NOTHING_TO_SEND);
				if (sendMessageResult == NW_OK)
				{
					id = newId;
					NetworkEntity* newEntity = new NetworkEntity(id, Sprite::Create(L"Resources\\player.png"));
					newEntity->SetPosition(Vector2(0, 0));
					AddChild(newEntity);
					networkEntities->push_back(newEntity);
				}
				else clientSharedData.isWorkerThreadRunning = false;
			}
			else clientSharedData.isWorkerThreadRunning = false;
			break;
		}
		case NW_TIMEOUT:
			clientSharedData.isWorkerThreadRunning = false;
			break;
		case NW_ERROR:
			clientSharedData.isWorkerThreadRunning = false;
			break;
		}

		bool doSend = false;
		return true;
	}
}

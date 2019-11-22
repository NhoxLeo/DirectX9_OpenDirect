#include "Server.h"
#include "Client.h"
#include <iostream>

static int id = 0;

Server::Server()
{
	networkEntities = new vector<NetworkEntity*>();
	clients = new vector<User>();
	Init();
	/*while (true)
	{
		count++;
		Update(1);
	}*/

}

Server::~Server()
{
	Shutdown();
}

void Server::Update(float deltaTime)
{
	Object::Update(deltaTime);
	// Used to trigger initial spawning of space ships. Will be set to true when a client joins if there has previously been no active 
// client and will be set to false after spawning is complete. Regular spawning (when ships are destroyed) is handled elsewhere
// and is not depending on this variable
	ServerCommand command = DoNothing;
	ClientMessage message = None;
	// process the clients as long as the main thread doesn't change the shared flag 'doRun' signalling shut down of the server
	if (serverSharedData.doRun)
	{
		//Register new clients
		Networker newNetworker;
		newNetworker = networker.acceptConnection();
		if (newNetworker.isValid())
		{
			if (newNetworker.setNonBlockingMode(true) == NW_OK)
			{
				// make sure no other thread is currently accessing the list for new networkers
				lock_guard<std::mutex> lock(serverSharedData.newSocketsMutex);
				// add the new networker to the container temporarily storing networkers of new clients
				serverSharedData.newNetworkers.push(newNetworker);
			}
			cout << "\nNew client connected with id: " << id << "\n";
			NetworkEntity* newEntity = new NetworkEntity(id, Sprite::Create(L"Resources\\player.png"));
			newEntity->SetPosition(Vector2(0, 0));
			AddChild(newEntity);
			networkEntities->push_back(newEntity);


			User client;
			client.id = id++;
			client.networker = serverSharedData.newNetworkers.front();
			client.dataReady = false;
			client.lastMessageProcessed = true;
			// send the id to the new client and make sure it is received
			NetworkerReturnCode sendIdResult = NW_NOTHING_TO_SEND;
			do { sendIdResult = client.networker.send<int>(&client.id, sizeof(int)); } while (sendIdResult == NW_NOTHING_TO_SEND);
			if (sendIdResult == NW_OK)
			{
				// id was sent successfully, now wait for the Ack from the client
				NetworkerReturnCode selectResult = client.networker.selectRead(TIMEOUT_INTERVAL);
				switch (selectResult)
				{
				case NW_OK:
				{
					// receive the client's acknowledgement
					message = None;
					NetworkerReturnCode recvAckResult = client.networker.receive<ClientMessage>(&message, sizeof(ClientMessage));
					if (recvAckResult == NW_OK && message == ClientAck) clients->push_back(client);
					else client.networker.shutDown();
					break;
				}
				case NW_TIMEOUT:
					// Ack wasn't received in time, terminate client
					client.networker.shutDown();
					break;
				case NW_ERROR:
					// an error occurred, terminate the client
					client.networker.shutDown();
					break;
				}
			}
			else client.networker.shutDown();
			// either way, remove the client from the waiting queue
			serverSharedData.newNetworkers.pop();

		}

		//// create new clients if the main thread added some new networkers to the queue
		//// if new networkers connected to the server, create a corresponding client data structure and add it to the vector of active clients
		//lock_guard<std::mutex> lock(serverSharedData.newSocketsMutex);
		//while (!(serverSharedData.newNetworkers.empty()))
		//{
		//	// create a Client data structure for the new client
		//	User client;
		//	client.id = id++;
		//	client.networker = serverSharedData.newNetworkers.front();
		//	client.dataReady = false;
		//	client.lastMessageProcessed = true;
		//	// send the id to the new client and make sure it is received
		//	NetworkerReturnCode sendIdResult = NW_NOTHING_TO_SEND;
		//	do { sendIdResult = client.networker.send<int>(&client.id, sizeof(int)); } while (sendIdResult == NW_NOTHING_TO_SEND);
		//	if (sendIdResult == NW_OK)
		//	{
		//		// id was sent successfully, now wait for the Ack from the client
		//		NetworkerReturnCode selectResult = client.networker.selectRead(TIMEOUT_INTERVAL);
		//		switch (selectResult)
		//		{
		//		case NW_OK:
		//		{
		//			// receive the client's acknowledgement
		//			message = None;
		//			NetworkerReturnCode recvAckResult = client.networker.receive<ClientMessage>(&message, sizeof(ClientMessage));
		//			if (recvAckResult == NW_OK && message == ClientAck) clients->push_back(client);
		//			else client.networker.shutDown();
		//			break;
		//		}
		//		case NW_TIMEOUT:
		//			// Ack wasn't received in time, terminate client
		//			client.networker.shutDown();
		//			break;
		//		case NW_ERROR:
		//			// an error occurred, terminate the client
		//			client.networker.shutDown();
		//			break;
		//		}
		//	}
		//	else client.networker.shutDown();
		//	// either way, remove the client from the waiting queue
		//	serverSharedData.newNetworkers.pop();
		//}

		//Interacting with clients					
		// only continue if there are active clients
		if (clients->size() != 0)
		{
			vector<User>::iterator it = clients->begin();
			while (it != clients->end())
			{
				// gets set if the current socket should be erased as consequence to an error occurring 
				// (erase will be executed at the end of the block)
				bool doErase = false;

				if (/*command == ServerAck || command == DoNothing*/ command != Draw)
				{
					NetworkerReturnCode sendResultData = NW_NOTHING_TO_SEND;
					do { sendResultData = (*it).networker.send<ServerCommand>(&(command), sizeof(ServerCommand)); } while (sendResultData == NW_NOTHING_TO_SEND);
				}

				NetworkerReturnCode recvMsgResult = NW_NOTHING_TO_RECEIVE;
				do { recvMsgResult = (*it).networker.receive<ClientMessage>(&message, sizeof(ClientMessage)); } while (recvMsgResult == NW_NOTHING_TO_RECEIVE);
				switch (recvMsgResult)
				{
				case NW_OK:
				{
					if (message == Input)
					{
						command = RequestInput;
						NetworkerReturnCode sendResultData = NW_NOTHING_TO_SEND;
						do { sendResultData = (*it).networker.send<ServerCommand>(&(command), sizeof(ServerCommand)); } while (sendResultData == NW_NOTHING_TO_SEND);
						NetworkerReturnCode selectResult = (*it).networker.selectRead(TIMEOUT_INTERVAL);
						if (selectResult == NW_OK)
						{
							DrawingData recvData;
							NetworkerReturnCode recvMsgResult = NW_NOTHING_TO_RECEIVE;
							do { recvMsgResult = (*it).networker.receive<DrawingData>(&recvData, sizeof(DrawingData)); } while (recvMsgResult == NW_NOTHING_TO_RECEIVE);
							for (int i = 0; i < networkEntities->size(); i++)
							{
								if (networkEntities->at(i)->id == recvData.id)
								{
									networkEntities->at(i)->SetPosition(networkEntities->at(i)->GetPosition() + Vector2(recvData.moveX, recvData.moveY));
								}
							}
							command = Draw;

						}
					}
					else if (message == None) command = ServerAck;
				}
				break;
				case NW_ERROR:
				{
					doErase = true;
				}
				default:
					break;
				}

				//for (int i = 0; i < networkEntities->size(); i++)
				//{
				//	NetworkerReturnCode sendResultData = NW_NOTHING_TO_SEND;
				//	DrawingData drawData;
				//	drawData.id = networkEntities->at(i)->id;
				//	drawData.posX = networkEntities->at(i)->GetPosition().x;
				//	drawData.posY = networkEntities->at(i)->GetPosition().y;
				//	do { sendResultData = (*it).networker.send<DrawingData>(&(drawData), sizeof(DrawingData)); } while (sendResultData == NW_NOTHING_TO_SEND);
				//}

				// Erase the client if an error occured during processing and move on to the next active client
				if (doErase)
				{
					std::cout << "\nClient disconnected. Their id: " << id << "\n";
					// avoids possible invalidation of iterators
					it = clients->erase(it);
				}
				else ++it;
			}
		}

		/*command = DoNothing;
		message = None;

		NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
		do { sendMessageResult = networker.receive<ClientMessage>(&message, sizeof(ClientMessage)); } while (sendMessageResult == NW_NOTHING_TO_SEND);

		NetworkerReturnCode receiveCommandResult = NW_NOTHING_TO_SEND;
		do { sendMessageResult = networker.send<ServerCommand>(&command, sizeof(ServerCommand)); } while (receiveCommandResult == NW_NOTHING_TO_SEND);*/
	}
}

bool Server::Init()
{
	if (networker.setup() != NW_OK)
	{
		cout << "\nSetting up the networker failed.";
		return -1;
	}
	// the two arguments could be obtained by asking for user input but are, in this case, left to these defaut values
	if (networker.bind("any", 8888) != NW_OK)
	{
		cout << "\nBinding the networker failed.";
		return -1;
	}
	if (networker.setNonBlockingMode(true) != NW_OK)
	{
		cout << "\nSetting the networker to non-blocking mode failed.";
		return -1;
	}
	if (networker.startListening(4) != NW_OK)
	{
		cout << "\nPlacing the networker in listening mode failed.";
		return -1;
	}
	else
	{
		// start a thread that will process clients connecting to the server
		serverSharedData.doRun = true;
		//workerThread = thread(ClientCommunicator(), &sharedData);
		// Determine the IP address of the machine the server is running on and display it on the console.
		// (as the machine may have several IP addresses associated to it, all of them will be displayed and it is up to
		// the user to choose the correct one when starting a client)
		// This is just for convenience (if it doesn't work, ipconfig has to be used to obtain server's IP)
		list<string> ipAddresses = Networker::getLocalIPs();
		if (!ipAddresses.empty())
		{
			cout << "\nIP addresses associated to this machine:";
			for (list<string>::iterator it = ipAddresses.begin(); it != ipAddresses.end(); ++it) cout << "\n - " << (*it);
			cout << endl;
		}
		cout << "\nThis machine is now running the server side of the application.\nYou can now run the 'Client.exe' on this computer or another one in order to\ncreate a client and connect to the server.";
		cout << "\nYou can terminate the server at any time by closing this console window.\nThis will also lead to the termination of all clients currently connected\nto the server." << endl;
		cout << "\nWaiting for incoming connections...";
	}
	return true;
}

void Server::Shutdown()
{
	// main thread has issued termination of the worker thread;
	// shut down the networker of all clients
	for (vector<User>::iterator it = clients->begin(); it != clients->end(); ++it) (*it).networker.shutDown(); // no explicit 'Disconnect' message required as the clients will shut down themselves as soon as they notice that their networker has been shut down
	// delete all clients
	clients->clear();
	// WSACleanup() will be executed by main thread
}

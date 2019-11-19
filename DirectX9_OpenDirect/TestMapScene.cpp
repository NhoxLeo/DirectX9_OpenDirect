#define _CRT_SECURE_NO_WARNINGS

#include "Precompiled.h"
#include "TestMapScene.h"

bool g_isClosing = false;
const long TIMEOUT_INTERVAL = 5000;
static int id = 0;

TestMapScene::TestMapScene() {}
TestMapScene::~TestMapScene()
{
	if (isServer) ServerShutdown();
	else ClientShutdown();
}

bool TestMapScene::Initialize() {
	Scene::Initialize();
	InitializeUI();

	//m_Map = Map::Create(64, 64);
	//AddChild(m_Map);

	// 카메라 이동
	GetCamera()->Translate(0, 0);

	//PhysicsManager*a = PhysicsManager::GetInstance();


	return true;
}

void TestMapScene::InitializeUI() {
	isServer = true;

	m_UI = Object::Create();
	m_UI->SetZOrder(1000);
	entity = Sprite::Create(L"Resources\\player.png");
	entity->SetAnchorPoint(0.f, 0.f);
	entity->SetPosition(0, 0);
	m_UI->SetPosition(0, 200);
	//Rigidbody*a = new Rigidbody();
	//a->SetVelocity(10.f,10.f);
	//PhysicsManager::GetInstance()->RegisterRigidbody(a);
	//m_UI->AddChild(a);
	//m_UI->AddChild(animation);
	m_UI->SetParent(nullptr);
	GetCamera()->Translate(-200, 0);

	// create a console in addition to the window and bind the standard streams to it
	// console will be used to provide some instructions on startup and to read in the ip address of a server to connect to
	FILE *stream;
	AllocConsole();
	freopen_s(&stream, "CONIN$", "r+t", stdin);
	freopen_s(&stream, "CONOUT$", "w+t", stdout);
	freopen_s(&stream, "CONOUT$", "w+t", stderr);
	cout << "Choose 'server' or 'client' by typing it down \n";
	string option;
	do
	{
		cout << "Choose 'server' or 'client' by typing 'server' or 'client' down \n";
		cin >> option;
	} while (option != "server" && option != "client");
	if (option == "server")
	{
		isServer = true;

		////if (!networkManager->UDP_Host_Init()) MessageBox(NULL, L"Error UDP_Host_Init", L"Error", 0);
		//server = new Server();
		////server->entity = m_UI;
		//AddChild(server);
		////AddChild(m_UI);

		ServerInit();
	}
	else if (option == "client")
	{
		isServer = false;

		////if (!networkManager->UDP_Client_Init()) MessageBox(NULL, L"Error UDP_Client_Init", L"Error", 0);
		//client = new Client();
		//AddChild(client);

		CLientInit();
	}
}

void TestMapScene::Update(float deltaTime) {
	Scene::Update(deltaTime);
	//UpdateCamera();
	if (isServer)
	{
		ServerUpdate();
		//server->Update(deltaTime);
	}
	else
	{
		ClientUpdate();
		//client->Update(deltaTime);
	}
}

void TestMapScene::Render() {
	Object::Render();
}

void TestMapScene::UpdateCamera() {
	if (Input::GetInstance()->GetKeyState('A') == KeyState::Pressed) {
		GetCamera()->Translate(-15, 0);
	}
	else if (Input::GetInstance()->GetKeyState('D') == KeyState::Pressed) {
		GetCamera()->Translate(15, 0);
	}
	if (Input::GetInstance()->GetKeyState('W') == KeyState::Pressed) {
		GetCamera()->Translate(0, -15);
	}
	else if (Input::GetInstance()->GetKeyState('S') == KeyState::Pressed) {
		GetCamera()->Translate(0, 15);
	}

	if (Input::GetInstance()->GetKeyState('R') == KeyState::Pressed) {
		GetCamera()->Scale(0.01f, 0.01f);
	}
	else if (Input::GetInstance()->GetKeyState('T') == KeyState::Pressed) {
		GetCamera()->Scale(-0.01f, -0.01f);
	}

	if (Input::GetInstance()->GetKeyState('Q') == KeyState::Pressed) {
		GetCamera()->Rotate(0.05f);
	}
	else if (Input::GetInstance()->GetKeyState('E') == KeyState::Pressed) {
		GetCamera()->Rotate(-0.05f);
	}
}

bool TestMapScene::ServerInit()
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
bool TestMapScene::CLientInit()
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
				do
				{
					sendMessageResult = networker.send<ClientMessage>(&message, sizeof(ClientMessage));
				} while (sendMessageResult == NW_NOTHING_TO_SEND);

				if (sendMessageResult == NW_OK)
				{
					id = newId;
				}
				else
				{
					// an error occurred
					clientSharedData.isWorkerThreadRunning = false;
				}
			}
			else
			{
				clientSharedData.isWorkerThreadRunning = false;
			}
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
void TestMapScene::ServerUpdate()
{
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
		}
		command = DoNothing;
		message = None;
		// create new clients if the main thread added some new networkers to the queue
		{
			// if new networkers connected to the server, create a corresponding client data structure and add it to the vector of active clients
			lock_guard<std::mutex> lock(serverSharedData.newSocketsMutex);
			while (!(serverSharedData.newNetworkers.empty()))
			{
				// create a Client data structure for the new client
				Clients client;
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
						if (recvAckResult == NW_OK && message == ClientAck)
						{
							// the client received the id and answered with an acknowledgement . add to active clients
							clients.push_back(client);
						}
						else
						{
							// An error occurred, shut down the client's networker. This will lead to the client terminating itself
							client.networker.shutDown();
						}
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
				else
				{
					// an error occurred, terminate the client
					client.networker.shutDown();
				}
				// either way, remove the client from the waiting queue
				serverSharedData.newNetworkers.pop();
			}
		}

		//Interacting with clients					
		// only continue if there are active clients
		if (clients.size() != 0)
		{
			vector<Clients>::iterator it = clients.begin();
			NetworkerReturnCode sendResultData = NW_NOTHING_TO_SEND;
			command = Draw;
			DrawingData drawData;
			drawData.id = 10;
			do { sendResultData = (*it).networker.send<DrawingData>(&(drawData), sizeof(DrawingData)); } while (sendResultData == NW_NOTHING_TO_SEND);
			NetworkerReturnCode recvMsgResult = NW_NOTHING_TO_RECEIVE;
			do { recvMsgResult = (*it).networker.receive<ClientMessage>(&message, sizeof(ClientMessage)); } while (recvMsgResult == NW_NOTHING_TO_RECEIVE);


		}
	}
}
void TestMapScene::ClientUpdate()
{
	if (clientSharedData.isWorkerThreadRunning && clientSharedData.doRun == true)
	{

		ServerCommand command = DoNothing;
		ClientMessage message = None;

		// update containers holding objects leaving this client and objects being destroyed

		// get objects that have just reached the end of the screen
		{
			lock_guard<std::mutex> newSendObjectsLock(clientSharedData.newSendObjectsMutex);
			while (!(clientSharedData.newSendObjects.empty()))
			{
				SendQueueData data;
				data.message = ObjectMoved;
				data.drawObject = clientSharedData.newSendObjects.front();
				sendQueue.push(data);
				clientSharedData.newSendObjects.pop();
			}
		}

		// get the objects that have just been destroyed
		{
			lock_guard<std::mutex> newDestroySendObjectsLock(clientSharedData.newDestroySendObjectsMutex);
			while (!(clientSharedData.newDestroySendObjects.empty()))
			{
				SendQueueData data;
				data.message = ObjectDestroyed;
				data.drawObject = clientSharedData.newDestroySendObjects.front();
				sendQueue.push(data);
				clientSharedData.newDestroySendObjects.pop();
			}
		}

		// check if there is something to receive from the server

		DrawingData rcvData;
		// wait for the server to send the object that should be drawn
		NetworkerReturnCode selectResult = networker.selectRead(TIMEOUT_INTERVAL);
		if (selectResult == NW_OK)
		{
			NetworkerReturnCode recvResult = networker.receive<DrawingData>(&rcvData, sizeof(DrawingData));
			cout << rcvData.id << "\n";
		}
		NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
		message = ClientAck;
		do { sendMessageResult = networker.send<ClientMessage>(&message, sizeof(ClientMessage)); } while (sendMessageResult == NW_NOTHING_TO_SEND);
	}
}
void TestMapScene::ServerShutdown()
{
	// main thread has issued termination of the worker thread;
	// shut down the networker of all clients
	for (vector<Clients>::iterator it = clients.begin(); it != clients.end(); ++it) (*it).networker.shutDown(); // no explicit 'Disconnect' message required as the clients will shut down themselves as soon as they notice that their networker has been shut down
	// delete all clients
	clients.clear();
	// WSACleanup() will be executed by main thread
}
void TestMapScene::ClientShutdown()
{
	// main thread has issued termination of the worker thread;
	// shut down the networker of all clients
	for (vector<Clients>::iterator it = clients.begin(); it != clients.end(); ++it) (*it).networker.shutDown(); // no explicit 'Disconnect' message required as the clients will shut down themselves as soon as they notice that their networker has been shut down
	// delete all clients
	clients.clear();
	// WSACleanup() will be executed by main thread
}
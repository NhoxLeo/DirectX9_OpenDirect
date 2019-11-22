#define _CRT_SECURE_NO_WARNINGS

#include "Precompiled.h"
#include "TestMapScene.h"

//bool g_isClosing = false;
//const long TIMEOUT_INTERVAL = 5000;
//static int id = 0;

TestMapScene::TestMapScene() {}
TestMapScene::~TestMapScene()
{

}

bool TestMapScene::Initialize() {
	Scene::Initialize();
	InitializeUI();
	//m_Map = Map::Create(64, 64);
	//AddChild(m_Map);
	GetCamera()->Translate(0, 0);
	//PhysicsManager*a = PhysicsManager::GetInstance();

	return true;
}

void TestMapScene::InitializeUI() {
	isServer = true;
	GetCamera()->Translate(0, 0);

	/*entity = Sprite::Create(L"Resources\\player.png");
	entity->SetAnchorPoint(0.f, 0.f);
	entity->SetPosition(0, 0);
	AddChild(entity);*/

	//tilemap = new Tilemap(Sprite::Create(L"Resources\\level1.bmp"), L"Resources\\level1.bmp", L"Resources\\level1.txt", 40, 1, 13, 13, "Level1");
	//tilemap->SetAnchorPoint(0.f, 0.f);
	//tilemap->SetPosition(0, 0);
	//AddChild(tilemap);

	// create a console in addition to the window and bind the standard streams to it
	// console will be used to provide some instructions on startup and to read in the ip address of a server to connect to
	FILE *stream;
	AllocConsole();
	freopen_s(&stream, "CONIN$", "r+t", stdin);
	freopen_s(&stream, "CONOUT$", "w+t", stdout);
	freopen_s(&stream, "CONOUT$", "w+t", stderr);
	//cout << "Choose 'server' or 'client' by typing it down \n";
	//string option;
	//do
	//{
	//	cout << "Choose 'server' or 'client' by typing 'server' or 'client' down \n";
	//	cin >> option;
	//} while (option != "server" && option != "client");
	//if (option == "server")
	//{
	//	isServer = true;
	//	server = new Server();
	//	AddChild(server);
	//	//ServerInit();
	//}
	//else if (option == "client")
	//{
	//	isServer = false;
	//	client = new Client();
	//	AddChild(client);
	//	//CLientInit();
	//}

	networkClient = new NetworkClient();
	AddChild(networkClient);
}

void TestMapScene::Update(float deltaTime) {
	Scene::Update(deltaTime);
	UpdateCamera();
	if (isServer)
	{
		//ServerUpdate();
		//server->Update(deltaTime);
	}
	else
	{
		//ClientUpdate();
		//client->Update(deltaTime);
	}
	networkClient->Update(deltaTime);
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

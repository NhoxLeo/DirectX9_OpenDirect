#define _CRT_SECURE_NO_WARNINGS

#include "Precompiled.h"
#include "TestMapScene.h"
#include <typeinfo>
#include <fstream>

//bool g_isClosing = false;
//const long TIMEOUT_INTERVAL = 5000;
//static int id = 0;

TestMapScene::TestMapScene() {}
TestMapScene::~TestMapScene() {}

bool TestMapScene::Initialize() {
	Scene::Initialize();
	InitializeUI();
	//m_Map = Map::Create(64, 64);
	//AddChild(m_Map);
	GetCamera()->Translate(0, 0);
	//PhysicsManager*a = PhysicsManager::GetInstance();
	collisionManager = CollisionManager::GetInstance();

	return true;
}

void TestMapScene::InitializeUI() {
	isServer = true;
	GetCamera()->Translate(0, 0);
	/*FILE *stream;
	AllocConsole();
	freopen_s(&stream, "CONIN$", "r+t", stdin);
	freopen_s(&stream, "CONOUT$", "w+t", stdout);
	freopen_s(&stream, "CONOUT$", "w+t", stderr);*/

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
	string ip = "127.0.0.1";
	/*cout << "Type in the server address \n";
	cin >> ip;*/

	tilemap = new Tilemap(Sprite::Create(L"Resources\\Level1.bmp"), L"Resources\\Level1.bmp", L"Resources\\Level1.txt", 20, 3, 26, 26, "Level1");
	tilemap->SetAnchorPoint(0.f, 0.f);
	tilemap->SetPosition(0, 0);
	AddChild(tilemap);

	networkClient = new NetworkClient(ip);
	AddChild(networkClient);

	/*entity = Sprite::Create(L"Resources\\tank.png");
	entity->SetSize(16, 16);
	entity->AddComponent<BoxCollider>(new BoxCollider());
	AddChild(entity);
	entity->AddComponent<Rigidbody>(new Rigidbody());*/
}

void TestMapScene::Update(float deltaTime) {
	Scene::Update(deltaTime);
	//UpdateCamera();

	/*entity->GetComponent<Rigidbody>()->SetVelocity(
		(Input::GetInstance()->GetKeyState('A') == KeyState::Pressed) ? -1 : ((Input::GetInstance()->GetKeyState('D') == KeyState::Pressed) ? 1 : 0)
		, (Input::GetInstance()->GetKeyState('W') == KeyState::Pressed) ? -1 : ((Input::GetInstance()->GetKeyState('S') == KeyState::Pressed) ? 1 : 0));
	if (entity->GetComponent<Rigidbody>()->GetVelocity().x != 0 || entity->GetComponent<Rigidbody>()->GetVelocity().y != 0)
	{
		entity->SetRotation(180 * 3.14f / 180);
	}
	Vector2 normalVector = Vector2(0, 0);
	for (size_t j = 0; j < m_Children.size(); j++)
	{
		float normalX, normalY;
		int checkAABB = collisionManager->CheckSweptAABB(entity, m_Children.at(j), normalX, normalY);
		if (checkAABB < 1) normalVector = Vector2(normalX, normalY);
		else if (checkAABB == 1) if (normalX > 0 || normalY > 0) normalVector = Vector2(normalX, normalY);
	}
	entity->SetPosition(entity->GetPosition() + entity->GetComponent<Rigidbody>()->GetVelocity() + normalVector);*/

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

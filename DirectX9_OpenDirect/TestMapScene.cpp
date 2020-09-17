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
	fireRate = 0.5f;
	fireTime = 0;

	//string ip = "127.0.0.1";

	/*FILE *stream;
	AllocConsole();
	freopen_s(&stream, "CONIN$", "r+t", stdin);
	freopen_s(&stream, "CONOUT$", "w+t", stdout);
	freopen_s(&stream, "CONOUT$", "w+t", stderr);
	string ip;
	cout << "Type in the server address \n";
	cin >> ip;*/

	tilemap = new Tilemap(Sprite::Create(L"Resources\\Level1.bmp"), L"Resources\\Level1.bmp", L"Resources\\Level1.txt", 20, 3, 26, 26, "Level1");
	tilemap->SetAnchorPoint(0.f, 0.f);
	tilemap->SetPosition(0, 0);
	AddChild(tilemap);
	/*for (size_t i = 0; i < tilemap->GetAllObjects().size(); i++)
	{
		AddChild(tilemap->GetAllObjects().at(i));
	}*/

	/*networkClient = new NetworkClient(ip);
	AddChild(networkClient);*/

	entity = Object::Create();
	entity->SetSize(16, 16);
	entity->SetPosition(0, 0);
	entity->SetTag("Player");
	AddChild(entity);
	entity->AddComponent<Rigidbody>(new Rigidbody());
	entity->AddComponent<AnimationSprite>(new AnimationSprite(L"Resources\\Tank1Up.png", 1, 2, 1));
	
}

void TestMapScene::Update(float deltaTime) {
	Scene::Update(deltaTime);
	//UpdateCamera();
	/*if (Input::GetInstance()->GetKeyState('Q') == KeyState::Pressed && fireTime > fireRate)
	{
		fireTime = 0;
		Sprite* bullet = Sprite::Create(L"Resources\\Tank\\testbullet.bmp");
		bullet->SetTag("Bullet");
		bullet->AddComponent<Rigidbody>(new Rigidbody());
		bullet->SetPosition(entity->GetPosition().x, entity->GetPosition().y);
		bullet->GetComponent<Rigidbody>()->SetVelocity(entity->GetComponent<Rigidbody>()->GetVelocity().x, entity->GetComponent<Rigidbody>()->GetVelocity().y);
		AddChild(bullet);
	}
	if (fireTime <= fireRate) fireTime += deltaTime;*/


	if (Input::GetInstance()->GetKeyState('A') == KeyState::Pressed)
	{
		entity->GetComponent<Rigidbody>()->SetVelocity(-0.5f, 0);
		entity->SetRotation(-90 * 3.14f / 180);
	}
	else if (Input::GetInstance()->GetKeyState('D') == KeyState::Pressed)
	{
		entity->GetComponent<Rigidbody>()->SetVelocity(0.5f, 0);
		entity->SetRotation(90 * 3.14f / 180);
	}
	else if (Input::GetInstance()->GetKeyState('W') == KeyState::Pressed)
	{
		entity->GetComponent<Rigidbody>()->SetVelocity(0, -0.5f);
		entity->SetRotation(0);
	}
	else if (Input::GetInstance()->GetKeyState('S') == KeyState::Pressed)
	{
		entity->GetComponent<Rigidbody>()->SetVelocity(0, 0.5f);
		entity->SetRotation(180 * 3.14f / 180); 
	}
	else
	{
		entity->GetComponent<Rigidbody>()->SetVelocity(0, 0);
	}
	
	
	Vector2 normalVector = Vector2(0, 0);
	float normalX, normalY;
	for (size_t i = 0; i < m_Children.size(); i++)
	{
		if (m_Children.at(i)->GetTag() == "Player")
		{
			for (size_t j = 0; j < m_Children.size(); j++)
			{
				if (j != i)
				{
					int checkAABB = collisionManager->CheckSweptAABB(m_Children.at(i), m_Children.at(j), normalX, normalY);
					if (checkAABB < 1)
						normalVector = Vector2(normalX, normalY);
					else if (checkAABB == 1)
						if (normalX > 0 || normalY > 0) normalVector = Vector2(normalX, normalY);
				}
			}
		}
		//else if (m_Children.at(i)->GetTag() == "Bullet")
		//{
		//	for (size_t j = 0; j < m_Children.size(); j++)
		//	{
		//		if (j != i && m_Children.at(j)->GetTag() != "Player")
		//		{
		//			Object* bullet = m_Children.at(i);
		//			Object* obj = m_Children.at(j);
		//			int checkAABB = collisionManager->CheckSweptAABB(bullet, m_Children.at(j), normalX, normalY);
		//			if (checkAABB < 1)
		//			{
		//				tilemap->EraseObject(obj->GetPosition());
		//				m_Children.erase(m_Children.begin() + i);
		//				m_Children.erase(m_Children.begin() + j);
		//				normalVector = Vector2(normalX, normalY);
		//				break;
		//			}
		//			else if (checkAABB == 1) if (normalX > 0 || normalY > 0) normalVector = Vector2(normalX, normalY);
		//			//if (bullet->GetPosition().x < obj->GetPosition().x + obj->GetSize().x
		//			//	&& bullet->GetPosition().x + bullet->GetSize().x > obj->GetPosition().x
		//			//	&& bullet->GetPosition().y < obj->GetPosition().y + obj->GetSize().y
		//			//	&& bullet->GetPosition().y + bullet->GetSize().y > obj->GetPosition().y)
		//			//{
		//			//	//tilemap->GetAllObjects().erase(tilemap->GetAllObjects().begin() + j);
		//			//	//tilemap->EraseObject(std::stoi(obj->GetName()));
		//			//	tilemap->EraseObject(obj->GetPosition());
		//			//	m_Children.erase(m_Children.begin() + i);
		//			//	m_Children.erase(m_Children.begin() + j);
		//			//	break;
		//			//}
		//		}
		//	}
		//}
	}
	entity->SetPosition(entity->GetPosition() + entity->GetComponent<Rigidbody>()->GetVelocity() + normalVector);
}
void TestMapScene::Render() {
	Object::Render();
	//networkClient->Render();
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

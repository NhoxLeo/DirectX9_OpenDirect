#include "Precompiled.h"
#include "TestMapScene.h"

TestMapScene::TestMapScene() {}
TestMapScene::~TestMapScene() {}

bool TestMapScene::Initialize() {
	Scene::Initialize();
	InitializeUI();

	//m_Map = Map::Create(64, 64);
	//AddChild(m_Map);

	// ī�޶� �̵�
	GetCamera()->Translate(0, 0);

	//PhysicsManager*a = PhysicsManager::GetInstance();


	return true;
}

void TestMapScene::InitializeUI() {
	m_UI = Object::Create();
	m_UI->SetZOrder(1000);
	AddChild(m_UI);

	/*auto p_water = ProgressBar::Create(L"Resources\\ui_water_background.png", L"Resources\\ui_water_foreground.png");
	p_water->SetPosition(10, 10);
	p_water->SetValue(100);*/

	auto p_water_label = Sprite::Create(L"Resources\\animation.png");
	AnimationSprite* animation = new AnimationSprite(L"Resources\\animations\\cap1.png",1,11,5.0f);
	animation->SetAnchorPoint(0.f, 0.f);
	animation->SetPosition(200, 200);
	p_water_label->SetAnchorPoint(0.f, 0.f);
	p_water_label->SetPosition(300, 300);

	//p_water->AddChild(p_water_label);

	//m_UI->AddChild(p_water_label);
	Rigidbody*a = new Rigidbody();
	//a->SetVelocity(10.f,10.f);
	PhysicsManager::GetInstance()->RegisterRigidbody(a);
	m_UI->AddChild(a);
	m_UI->AddChild(animation);

	m_UI->SetParent(nullptr);
	//// ���� ������
	//auto p_water = ProgressBar::Create(L"Resources\\ui_water_background.png", L"Resources\\ui_water_foreground.png");
	//p_water->SetPosition(10, 10);
	//p_water->SetValue(100);

	//auto p_water_label = Sprite::Create(L"Resources\\ui_water_label.png");
	//p_water_label->SetAnchorPoint(0.f, 0.f);
	//p_water_label->SetPosition(10, 6);

	//p_water->AddChild(p_water_label);

	//m_UI->AddChild(p_water);

	//// ���� ����
	//auto slotBox = Object::Create();
	//slotBox->SetAnchorPoint(.0f, .0f);
	//slotBox->SetPosition(382.5f, 650);

	//for (int i = 0; i < 8; i++) {
	//	auto slot = new ItemSlot();
	//	slot->Initialize(static_cast<ItemID::Enum>(i + 1));
	//	slot->SetPosition(65 * i, 0);

	//	slotBox->AddChild(slot);
	//}

	//m_UI->AddChild(slotBox);
}

void TestMapScene::Update(float deltaTime) {
	Scene::Update(deltaTime);
	UpdateCamera();
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
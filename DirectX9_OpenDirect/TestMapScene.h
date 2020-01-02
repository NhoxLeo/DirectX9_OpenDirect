#pragma once

#define	WIN32_LEAN_AND_MEAN
#include "stdafx.h"
#include <iostream>
#include "NetworkClient.h"
#include "Tilemap.h"

using namespace std;

class TestMapScene : public Scene {
private:
	//Map* m_Map;
	Object* m_UI;
	bool isServer;
	Sprite *entity, *block1, *block2;

	NetworkClient* networkClient;
	CollisionManager* collisionManager;
	Tilemap* tilemap;

	float fireRate, fireTime;
public:
	TestMapScene();
	~TestMapScene();

public:
	CREATE_FUNC(TestMapScene);

public:
	bool Initialize() override;

public:
	void InitializeUI();

public:
	void Update(float deltaTime) override;
	void UpdateCamera();

	//bool sendToClient(Clients & client, Action & action);

	//void shutDownClient(Clients & client);

public:
	void Render() override;
};


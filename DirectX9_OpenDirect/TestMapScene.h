#pragma once

#define	WIN32_LEAN_AND_MEAN
#include "stdafx.h"
#include "NetworkCommunicationData.h"
#include <iostream>

using namespace std;

class TestMapScene : public Scene {
private:
	//Map* m_Map;
	Object* m_UI;
	bool isServer;

	Networker networker;

	////Server
	vector<Clients> clients;
	ServerSharedData serverSharedData;

	////Client
	ClientSharedData clientSharedData;
	string ipAddress = "localhost";
	queue<SendQueueData> sendQueue;
	int id = 0;

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

	bool ServerInit();

	bool CLientInit();

	void ServerUpdate();

	void ClientUpdate();

	void ServerShutdown();

	void ClientShutdown();

public:
	void Render() override;
};


#pragma once
#include "NetworkCommunicationData.h"

class Client;
class Server : public Object
{
public:
	Server();
	~Server();
	int count = 0;
	float currentTime, timeStep;

	bool g_isClosing = false;
	const long TIMEOUT_INTERVAL = 5000;

	Networker networker;
	vector<User>* clients;
	ServerSharedData serverSharedData;
	vector<NetworkEntity*>* networkEntities;

	void Update(float deltaTime) override;

	bool Init();
	void Shutdown();
};


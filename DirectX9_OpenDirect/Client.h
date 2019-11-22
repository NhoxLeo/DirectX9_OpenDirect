#pragma once
#include "NetworkCommunicationData.h"

class Server;
class Client : public Object
{
public:
	Client();
	~Client();
	int count = 0;
	float currentTime, timeStep;

	bool g_isClosing = false;
	const long TIMEOUT_INTERVAL = 5000;

	Networker networker;

	int id = 0;
	ClientSharedData clientSharedData;
	string ipAddress = "localhost";
	queue<SendQueueData> sendQueue;

	vector<NetworkEntity*>* networkEntities;

	void Update(float deltaTime) override;

	bool Init();
};


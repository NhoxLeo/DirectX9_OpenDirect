#pragma once

#include <iostream>
#include <queue>
//#include <Ws2tcpip.h>
//#include <iterator>
#include "NetworkCommunicationData.h"
#include "Networker.h"
#include "ClientSharedData.h"

struct SendQueueData
{
	ClientMessage message;
	DrawingData drawObject;
};

class ServerCommunicator
{
public:
	// determines how long the thread will wait for send operations to be answered by an acknowledgement of the Server (in milliseconds)
	static const long TIMEOUT_INTERVAL;

	void operator()(ClientSharedData* sharedData, string ipAddress);
	ServerCommunicator(void);
	~ServerCommunicator(void);
private:
	Networker networker;
	// identifier received from the server
	int id;
	// holds data structures consisting of a client message and corresponding data, that will be sent to the server
	queue<SendQueueData> sendQueue;
};
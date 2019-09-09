#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include "NetworkCommunicationData.h"

using namespace std;

// the two client threads share an instance of this data type in order to be able to exchange data
struct ClientSharedData
{
public:
	// needed to distinguish between client-sided and server-sided termination
	bool doRun; // gets set to false by the client main thread if the client is about to shut down
	bool isWorkerThreadRunning; // gets set to false by the client worker thread if it encountered an error

	// the following data structures are used to exchange data between the client threads

	// objects that have to be added to the objects being drawn by the client
	queue<DrawingData> newDrawObjects;
	mutex newDrawObjectsMutex;
	// objects that have reached the end of the client's screen and have to be sent to the server
	queue<DrawingData> newSendObjects;
	mutex newSendObjectsMutex;
	// Drawing Data objects currently drawn by the client that should be destroyed
	queue<DrawingData> newDestroyObjects;
	mutex newDestroyObjectsMutex;
	// objects that have been destroyed on the client's screen
	queue<DrawingData> newDestroySendObjects;
	mutex newDestroySendObjectsMutex;

	// the three objects below are used to signal to the main thread that a connection to the server has successfully been established
	bool connectionEstablished;
	mutex connectionEstablishedMutex;
	condition_variable connectionEstablishedCondition;
};


#pragma once

#include <mutex>
#include <queue>
#include <condition_variable>
#include "Networker.h"

using namespace std;

// the two server threads share an instance of this data type in order to be able to exchange data
struct ServerSharedData
{
	// constructor, initialize boolean variables
	ServerSharedData() : doRun(false) {}
	// temporarily stores new clients that have connected to the server during the last drawing cycle
	queue<Networker> newNetworkers;
	// has to be acquired in order to write to or read from list of new clients
	mutex newSocketsMutex;
	// as long as this is set to true, the worker thread keeps running
	bool doRun;
};
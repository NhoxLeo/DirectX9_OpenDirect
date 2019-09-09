#ifndef PROCESS_CLIENTS_FUNCTOR_H
#define PROCESS_CLIENTS_FUNCTOR_H

#include <list>
#include <mutex>
#include <iterator>
#include <vector>
#include <map>
#include <iostream>

#include "NetworkCommunicationData.h"
#include "Networker.h"
#include "ServerSharedData.h"

using namespace std;


struct Action
{
	ServerCommand command;
	DrawingData data;
};

// encompasses information about a connected client required by the server
struct Client
{
public:
	// an identifier for the client (not really used yet, but would be required for things like scoring)
	int id;
	// the socket used to communicate with the client
	Networker networker;
	// the space ships that are currently being drawn by the client (mostly required to prevent loss of these objects in case of
	// unclean disconnection of a client)
	list<DrawingData> spaceShips;
	// tells whether this client has sent data or not
	bool dataReady;
	// Used for synchronization of the client and server. When this set, the client is free to send new data, otherwise the server
	// has not yet processed the last message/data and the client is not allowed to send new stuff
	bool lastMessageProcessed;
	// Stores actions to be executed on the client when it is his turn to be processed
	queue<Action> actions;

	bool operator==(const Client& rhs) const
	{
		return id == rhs.id;
	}
};

class ProcessClientsFunctor
{
public:
	static const int MAX_NUMBER_OF_SPACESHIPS; // this many objects of type SpaceShip can be deployed
	static const float FLIGHT_HEIGHTS[]; // stores available flight heights for the space ships (at the moment: fix values)

	// determines how long the thread will wait for send operations to be answered by an acknowledgement of the Client
	static const long TIMEOUT_INTERVAL;

	void operator()(ServerSharedData* sharedData);

	ProcessClientsFunctor(void);
	~ProcessClientsFunctor(void);
private:
	// stores all active clients
	vector<Client> clients;
	// stores which spaceship is flying on which height; used to prevent intersections of objects
	map<float, int> flightTracks;

	void createDrawingData(DrawableObject object, DrawingData& data);
	int getSpaceShipId();
	int getClientId();

	bool sendToClient(Client& client, Action& action);
	void shutDownClient(Client& client);
	void moveObject(Client& client, DrawingData& data);
	void destroyObject(Client& client, DrawingData& data);
};

#endif 
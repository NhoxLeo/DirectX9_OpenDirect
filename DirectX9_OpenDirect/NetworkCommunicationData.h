#pragma once
#include "stdafx.h"
#include "Networker.h"
#include <mutex>
#include <queue>
#include <condition_variable>



enum ServerCommand
{
	DoNothing, ServerAck, Send, Draw, Destroy, Disconnect, RequestInput
};
enum ClientMessage
{
	None, Input, ClientAck, DataReady, ObjectDestroyed, ObjectMoved, Disconnected
};
enum DrawableObject
{
	SpaceShip, Arrow, Projectile, Background
};
struct DrawingData
{
public:
	int count;
	// at the moment this value is only used to identify objects with type SpaceShip and objects of other types are simply assigned an id of -1
	int id;
	// The origin value is used to allow a client to distinguish between objects created by himself and objects created by the server 
	// or coming from other clients. (id = -1 -> "native" objects, id = 0 -> created by server, id = 1 -> created by other clients)
	// Note that a fired projectile (initially with origin -1) will be assigned a value of 1 once leaving the client's screen.
	int origin;
	// Type of the object to be drawn
	DrawableObject object;
	// translation that is applied every frame
	float moveX, moveY, moveZ;
	float posX, posY, posZ;
	// this flag is set when the object reaches the end of the screen and is sent to the next client (avoids repeated sending)
	bool wasSent;

	// At the moment, this operator is mostly used to compare objects of type SpaceShip and is in its current form unable to 
	// distinguish between projectiles
	bool operator==(const DrawingData& rhs) const
	{
		return id == rhs.id && origin == rhs.origin && object == rhs.object;
	}
	bool operator<(const DrawingData& rhs) const
	{
		return id < rhs.id;
	}
};
struct Action
{
	ServerCommand command;
	DrawingData data;
};
struct SendQueueData
{
	ClientMessage message;
	DrawingData drawObject;
};
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
struct User
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

	bool operator==(const User& rhs) const
	{
		return id == rhs.id;
	}
};
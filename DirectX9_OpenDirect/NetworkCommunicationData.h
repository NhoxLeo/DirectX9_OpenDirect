#pragma once
// This file holds enumerations and data types that are required by both, server and client.
// Objects of these types are being sent over the network between client and server

// commands that the server will send to the connected clients
enum ServerCommand
{
	DoNothing, ServerAck, Send, Draw, Destroy, Disconnect
};

// messages sent by the client to notify the server of certain events
enum ClientMessage
{
	None, ClientAck, DataReady, ObjectDestroyed, ObjectMoved, Disconnected
};

// used to identify objects that can be drawn by the client (client has a vertex buffer and render method for each type of object)
enum DrawableObject
{
	SpaceShip, Arrow, Projectile, Background
};

// holds all information required by the client to draw an object of a specific type
// (currently there are more variables in it than actually needed, i.e. some of the rotation variables, to allow for easy 
// extension later on) 
struct DrawingData
{
public:
	// at the moment this value is only used to identify objects with type SpaceShip and objects of other types are simply assigned an id of -1
	int id;
	// The origin value is used to allow a client to distinguish between objects created by himself and objects created by the server 
	// or coming from other clients. (id = -1 -> "native" objects, id = 0 -> created by server, id = 1 -> created by other clients)
	// Note that a fired projectile (initially with origin -1) will be assigned a value of 1 once leaving the client's screen.
	int origin;
	// Type of the object to be drawn
	DrawableObject object;

	// current position of the object in world coordinates
	float posX, posY, posZ;
	// current rotation of the object in degrees
	float rotX, rotY, rotZ;
	// size of the object in world space (fix values, are not calculated/changed)
	float sizeX, sizeY, sizeZ;
	// translation that is applied every frame
	float moveX, moveY, moveZ;
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
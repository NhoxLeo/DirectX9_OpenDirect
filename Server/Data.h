#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <stdio.h>
#include <conio.h>
// Settings that define how the server operates
#define WINSOCK_VERSION     MAKEWORD(2,2)
#define SERVER_COMM_PORT    27192
#define MAX_PACKET_SIZE     1024
#define MAX_USERS           16
#define MAX_LATE_FRAMES		10
enum Message
{
	MSG_LOGON,
	MSG_LOGOFF,
	MSG_UPDATEPLAYER,
	MSG_CONFIRMLOGON,
	MSG_PLAYERLOGGEDON,
	MSG_PLAYERLOGGEDOFF,
	MSG_PLAYERSHOOT,
	MSG_UPDATEOBJECT
};
struct MessageHeader
{
	Message MsgID;
};
struct LogOffMessage
{
	MessageHeader   Header;

	LogOffMessage() { Header.MsgID = MSG_LOGOFF; }
};
struct UpdatePlayerMessage
{
	MessageHeader   Header;
	DWORD           dwPlayerID;
	FLOAT			tick;
	FLOAT           fVelocity[3];
	FLOAT           fPosition[3];
	FLOAT           fRotation[3];
	FLOAT           fSize[3];
	BOOL			shoot;
	UpdatePlayerMessage() { Header.MsgID = MSG_UPDATEPLAYER; }
};
struct PlayerLoggedOffMessage
{
	MessageHeader   Header;
	DWORD           dwPlayerID;

	PlayerLoggedOffMessage() { Header.MsgID = MSG_PLAYERLOGGEDOFF; }
};
enum Action
{
	None,
	LogOn,
	LogOff,
	Move,
	Shoot,
	Dead,
	Create,
	UpdateObj,
	Destroy
};
enum ObjectType
{
	Player,
	Bullet,
	Brick,
};
struct Messenger
{
	int id;
	Action action;
	float position[3], rotation[3], size[3], velocity[3];
	Messenger()
	{
		id = 0;
		position[0] = position[1] = position[2] = 0;
		rotation[0] = rotation[1] = rotation[2] = 0;
		size[0] = size[1] = size[2] = 0;
		velocity[0] = velocity[1] = velocity[2] = 0;
	}
};
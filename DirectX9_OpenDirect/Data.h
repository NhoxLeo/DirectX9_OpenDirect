#pragma once
// Link header files required to compile the project
#include "stdafx.h"
#include <winsock2.h>   // Basic networking library
// Constants used in the program
#define DECRYPT_STREAM_SIZE 512                             /* File decryption byte window */
#define WINSOCK_VERSION     MAKEWORD(2,2)                   /* Use Winsock version 2.2 */
//#define SERVER_COMM_PORT    27192                           /* Connect to port 27192 */
#define SERVER_COMM_PORT    8888                           /* Connect to port 8888 */
//#define SERVER_COMM_PORT    3389                           /* Connect to port 3389 */
#define MAX_PACKET_SIZE     1024                            /* Largest packet is 1024 bytes */
#define MAX_USERS           16                              /* Maximum of 16 players */
#define UPDATE_FREQUENCY        10                          /* Update 10 times per second */
#define IDLE_UPDATE_FREQUENCY   2                           /* When idle, update twice a second */
struct OtherPlayer
{
	// Internal data
	BOOL bActive;
	D3DXVECTOR3 vRenderPos;
	FLOAT fRenderYaw;
	Sprite* entity;
};
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
struct LogOnMessage
{
	MessageHeader   Header;
	int id;
	float size[2];
	LogOnMessage() { Header.MsgID = Message::MSG_LOGON; size[0] = size[1] = 0; }
};
struct LogOffMessage
{
	MessageHeader Header;
	int id;
	LogOffMessage() { Header.MsgID = Message::MSG_LOGOFF; }
};
struct UpdateMessage
{
	MessageHeader   Header;
	DWORD           id;
	float			tick;
	float           fVelocity[3];
	//float           fPosition[3];
	float           fRotation[3];
	//float           fSize[3];
	bool			shoot;
	UpdateMessage() { Header.MsgID = MSG_UPDATEPLAYER; }
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
	SimpleObject,
	Player,
	Bullet,
	Brick,
};
struct Messenger
{
	int id;
	int inputSequenceNumber;
	unsigned long tick;
	Action action;
	ObjectType type;
	float position[2], rotation[2], size[2], velocity[2];
	Messenger()
	{
		tick = 0;
		inputSequenceNumber = 0;
		type = SimpleObject;
		id = 0;
		position[0] = position[1] = 0;
		rotation[0] = rotation[1] = 0;
		velocity[0] = velocity[1] = 0;
		size[0] = size[1] = 0;
	}
};
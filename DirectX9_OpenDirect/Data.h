#pragma once
// Link header files required to compile the project
#include "stdafx.h"
#include <winsock2.h>   // Basic networking library
#include <stdio.h>
// Constants used in the program
#define DECRYPT_STREAM_SIZE 512                             /* File decryption byte window */
#define WINSOCK_VERSION     MAKEWORD(2,2)                   /* Use Winsock version 2.2 */
#define SERVER_COMM_PORT    27192                           /* Connect to port 27192 */
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
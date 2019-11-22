#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <stdio.h>
#include <conio.h>
#include "user.h"

// Settings that define how the server operates
#define WINSOCK_VERSION     MAKEWORD(2,2)
#define SERVER_COMM_PORT    27192
#define MAX_PACKET_SIZE     1024
#define MAX_USERS           16

// Global variables used in the server program.  These variables are global because they are used
// by the server thread and initialized in the main thread.  It would be inefficient and
// duplicative to find an object-oriented workaround.
HANDLE g_hExitEvent;    // Set when the server shuts down
HANDLE g_hCommThread;   // Thread processing object
WSAEVENT g_hRecvEvent;  // Event triggered when data is received
SOCKET g_sSocket;       // Socket to send and receive on

// Global variables for client management
//HANDLE g_hUserSemaphore;    // Allows only a certain number of users to process simultaneously
User g_Users[MAX_USERS];    // List of all of the users

enum Message
{
	MSG_LOGON,
	MSG_LOGOFF,
	MSG_UPDATEPLAYER,
	MSG_CONFIRMLOGON,
	MSG_PLAYERLOGGEDON,
	MSG_PLAYERLOGGEDOFF,
};

struct MessageHeader
{
	Message MsgID;
};

struct LogOnMessage
{
	MessageHeader   Header;

	LogOnMessage() { Header.MsgID = MSG_LOGON; }
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
	FLOAT           fVelocity[3];       // Expressed in meters / second
	FLOAT           fPosition[3];
	DWORD           dwState;
	FLOAT           fYaw;

	UpdatePlayerMessage() { Header.MsgID = MSG_UPDATEPLAYER; }
};

struct ConfirmLogOnMessage
{
	MessageHeader   Header;

	ConfirmLogOnMessage() { Header.MsgID = MSG_CONFIRMLOGON; }
};

struct PlayerLoggedOnMessage
{
	MessageHeader   Header;
	DWORD           dwPlayerID;

	PlayerLoggedOnMessage() { Header.MsgID = MSG_PLAYERLOGGEDON; }
};
struct PlayerLoggedOff
{
	MessageHeader   Header;
	DWORD           dwPlayerID;

	PlayerLoggedOff() { Header.MsgID = MSG_PLAYERLOGGEDOFF; }
};
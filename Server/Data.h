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

/**
 * List of message IDs that are transacted with the server
 *   @author Karl Gluck
 */
enum Message
{
	MSG_LOGON,
	MSG_LOGOFF,
	MSG_UPDATEPLAYER,
	MSG_CONFIRMLOGON,
	MSG_PLAYERLOGGEDON,
	MSG_PLAYERLOGGEDOFF
};
/**
 * First structure in every message
 *   @author Karl Gluck
 */
struct MessageHeader
{
	Message MsgID;
};
/**
 * Message sent to let the server know we want to log on
 *   @author Karl Gluck
 */
struct LogOnMessage
{
	MessageHeader   Header;

	LogOnMessage() { Header.MsgID = MSG_LOGON; }
};
/**
 * Client wants to disconnect from the server
 *   @author Karl Gluck
 */
struct LogOffMessage
{
	MessageHeader   Header;

	LogOffMessage() { Header.MsgID = MSG_LOGOFF; }
};
/**
 * Sent between the server and client to update player information
 *   @author Karl Gluck
 */
struct UpdatePlayerMessage
{
	MessageHeader   Header;
	DWORD           dwPlayerID;
	FLOAT           fVelocity[3];
	FLOAT           fPosition[3];
	FLOAT           fRotation[3];
	FLOAT           fSize[3];
	UpdatePlayerMessage() { Header.MsgID = MSG_UPDATEPLAYER; }
};
/**
 * Sent by the server to tell the client that it has successfully logged on
 *   @author Karl Gluck
 */
struct ConfirmLogOnMessage
{
	MessageHeader   Header;

	ConfirmLogOnMessage() { Header.MsgID = MSG_CONFIRMLOGON; }
};
/**
 * Another player has logged off
 *   @author Karl Gluck
 */
struct PlayerLoggedOffMessage
{
	MessageHeader   Header;
	DWORD           dwPlayerID;

	PlayerLoggedOffMessage() { Header.MsgID = MSG_PLAYERLOGGEDOFF; }
};
struct PlayerLoggedOnMessage
{
	MessageHeader   Header;
	DWORD           dwPlayerID;

	PlayerLoggedOnMessage() { Header.MsgID = MSG_PLAYERLOGGEDON; }
};
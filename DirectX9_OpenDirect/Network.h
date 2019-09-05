#pragma once

//#define WIN32_LEAN_AND_MEAN         // Get rid of rarely-used stuff from Windows headers
//#define DIRECTINPUT_VERSION 0x800   // Tell DirectInput what version to use
//
//// Link header files required to compile the project
//#include "stdafx.h"
//#include <commctrl.h>   // Common controls for Windows (needed for the IP address control)
//#include <dinput.h>     // Include DirectInput information
//#include <iostream>     // Used for error reporting
//#include <WinSock2.h>
//
//// Constants used in the program
//#define DECRYPT_STREAM_SIZE 512                             /* File decryption byte window */
//#define BACKGROUND_COLOR    D3DCOLOR_XRGB( 128, 128, 255 )  /* Sky-blue background color */
//#define WINSOCK_VERSION     MAKEWORD(2,2)                   /* Use Winsock version 2.2 */
//#define SERVER_COMM_PORT    27192                           /* Connect to port 27192 */
//#define MAX_PACKET_SIZE     1024                            /* Largest packet is 1024 bytes */
//#define MAX_USERS           16                              /* Maximum of 16 players */
//#define UPDATE_FREQUENCY        10                          /* Update 10 times per second */
//#define IDLE_UPDATE_FREQUENCY   2                           /* When idle, update twice a second */

// When an error occurs, this has a value
//LPCSTR g_strError = NULL;

class Network
{
public:
	Network();
	//HRESULT InitializeWinsock(SOCKET * pSocket, HANDLE * pRecvEvent);
	//HRESULT ConnectToServer(SOCKET sSocket, HANDLE hRecvEvent);
	//DWORD GetServerAddress();
	~Network();
};
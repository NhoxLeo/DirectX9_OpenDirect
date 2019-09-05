#include "Precompiled.h"
#include "TestMapScene.h"

#define WIN32_LEAN_AND_MEAN         // Get rid of rarely-used stuff from Windows headers
#define DIRECTINPUT_VERSION 0x800   // Tell DirectInput what version to use

// Link header files required to compile the project
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "stdafx.h"
#include <WinSock2.h>
#include <commctrl.h>   // Common controls for Windows (needed for the IP address control)
//#include <dinput.h>     // Include DirectInput information
//#include <iostream>     // Used for error reporting

// Constants used in the program
#define DECRYPT_STREAM_SIZE 512                             /* File decryption byte window */
#define BACKGROUND_COLOR    D3DCOLOR_XRGB( 128, 128, 255 )  /* Sky-blue background color */
#define WINSOCK_VERSION     MAKEWORD(2,2)                   /* Use Winsock version 2.2 */
#define SERVER_COMM_PORT    27192                           /* Connect to port 27192 */
#define MAX_PACKET_SIZE     1024                            /* Largest packet is 1024 bytes */
#define MAX_USERS           16                              /* Maximum of 16 players */
#define UPDATE_FREQUENCY        10                          /* Update 10 times per second */
#define IDLE_UPDATE_FREQUENCY   2                           /* When idle, update twice a second */


enum Message
{
	MSG_LOGON,
	MSG_LOGOFF,
	MSG_UPDATEPLAYER,
	MSG_CONFIRMLOGON,
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
HRESULT InitializeWinsock(SOCKET * pSocket, HANDLE * pRecvEvent)
{
	// Start up Winsock
	{
		// Stores information about Winsock
		WSADATA wsaData;

		// Call the DLL initialization function
		if (SOCKET_ERROR == WSAStartup(WINSOCK_VERSION, &wsaData) ||
			wsaData.wVersion != WINSOCK_VERSION)
			return E_FAIL;
	}

	// Set up the main socket 
	SOCKET sSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// Create the event
	HANDLE hRecvEvent = WSACreateEvent();
	if (SOCKET_ERROR == WSAEventSelect(sSocket, hRecvEvent, FD_READ))
	{
		closesocket(sSocket);
		return E_FAIL;
	}

	// Store data
	*pSocket = sSocket;
	*pRecvEvent = hRecvEvent;

	// Success
	return S_OK;
}
INT_PTR CALLBACK IPAddressDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		// Set the default IP
		SendMessage(GetDlgItem(hDlg, IDC_IPADDRESS), IPM_SETADDRESS, 0, MAKEIPADDRESS(127, 0, 0, 1));

		// Store the IP address pointer
		SetWindowLong(hDlg, GWL_USERDATA, lParam);

	} break;

	// The user pressed a button
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			// Get the IP pointer
			LPDWORD pIp = (LPDWORD)GetWindowLong(hDlg, GWL_USERDATA);

			// Get the IP address
			if (4 == SendMessage(GetDlgItem(hDlg, IDC_IPADDRESS), IPM_GETADDRESS, 0, (LPARAM)pIp))
				EndDialog(hDlg, S_OK);
			else
				EndDialog(hDlg, E_FAIL);

		}break;

		case IDCANCEL:
		{
			// Close the dialog box
			EndDialog(hDlg, E_FAIL);

		}break;
		}
		break;

		// Message wasn't processed
	default:
		return FALSE;
	}

	return TRUE;
}
DWORD GetServerAddress()
{
	// This value is sent so that the IP can be stored
	DWORD dwIp;

	// Make sure that the IP address control is loaded
	INITCOMMONCONTROLSEX cex = { sizeof(cex), ICC_INTERNET_CLASSES };
	InitCommonControlsEx(&cex);

	// Pop a dialog box
	int ret = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SERVERIP),
		GetDesktopWindow(), IPAddressDlgProc, (LPARAM)&dwIp);

	// Return a result based on whether or not the user entered an IP
	if (ret == S_OK)
		return ntohl(dwIp);
	else
		return 0;
}
HRESULT ConnectToServer(SOCKET sSocket, HANDLE hRecvEvent)
{
	// Let the user enter the server's IP address
	DWORD dwAddr = GetServerAddress();
	if (dwAddr == 0)
		return E_FAIL;

	// Build a server address
	LPHOSTENT pTargetAddress = gethostbyaddr((CHAR*)&dwAddr, 4, AF_INET);
	SOCKADDR_IN addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_COMM_PORT);
	addr.sin_addr = *((LPIN_ADDR)*pTargetAddress->h_addr_list);

	// Send the log on message
	LogOnMessage packet;
	if (SOCKET_ERROR == sendto(sSocket, (CHAR*)&packet, sizeof(packet), 0,
		(LPSOCKADDR)&addr, sizeof(SOCKADDR_IN)))
		return E_FAIL;

	// Wait for a reply or for a few seconds to pass
	if (WAIT_TIMEOUT == WSAWaitForMultipleEvents(1, &hRecvEvent, TRUE, 3000, FALSE))
		return E_FAIL;

	// Recieve the result
	CHAR buffer[MAX_PACKET_SIZE];
	int length;
	SOCKADDR_IN src;
	ZeroMemory(&src, sizeof(src));
	int fromlen = sizeof(SOCKADDR_IN);
	if (SOCKET_ERROR == (length = recvfrom(sSocket, buffer, sizeof(buffer), 0, (LPSOCKADDR)&src, &fromlen)))
		return E_FAIL;

	// Connect to this address
	connect(sSocket, (LPSOCKADDR)&src, sizeof(SOCKADDR_IN));

	// Success
	return S_OK;
}

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{
	Application* app = Application::GetInstance();

	wchar_t* asd = (wchar_t*)L"Test";

	//app->Initialize(L"Universe2D.Sample1", 1280, 720, false);
	app->Initialize(asd, 640, 480, false);

	Director::GetInstance()->SetScene(TestMapScene::Create());

	app->Run();
	app->Release();
}
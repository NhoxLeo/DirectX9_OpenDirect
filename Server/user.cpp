
#include "user.h"

User::User()
{
	m_hRecvEvent = NULL;
	m_sSocket = INVALID_SOCKET;
	m_hThread = NULL;
}
User::~User()
{
	Destroy();
}
HRESULT User::Create(DWORD dwId, WORD wPort)
{
	// We are not connected
	m_bConnected = FALSE;

	// Store the ID number
	m_dwId = dwId;

	// Create the termination event
	m_hDisconnectEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Create the socket to recieve and send data on
	if (INVALID_SOCKET == (m_sSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)))
	{
		Destroy();
		return E_FAIL;
	}

	// Create the address to bind to
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(wPort);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;

	// Bind it to a port
	if (SOCKET_ERROR == bind(m_sSocket, (LPSOCKADDR)&addr, sizeof(SOCKADDR_IN)))
	{
		Destroy();
		return E_FAIL;
	}

	// Create the event
	m_hRecvEvent = WSACreateEvent();
	if (SOCKET_ERROR == WSAEventSelect(m_sSocket, m_hRecvEvent, FD_READ))
	{
		Destroy();
		return E_FAIL;
	}

	// Successful initialization
	return true;
}
VOID User::Destroy()
{
	// If the program is still running, tell it to shut off and wait for the thread to close
	if (m_sSocket != INVALID_SOCKET)
	{
		// Disconnect the user
		Disconnect();

		// Wait for the thread to close
		WaitForSingleObject(m_hThread, INFINITE);
	}

	if (m_hDisconnectEvent != NULL)
	{
		CloseHandle(m_hDisconnectEvent);
		m_hDisconnectEvent = NULL;
	}

	if (m_hRecvEvent != NULL)
	{
		WSACloseEvent(m_hRecvEvent);
		m_hRecvEvent = NULL;
	}

	if (m_sSocket != INVALID_SOCKET)
	{
		closesocket(m_sSocket);
		m_sSocket = INVALID_SOCKET;
	}

	if (m_hThread != NULL)
	{
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
}
DWORD User::GetId()
{
	return m_dwId;
}
HRESULT User::Connect(const SOCKADDR_IN * pAddress, LPTHREAD_START_ROUTINE lpProcAddress)
{
	// Connect the socket so that it only receives from this address
	connect(m_sSocket, (LPSOCKADDR)pAddress, sizeof(SOCKADDR_IN));

	// Create the client thraed
	if (NULL == (m_hThread = CreateThread(NULL, 0, lpProcAddress, (LPVOID)this, 0, NULL)))
		return E_FAIL;

	// Make the disconnect event non-triggered
	ResetEvent(m_hDisconnectEvent);

	// We are now connected
	m_bConnected = TRUE;

	// Success
	return S_OK;
}
VOID User::Disconnect()
{
	// Simply set the disconnect event.  We wait for the thread to exit before finally
	// disconnecting for real.
	SetEvent(m_hDisconnectEvent);
}
VOID User::ThreadFinished()
{
	// Get rid of the target address
	SOCKADDR_IN addr;
	memset(&addr, 0, sizeof(SOCKADDR_IN));
	connect(m_sSocket, (LPSOCKADDR)&addr, sizeof(SOCKADDR_IN));

	// The user is no longer connected
	m_bConnected = FALSE;
}
BOOL User::IsConnected()
{
	return m_bConnected;
}
EndWaitResult User::WaitForPackets(DWORD dwTimeout)
{
	HANDLE hEvents[] = { m_hRecvEvent, m_hDisconnectEvent };

	// Call the wait function to see what events have been triggered
	switch (WSAWaitForMultipleEvents(2, hEvents, FALSE, dwTimeout, FALSE))
	{
	case WAIT_OBJECT_0 + 0: return EWR_RECVPACKET;
	case WAIT_OBJECT_0 + 1: return EWR_DISCONNECT;
	default:                return EWR_TIMEOUT;
	}
}
int User::SendPacket(const CHAR * pBuffer, int length)
{
	// We can use "send" even though this is a UDP connection because we 'connected' the socket.
	// This doesn't make it reliable, but does make it send and recieve only to one address.
	return send(m_sSocket, pBuffer, length, 0);
}
int User::RecvPacket(char * pBuffer, int length)
{
	// We have to reset the receive event otherwise Winsock forgets to do this...
	ResetEvent(m_hRecvEvent);
	// Receive messages
	return recv(m_sSocket, pBuffer, length, 0);
}
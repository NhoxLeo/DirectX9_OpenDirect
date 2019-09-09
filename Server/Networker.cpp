#include "Networker.h"

// Creates the socket that calls to the Networker's functions are forwarded to.
// Returns NW_OK if the creation of the socket was successful, NW_ERROR otherwise.
NetworkerReturnCode Networker::setup()
{
	WSADATA WsaDat;

	// Initialise Windows Sockets
	if (WSAStartup(MAKEWORD(2, 2), &WsaDat) != 0)
	{
		return NW_ERROR;
	}

	// Create the socket
	socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_ == INVALID_SOCKET)
	{
		return NW_ERROR;
	}

	// socket setup successful
	return NW_OK;
}

// Creates a connection to a server with the ip address provided as the first parameter on the port specified as second parameter.
// Returns NW_OK if the connection was successfully established, NW_ERROR otherwise.
NetworkerReturnCode Networker::connectToServer(const char* ip, unsigned short port)
{
	// Resolve IP address for hostname.
	struct hostent *host;

	// Setup client socket address structure.
	SOCKADDR_IN SockAddr;
	SockAddr.sin_port = htons(port);	// Port number
	SockAddr.sin_family = AF_INET;		// Use the Internet Protocol (IP)

	// depending on value of parameter, select localhost or an ip address
	if (strcmp(ip, "localhost") == 0)
	{
		if ((host = gethostbyname("localhost")) == NULL)
		{
			return NW_ERROR;
		}
		SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);
	}
	else
	{
		// turn IP address from string into a form that is usable by Winsock 
		SockAddr.sin_addr.s_addr = inet_addr(ip);
	}

	// Attempt to connect to server...

	if (connect(socket_, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0)
	{
		return NW_ERROR;
	}

	// connection successfully established
	return NW_OK;
}

// Places the underlying socket in either non-blocking (parameter evaluates to true) or blocking mode (parameter evaluates to false)
// Returns NW_OK if the socket was successfully placed in the desired mode, NW_ERROR otherwise
NetworkerReturnCode Networker::setNonBlockingMode(bool isNonBlocking)
{
	u_long iMode;
	if (isNonBlocking)
	{
		// place the socket in non-blocking mode
		u_long iMode = 1;
	}
	else
	{
		// place the socket in blocking mode
		u_long iMode = 0;
	}

	if (ioctlsocket(socket_, FIONBIO, &iMode) == SOCKET_ERROR)
	{
		return NW_ERROR;
	}

	// socket was successfully placed in the desired mode
	return NW_OK;
}

// Closes and shuts down the underlying socket and cleans up winsock. Afterwards no more communication through this socket possible,
// unless set up anew.
// Returns NW_OK if the socket was shut down successfully.
NetworkerReturnCode Networker::shutDown()
{
	// Shutdown and close the client socket.
	if (shutdown(socket_, SD_BOTH) == SOCKET_ERROR)
	{
		return NW_ERROR;
	}

	if (closesocket(socket_) == SOCKET_ERROR)
	{
		return NW_ERROR;
	}

	// socket was closed successfully
	return NW_OK;
}

// Terminates use of the winsock library for all threads and should therefore only be called by the last thread using winsock.
NetworkerReturnCode Networker::cleanUp()
{
	// Cleanup Winsock
	if (WSACleanup() == SOCKET_ERROR)
	{
		return NW_ERROR;
	}
	else
	{
		return NW_OK;
	}
}

// Binds the underlying socket to the ip address and port specified as parameters.
// Providing "any" as value for the ip address, will bind the socket to any ip address on the specified port.
// Returns NW_OK if the socket was successfully bound to the specified ip and port, NW_ERROR otherwise.
NetworkerReturnCode Networker::bind(const char* ip, unsigned short port)
{
	SOCKADDR_IN SockAddr;

	SockAddr.sin_family = AF_INET;
	if (strcmp(ip, "any") == 0)
	{
		// bind the socket to any incoming IP address
		SockAddr.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		SockAddr.sin_addr.s_addr = inet_addr(ip);
	}
	SockAddr.sin_port = htons(port);

	// global scope for bind to avoid ambiguity with bind in Networker
	if (::bind(socket_, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) == SOCKET_ERROR)
	{
		return NW_ERROR;
	}

	// the socket was successfully bound
	return NW_OK;
}

// Places the underlying socket in listening state.
// The parameter specifies how many connections can be added to the queue of pending connections.
// Returns NW_OK if the socket was successfully placed in listening state, NW_ERROR otherwise.
NetworkerReturnCode Networker::startListening(int sizeOfPendingQueue)
{
	// start listening for connections
	if (listen(socket_, sizeOfPendingQueue) == SOCKET_ERROR)
	{
		return NW_ERROR;
	}

	// socket successully placed in listening state
	return NW_OK;
}

// Accepts a connection with an entity trying to connect to this networker. The Networker has to be in listening state to be able
// to accept connections.
// The function returns an instance of the Networker class, that can be used to communicate with the just connected client.
// Given a Networker in non-blocking mode, the function will return an invalid Networker if there are no clients trying to connect.
// The validity of the returned Networker has to be checked by the user by calling isValid() on the returned object.
Networker Networker::acceptConnection()
{
	SOCKET tempSock = INVALID_SOCKET;
	tempSock = accept(socket_, NULL, NULL);

	if (tempSock == INVALID_SOCKET)
	{
		return Networker(INVALID_SOCKET);
	}
	else
	{
		return Networker(tempSock);
	}
}

// Can be used to determine the status of the socket in terms of reading data from it. The function will wait for a specified
// amount of time or until there is some data readable from the socket.
// Returns NW_OK if there is something to be received on the socket.
// Returns NW_TIMEOUT if the time specified in the parameter passed without data becoming available for reading from the socket
// Returns NW_ERROR if an error occurs.
NetworkerReturnCode Networker::selectRead(long milliseconds)
{
	// setup the structure holding the timeout interval
	timeval tVal;
	tVal.tv_sec = milliseconds / 1000;
	tVal.tv_usec = (milliseconds % 1000) * 1000;

	// setup fd set with the socket of this Networker
	fd_set fdRead;
	FD_ZERO(&fdRead);
	FD_SET(socket_, &fdRead);

	int result = SOCKET_ERROR;

	// returns as soon as there is something to read or the time specified in tVal
	result = select(0, &fdRead, 0, 0, &tVal);

	if (result == SOCKET_ERROR)
	{
		// an error occurred
		return NW_ERROR;
	}
	else if (result == 0)
	{
		// a timeout occurred as there was nothing to receive within the specified time interval
		return NW_TIMEOUT;
	}
	else
	{
		// there is something to be received on the socket
		return NW_OK;
	}
}

// Returns true if the Networker object is valid and be used to communicate with a client, false otherwise
bool Networker::isValid()
{
	return socket_ != INVALID_SOCKET;
}

Networker::Networker() : socket_(INVALID_SOCKET)
{
}

Networker::Networker(SOCKET newSocket) : socket_(newSocket)
{
}

// Returns the IP addresses of the local machine as a list of strings
// If the returned list is empty, the function was not able to obtain the local IP addresses
list<string> Networker::getLocalIPs()
{
	list<string> ipAddresses;

	char ac[80];
	if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR)
	{
		return ipAddresses;
	}

	struct hostent *phe = gethostbyname(ac);
	if (phe == 0)
	{
		return ipAddresses;
	}

	for (int i = 0; phe->h_addr_list[i] != 0; ++i)
	{
		struct in_addr addr;
		memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
		ipAddresses.push_back(inet_ntoa(addr));
	}

	return ipAddresses;
}
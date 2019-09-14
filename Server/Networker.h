#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <list>
#include <string>

using namespace std;

// Return codes used by the functions of the Networker to indicate error and success to the user.
enum NetworkerReturnCode
{
	NW_OK, NW_ERROR, NW_NOTHING_TO_SEND, NW_NOTHING_TO_RECEIVE, NW_CONNECTION_CLOSED, NW_TIMEOUT
};

// The networker class basically wraps a winsock socket and provides networking operations that will carry out corresponding
// operations on the underlying socket. By using a networker, other objects do not have to know anything about Winsock's
// specific implementation of these functions. Therefore, it may also be possible to replace Winsock
// with some other networking library with changes only being necessary to the Networker.
// Although not necessary for this application, it might be a good idea to create an abstract base class "Networker" and to have
// i.e. a "WinsockNetworker" inherit from it providing implementations for the abstract functions.
class Networker
{
public:
	Networker();

	NetworkerReturnCode setup();
	NetworkerReturnCode setNonBlockingMode(bool isNonBlocking);
	NetworkerReturnCode shutDown();

	template<typename T>
	NetworkerReturnCode send(T* data, int dataSize);
	template<typename T>
	NetworkerReturnCode receive(T* buffer, int bufferSize);

	NetworkerReturnCode connectToServer(const char* ip, unsigned short port);
	NetworkerReturnCode bind(const char* ip, unsigned short port);
	NetworkerReturnCode startListening(int sizeOfPendingQueue);
	Networker acceptConnection();
	NetworkerReturnCode selectRead(long milliseconds);

	bool isValid();

	static list<string> getLocalIPs();
	static NetworkerReturnCode cleanUp();
private:
	SOCKET socket_;
	Networker(SOCKET newSocket);
};

// Sends data of Type T and specified length (in bytes, second parameter) starting at the address provided as first parameter 
// into the underlying socket.
// If no error occurs, the function is guaranteed to send the whole data, even if it takes several calls to the underlying winsock
// send function. The function will return NW_OK to indicate success.
// If the Networker (and the underlying socket) are placed in non-blocking mode, the function will return NW_NOTHING_TO_SEND if there
// is no data available at the buffer to be sent.
// If an error occurs, the function will return NW_ERROR.
template<typename T>
NetworkerReturnCode Networker::send(T* buffer, int bufferSize)
{
	int totalBytesSent = 0;
	int bytesSent = 0;

	// make sure everything is sent
	while (totalBytesSent < bufferSize)
	{
		// returns 0 if connection closed, SOCKET_ERROR when error (specific error code can be retrieved by calling WSAGetLastError)
		bytesSent = ::send(socket_, reinterpret_cast<const char*>(buffer) + totalBytesSent, bufferSize - totalBytesSent, 0);
		if (bytesSent == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				return NW_NOTHING_TO_SEND;
			}
			else
			{
				return NW_ERROR;
			}
		}
		totalBytesSent += bytesSent;
	}
	// everything went fine, complete data sent
	return NW_OK;
}

// Receives data of Type T and specified length (in bytes, second parameter) from the underlying socket and writes it to the buffer
// specified by the first parameter
// If no error occurs, the function is guaranteed to receive the whole data, even if it takes several calls to the underlying winsock
// recv function. The function will return NW_OK to indicate success.
// If the Networker (and the underlying socket) are placed in non-blocking mode, the function will return NW_NOTHING_TO_RECEIVE if there
// is no data available from the socket to be received.
// If the connection has been closed, the function will return NW_CONNECTION_CLOSED
// If an error occurs, the function will return NW_ERROR.
template<typename T>
NetworkerReturnCode Networker::receive(T* buffer, int bufferSize)
{
	int totalBytesReceived = 0;
	int bytesReceived = 0;

	// make sure everything is received
	while (totalBytesReceived < bufferSize)
	{
		// returns 0 if connection closed, SOCKET_ERROR when error (specific error code can be retrieved by calling WSAGetLastError)
		bytesReceived = ::recv(socket_, reinterpret_cast<char*>(buffer) + totalBytesReceived, bufferSize - totalBytesReceived, 0);

		if (bytesReceived == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				return NW_NOTHING_TO_RECEIVE;
			}
			else
			{
				return NW_ERROR;
			}
		}
		else if (bytesReceived == 0)
		{
			return NW_CONNECTION_CLOSED;
		}
		totalBytesReceived += bytesReceived;
	}
	// everything went fine, complete data received
	return NW_OK;
}


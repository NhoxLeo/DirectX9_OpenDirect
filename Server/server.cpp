#include "Server.h"


Server::Server(void)
{
}

// start the server, create a listen socket and start the worker thread
// returns 0 if everything was setup successfully, -1 otherwise
int Server::start(void)
{
	if (networker.setup() != NW_OK)
	{
		cout << "\nSetting up the networker failed.";
		return -1;
	}
	// the two arguments could be obtained by asking for user input but are, in this case, left to these defaut values
	if (networker.bind("any", 8888) != NW_OK)
	{
		cout << "\nBinding the networker failed.";
		return -1;
	}
	if (networker.setNonBlockingMode(true) != NW_OK)
	{
		cout << "\nSetting the networker to non-blocking mode failed.";
		return -1;
	}
	if (networker.startListening(4) != NW_OK)
	{
		cout << "\nPlacing the networker in listening mode failed.";
		return -1;
	}
	else
	{
		// start a thread that will process clients connecting to the server
		sharedData.doRun = true;
		workerThread = thread(ProcessClientsFunctor(), &sharedData);

		// Determine the IP address of the machine the server is running on and display it on the console.
		// (as the machine may have several IP addresses associated to it, all of them will be displayed and it is up to
		// the user to choose the correct one when starting a client)
		// This is just for convenience (if it doesn't work, ipconfig has to be used to obtain server's IP)
		list<string> ipAddresses = Networker::getLocalIPs();
		if (!ipAddresses.empty())
		{
			cout << "\nIP addresses associated to this machine:";
			for (list<string>::iterator it = ipAddresses.begin(); it != ipAddresses.end(); ++it)
			{
				cout << "\n - " << (*it);
			}
			cout << endl;
		}

		cout << "\nThis machine is now running the server side of the application.\nYou can now run the 'Client.exe' on this computer or another one in order to\ncreate a client and connect to the server.";
		cout << "\nYou can terminate the server at any time by closing this console window.\nThis will also lead to the termination of all clients currently connected\nto the server." << endl;
		cout << "\nWaiting for incoming connections...";

		return 0;
	}
}

// listen for and accept new connections
void Server::run(void)
{
	Networker newNetworker;

	newNetworker = networker.acceptConnection();

	if (newNetworker.isValid())
	{
		if (newNetworker.setNonBlockingMode(true) == NW_OK)
		{
			// make sure no other thread is currently accessing the list for new networkers
			lock_guard<std::mutex> lock(sharedData.newSocketsMutex);

			// add the new networker to the container temporarily storing networkers of new clients
			sharedData.newNetworkers.push(newNetworker);
		}
		cout << "\nNew client connected.";
	}
}

Server::~Server(void)
{
	// shut down worker thread
	sharedData.doRun = false;

	// wait for the worker thread to terminate
	workerThread.join();

	// shut down and close the networker (listening socket)
	networker.shutDown();

	// clean up any ressources left
	Networker::cleanUp();

	cout << "\nServer terminated.";
}


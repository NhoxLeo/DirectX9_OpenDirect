#include "Server.h"

// global flag that is set when the user attempts to close the console window of the server
bool g_isClosing = false;

int main()
{
	BOOL ConsoleCtrlHandler(DWORD eventType);

	// create a server object
	Server server;

	// start the server and check if startup succeeded
	if (server.start() == 0)
	{
		// register a new console handler to be able to determine when the user closes the console
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleCtrlHandler, TRUE);

		// as long as the user doesn't close the console, keep the server running (listening to new connections)
		while (!g_isClosing)
		{
			server.run();
		}
	}

	// the server object's destructor will be called automatically
	return 0;
}

// handles the close event for the console and passes other events to the next handler
BOOL ConsoleCtrlHandler(DWORD eventType)
{
	if (eventType == CTRL_CLOSE_EVENT)
	{
		g_isClosing = true;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


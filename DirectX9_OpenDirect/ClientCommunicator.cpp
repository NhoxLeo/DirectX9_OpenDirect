#include "ClientCommunicator.h"

// initialize the constants
const float ClientCommunicator::FLIGHT_HEIGHTS[] = { -6.5f, -1.5f, 2.5f, 7.0f, 11.5f };
const int ClientCommunicator::MAX_NUMBER_OF_SPACESHIPS = 5;
const long ClientCommunicator::TIMEOUT_INTERVAL = 5000;

ClientCommunicator::ClientCommunicator(void)
{
	// initialize flightTracks
	for (int i = 0; i < MAX_NUMBER_OF_SPACESHIPS; ++i)
	{
		flightTracks.insert(pair<float, int>(FLIGHT_HEIGHTS[i], -1));
	}

	// seed the random number generator using the system time
	srand(static_cast<unsigned int>(time(NULL)));
}

ClientCommunicator::~ClientCommunicator(void)
{
}

void ClientCommunicator::operator()(ServerSharedData* sharedData)
{
	// Used to trigger initial spawning of space ships. Will be set to true when a client joins if there has previously been no active 
	// client and will be set to false after spawning is complete. Regular spawning (when ships are destroyed) is handled elsewhere
	// and is not depending on this variable
	bool doSpawn = false;
	ServerCommand command = DoNothing;
	ClientMessage message = None;

	// process the clients as long as the main thread doesn't change the shared flag 'doRun' signalling shut down of the server
	while (sharedData->doRun)
	{
		command = DoNothing;
		message = None;

		//-------------------------------------------------------------------------------------------
		// create new clients if the main thread added some new networkers to the queue

		{
			// if new networkers connected to the server, create a corresponding client data structure and add it to the vector of active clients
			lock_guard<std::mutex> lock(sharedData->newSocketsMutex);
			while (!(sharedData->newNetworkers.empty()))
			{
				// create a Client data structure for the new client
				Client client;
				client.id = getClientId();
				client.networker = sharedData->newNetworkers.front();
				client.dataReady = false;
				client.lastMessageProcessed = true;

				// send the id to the new client and make sure it is received

				NetworkerReturnCode sendIdResult = NW_NOTHING_TO_SEND;
				do
				{
					sendIdResult = client.networker.send<int>(&client.id, sizeof(int));
				} while (sendIdResult == NW_NOTHING_TO_SEND);

				if (sendIdResult == NW_OK)
				{
					// id was sent successfully, now wait for the Ack from the client
					NetworkerReturnCode selectResult = client.networker.selectRead(TIMEOUT_INTERVAL);
					switch (selectResult)
					{
					case NW_OK:
					{
						// receive the client's acknowledgement
						message = None;
						NetworkerReturnCode recvAckResult = client.networker.receive<ClientMessage>(&message, sizeof(ClientMessage));
						if (recvAckResult == NW_OK && message == ClientAck)
						{
							// the client received the id and answered with an acknowledgement -> add to active clients
							clients.push_back(client);

							if (clients.size() == 1)
							{
								// if this client is the first one to be added to an empty vector of active clients, make sure
								// that new ships will be spawned
								doSpawn = true;
							}
						}
						else
						{
							// An error occurred, shut down the client's networker. This will lead to the client terminating itself
							client.networker.shutDown();
						}
						break;
					}
					case NW_TIMEOUT:
						// Ack wasn't received in time, terminate client
						client.networker.shutDown();
						break;
					case NW_ERROR:
						// an error occurred, terminate the client
						client.networker.shutDown();
						break;
					}
				}
				else
				{
					// an error occurred, terminate the client
					client.networker.shutDown();
				}
				// either way, remove the client from the waiting queue
				sharedData->newNetworkers.pop();
			}
		}

		// only continue if there are active clients
		if (clients.size() != 0)
		{
			//-----------------------------------------------------------------------------------------------------
			// Spawn new space ships

			if (doSpawn)
			{
				// create the maximal number of new space ships and make sure they are spawned at the appropriate client
				for (int i = 0; i < MAX_NUMBER_OF_SPACESHIPS; ++i)
				{
					// create space ship drawing data that will be sent to clients
					DrawingData data;
					createDrawingData(SpaceShip, data);

					Client* targetClient = nullptr;

					// Determine the client at which to spawn the ships (depending on the direction the ships are moving into)
					if (data.moveX > 0)
					{
						// space ships flying to the right start on the first client's screen
						targetClient = &(clients[0]);
					}
					else
					{
						// space ships flying to the left start on the last client's screen
						targetClient = &(clients[clients.size() - 1]);
					}

					// Create a new action to be executed by the chosen client
					Action newAction2;
					newAction2.command = Draw;
					newAction2.data = data;

					// target client will execute queued actions when it is his turn
					(*targetClient).actions.push(newAction2);

					// Keep track of created space ships and the height they are flying on.
					// If the key does not exist a new pair will be created and inserted into the map (first pass of this function)
					flightTracks[data.posY] = data.id;
				}
				// initial spawn finished
				doSpawn = false;
			}

			//-------------------------------------------------------------------------------------------------
			// Process clients

			// iterate through all active clients
			vector<Client>::iterator it = clients.begin();
			while (it != clients.end())
			{
				// gets set if the current socket should be erased as consequence to an error occurring 
				// (erase will be executed at the end of the block)
				bool doErase = false;

				//----------------------------------------------------------------------------------------
				// Receive data from client

				// If the client sent some data, get it now
				if ((*it).dataReady)
				{
					// make sure to receive the client's message
					message = None;
					NetworkerReturnCode recvMsgResult = NW_NOTHING_TO_RECEIVE;
					do
					{
						recvMsgResult = (*it).networker.receive<ClientMessage>(&message, sizeof(ClientMessage));
					} while (recvMsgResult == NW_NOTHING_TO_RECEIVE);

					if (recvMsgResult == NW_OK)
					{
						// acknowledge the receipt of the message
						NetworkerReturnCode sendCommandResult = NW_NOTHING_TO_SEND;
						command = ServerAck;
						do
						{
							sendCommandResult = (*it).networker.send<ServerCommand>(&command, sizeof(ServerCommand));
						} while (sendCommandResult == NW_NOTHING_TO_SEND);

						if (sendCommandResult == NW_OK)
						{
							// Ack successfully sent, now receive data
							switch (message)
							{
							case ObjectMoved:
							{
								// the client has notfied that an object he is drawing has just reached the end of the screen and has to be
								// passed to an adjacent client

								// wait to receive the data of the object to be moved
								NetworkerReturnCode selectResult = (*it).networker.selectRead(TIMEOUT_INTERVAL);
								if (selectResult == NW_OK)
								{
									// receive the drawing data of the object
									DrawingData rcvData;
									NetworkerReturnCode recvDataResult = (*it).networker.receive<DrawingData>(&rcvData, sizeof(DrawingData));

									if (recvDataResult == NW_OK)
									{
										// acknowledge the receipt of the data
										NetworkerReturnCode sendCommandResult = NW_NOTHING_TO_SEND;
										command = ServerAck;
										do
										{
											sendCommandResult = (*it).networker.send<ServerCommand>(&command, sizeof(ServerCommand));
										} while (sendCommandResult == NW_NOTHING_TO_SEND);

										if (sendCommandResult == NW_OK)
										{
											// move the object to the appropriate client
											moveObject(*it, rcvData);
										}
										else
										{
											// an error occurred
											shutDownClient(*it);
											doErase = true;
										}
									}
									else
									{
										// an error occurred
										shutDownClient(*it);
										doErase = true;
									}

								}
								else if (selectResult == NW_TIMEOUT)
								{
									// didn't receive data from client in time
									shutDownClient(*it);
									doErase = true;
								}
								else
								{
									// an error occurred
									shutDownClient(*it);
									doErase = true;
								}
								// indicate that the last message sent by that client was processed and that the client can send the
								// next one
								(*it).lastMessageProcessed = true;
								break;
							}
							case ObjectDestroyed:
							{
								// a space ship has been destroyed by a projectile 

								// get the data of the object that has been destroyed
								DrawingData rcvData;
								NetworkerReturnCode selectResult = (*it).networker.selectRead(TIMEOUT_INTERVAL);
								if (selectResult == NW_OK)
								{
									NetworkerReturnCode recvDataResult = (*it).networker.receive<DrawingData>(&rcvData, sizeof(DrawingData));
									if (recvDataResult == NW_OK)
									{
										// acknowledge the receipt of the data
										NetworkerReturnCode sendCommandResult = NW_NOTHING_TO_SEND;
										command = ServerAck;
										do
										{
											sendCommandResult = (*it).networker.send<ServerCommand>(&command, sizeof(ServerCommand));
										} while (sendCommandResult == NW_NOTHING_TO_SEND);

										if (sendCommandResult == NW_OK)
										{
											// destroy the object
											destroyObject(*it, rcvData);
										}
										else
										{
											// an error occurred
											shutDownClient(*it);
											doErase = true;
										}
									}
									else
									{
										// an error occurred while trying to receive the data of the destroyed object
										shutDownClient(*it);
										doErase = true;
									}
								}
								else if (selectResult == NW_TIMEOUT)
								{
									// timeout while trying to receive the data of the object that was destroyed
									shutDownClient(*it);
									doErase = true;
								}
								else
								{
									// an error occurred
									shutDownClient(*it);
									doErase = true;
								}
								// indicate that the last message sent by that client was processed and that the client can send the
								// next one
								(*it).lastMessageProcessed = true;
								break;
							}
							}
						}
						else
						{
							// an error occurred
							shutDownClient(*it);
							doErase = true;
						}


					}
					else if (recvMsgResult == NW_ERROR)
					{
						// terminate the client
						shutDownClient(*it);
						doErase = true;
					}
					else if (recvMsgResult == NW_CONNECTION_CLOSED)
					{
						// terminate the client
						shutDownClient(*it);
						doErase = true;
					}
				}
				//-------------------------------------------------------------------------------------
				// Process the client's queue of actions

				// only process the action queue if there was no error and the client won't be erased
				while ((!(*it).actions.empty()) && (!doErase))
				{
					// send the action to the client
					if (sendToClient(*it, (*it).actions.front()))
					{
						// make sure to update the client's space ship list according to the action
						if ((*it).actions.front().data.object == SpaceShip)
						{
							if ((*it).actions.front().command == Draw)
							{
								(*it).spaceShips.push_back((*it).actions.front().data);
							}
							else
							{
								list<DrawingData>::iterator it1 = find((*it).spaceShips.begin(), (*it).spaceShips.end(), (*it).actions.front().data);
								if (it1 != (*it).spaceShips.end())
								{
									(*it).spaceShips.erase(it1);
								}
							}
						}
						(*it).actions.pop();
					}
					else
					{
						// an error occurred during sending of the action's data to the client
						shutDownClient(*it);
						doErase = true;
					}
				}


				//------------------------------------------------------------------------------------
				// Tell the client to send new data (in case the last received one has been processed)

				// only do this if the client has not been shut down already/will be erased
				if ((!(doErase)) && ((*it).lastMessageProcessed))
				{
					// tell the client to send new data/message if available
					command = Send;

					NetworkerReturnCode sendGetResult = NW_NOTHING_TO_SEND;
					do
					{
						sendGetResult = (*it).networker.send<ServerCommand>(&command, sizeof(ServerCommand));
					} while (sendGetResult == NW_NOTHING_TO_SEND);

					if (sendGetResult == NW_OK)
					{
						// command was sent successfully, now wait for the Ack from the client
						NetworkerReturnCode selectResult = (*it).networker.selectRead(TIMEOUT_INTERVAL);
						switch (selectResult)
						{
						case NW_OK:
						{
							message = None;
							NetworkerReturnCode recvAckResult = (*it).networker.receive<ClientMessage>(&message, sizeof(ClientMessage));
							if (recvAckResult == NW_OK && (message == ClientAck || message == DataReady))
							{
								// client acknowledged receipt of the command (returns DataReady to ack and tell that he has data to send;
								// make sure not to issue another 'Get' command before a client message has been received and
								// processed
								if (message == DataReady)
								{
									(*it).dataReady = true;
									(*it).lastMessageProcessed = false;
								}
								else
								{
									(*it).dataReady = false;
								}
							}
							else
							{
								// An error occurred, shut down the client
								shutDownClient(*it);
								doErase = true;
							}
							break;
						}
						case NW_TIMEOUT:
							// Ack wasn't received in time, terminate client
							shutDownClient(*it);
							doErase = true;
							break;
						case NW_ERROR:
							// an error occurred, terminate the client
							shutDownClient(*it);
							doErase = true;
							break;
						}
					}
					else
					{
						// an error occurred, terminate the client
						shutDownClient(*it);
						doErase = true;
					}
				}

				//---------------------------------------------------------------------------------------------
				// Erase the client if an error occured during processing and move on to the next active client

				if (doErase)
				{
					std::cout << "\nClient disconnected.";

					// avoids possible invalidation of iterators
					it = clients.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
	}

	//-----------------------------------------------------------------------------------------
	// Terminate the thread

	// main thread has issued termination of the worker thread;
	// shut down the networker of all clients
	for (vector<Client>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		// no explicit 'Disconnect' message required as the clients will shut down themselves as soon as they notice that their
		// networker has been shut down
		(*it).networker.shutDown();
	}

	// delete all clients
	clients.clear();

	// WSACleanup() will be executed by main thread
}

// Included for convenience and to keep the code of the operator() cleaner.
// Moves the object specified by the second parameter from the client specified by the first parameter to an appropriate
// target client.
void ClientCommunicator::moveObject(Client& client, DrawingData& data)
{
	// determine position of the source client
	unsigned int index = distance(clients.begin(), find(clients.begin(), clients.end(), client));

	// determine the socket that the just received object will be sent to (depends of flying direction
	// of the object)
	Client* targetClient;
	if (clients.size() > 1)
	{
		if (data.moveX > 0)
		{
			// object is flying to the right -> target client will be the next in the vector
			if (clients.size() > index + 1)
			{
				targetClient = &(clients[index + 1]);
			}
			else
			{
				// source client is last one in the vector -> choose the first one in the vector
				targetClient = &(clients[0]);
			}
		}
		else
		{
			// object is flying to the left -> target client will be the previous one in the vector
			if (index > 0)
			{
				targetClient = &(clients[index - 1]);
			}
			else
			{
				// source client is first one in the vector -> choose the last one in the vector
				targetClient = &(clients[clients.size() - 1]);
			}
		}
	}
	else
	{
		// if there is only one active client -> source client == target client
		targetClient = &(clients[index]);
	}

	// if the moved object is a space ship, make sure to update the space ship list of the client
	if (data.object == SpaceShip)
	{
		client.spaceShips.erase(find(client.spaceShips.begin(), client.spaceShips.end(), data));
	}

	// add drawing of the moved object to the action queue of the target client
	Action newAction;
	newAction.command = Draw;
	newAction.data = data;
	(*targetClient).actions.push(newAction);
}

// Included for convenience and to keep the code of the operator() cleaner.
// Destroys the object specified by the second parameter that was destroyed on the screen of the client specified by the first 
// parameter.
void ClientCommunicator::destroyObject(Client& client, DrawingData& data)
{
	// check if object was already notified as destroyed (just in case a space ship is hit simultaneously
	// by two projectiles or some similar very rare occurrence)
	if (flightTracks[data.posY] == data.id)
	{
		// update flightTracks structure -> currently no ship on this flight height
		flightTracks[data.posY] = -1;

		// determine index of source client
		int index = distance(clients.begin(), find(clients.begin(), clients.end(), client));

		// notify the adjacent clients of the object's destruction (it might be possible that one of them is already
		// partially drawing it if it was destroyed while leaving one screen and already appearing on the other one

		// determine the target clients (it's not possible to tell exactly which neighbouring client should be notified)
		Client* targetClient1 = nullptr;
		Client* targetClient2 = nullptr;

		if (clients.size() > 1)
		{
			if (index == 0)
			{
				targetClient1 = &(clients[index + 1]);
				targetClient2 = &(clients[clients.size() - 1]);
			}
			else if (index == clients.size() - 1)
			{
				targetClient1 = &(clients[0]);
				targetClient2 = &(clients[index - 1]);
			}
			else
			{
				targetClient1 = &(clients[index + 1]);
				targetClient2 = &(clients[index - 1]);
			}

			// create a new destroy action 
			Action newAction;
			newAction.command = Destroy;
			newAction.data = data;

			if (targetClient1 != targetClient2)
			{
				// if there are two different neighbours (more than two clients in total), make sure
				// both of them will be notified of the objects destrcution

				//add the destroy action to the action queue of both clients

				(*targetClient1).actions.push(newAction);
				(*targetClient2).actions.push(newAction);
			}
			else
			{
				// there is only one other client (than the source client) -> it is sufficient to add
				// the destroy action once
				(*targetClient1).actions.push(newAction);
			}
			if (!data.wasSent)
			{
				// Update the space ship lists of the source client and remove the destroyed space ship from it.
				// Only do this, if the object has not yet been moved to another client as a consequence to reaching
				// the end of the screen
				client.spaceShips.erase(find(client.spaceShips.begin(), client.spaceShips.end(), data));
			}
		}
		else
		{
			// there is only 1 client, so no need to notify adjacent clients of the destruction of the object;
			// just update the space ship list
			client.spaceShips.erase(find(client.spaceShips.begin(), client.spaceShips.end(), data));
		}

		// Spawn a new space ship to replace the one, that was just destroyed

		DrawingData newData;
		createDrawingData(SpaceShip, newData);

		// Determine spawn client
		Client* spawnTargetClient = nullptr;
		if (newData.moveX > 0)
		{
			// space ships flying to the right start on the first client's screen
			spawnTargetClient = &(clients[0]);
		}
		else
		{
			// space ships flying to the left start on the last client's screen
			spawnTargetClient = &(clients[clients.size() - 1]);
		}

		Action newAction2;
		newAction2.command = Draw;
		newAction2.data = newData;

		(*spawnTargetClient).actions.push(newAction2);
		flightTracks[newData.posY] = newData.id;
	}
}

// Included for convenience and to keep the code of the operator() cleaner.
// Removes a client from the server, this includes
//		- moving all space ships currently active at that client to adjacent clients
//		- shutting down the networker of that client
//		- moving queued actions of the client to adjacent clients
void ClientCommunicator::shutDownClient(Client& client)
{
	if (clients.size() > 1)
	{
		// if there are still active clients, move all space ships and actions of the leaving client to the adjacent clients
		// -> make sure no objects get lost

		if (client.spaceShips.size() > 0)
		{
			for (list<DrawingData>::iterator it = client.spaceShips.begin(); it != client.spaceShips.end(); ++it)
			{
				// add draw actions for all spaceships to the action queue (just for convenience)
				Action newAction;
				newAction.command = Draw;
				newAction.data = *it;

				client.actions.push(newAction);
			}
		}

		Client* targetClient1 = nullptr;
		Client* targetClient2 = nullptr;

		// move all queued actions to adjacent clients
		while (!client.actions.empty())
		{
			// ignore Projectiles (only transfer space ships)
			if (client.actions.front().data.object == SpaceShip)
			{
				// get current position of source client
				unsigned int clientIndex = distance(clients.begin(), find(clients.begin(), clients.end(), client));

				if (client.actions.front().command == Draw)
				{
					// determine target socket for current action
					if (client.actions.front().data.moveX > 0)
					{
						// object moving to the right -> target client will be the next in the vector
						if (clients.size() > clientIndex + 1)
						{
							targetClient1 = &(clients[clientIndex + 1]);
						}
						else
						{
							// source client is last one in the vector -> choose the first one in the vector
							targetClient1 = &(clients[0]);
						}
					}
					else
					{
						// object moving to the left -> target client will be the previous one in the vector
						if (clientIndex > 0)
						{
							targetClient1 = &(clients[clientIndex - 1]);
						}
						else
						{
							// source client is first one in the vector -> choose the last one in the vector
							targetClient1 = &(clients[clients.size() - 1]);
						}
					}

					// add the action to the queue of the target client
					Action newAction;
					newAction.command = client.actions.front().command;
					newAction.data = client.actions.front().data;
					(*targetClient1).actions.push(newAction);
				}
				else
				{
					// Destroy action -> notify both adjacent clients to make sure that object is really destroyed in case it was
					// drawn in parts on two screens at the moment of destruction

					if (clientIndex == 0)
					{
						targetClient1 = &(clients[clientIndex + 1]);
						targetClient2 = &(clients[clients.size() - 1]);
					}
					else if (clientIndex == clients.size() - 1)
					{
						targetClient1 = &(clients[0]);
						targetClient2 = &(clients[clientIndex - 1]);
					}
					else
					{
						targetClient1 = &(clients[clientIndex + 1]);
						targetClient2 = &(clients[clientIndex - 1]);
					}

					Action newAction;
					newAction.command = client.actions.front().command;
					newAction.data = client.actions.front().data;
					(*targetClient1).actions.push(newAction);
					(*targetClient2).actions.push(newAction);
				}
			}
			client.actions.pop();
		}
	}
	else
	{
		// The last active is to be shut down -> not necessary to send anything to other clients (as there aren't any left),
		// all data will be destroyed.
		// Reset flight tracks
		for (map<float, int>::iterator it = flightTracks.begin(); it != flightTracks.end(); ++it)
		{
			(*it).second = -1;
		}
	}

	// shut down the client's networker -> the client will terminate itself as he notices that his networker was shut down
	client.networker.shutDown();
}


// Included for convenience and to keep the code of the operator() cleaner.
// Sends the command and data included in the passed action to the client specified by the first parameter and obtaines 
// acknowledgements for both sending operations.
// Returns true if everything went fine (Acks obtained for both send operations: command and data)
// Returns false if anything goes wrong (including network errors, timeout while waiting for ack,...)
bool ClientCommunicator::sendToClient(Client& client, Action& action)
{
	ClientMessage cMessage = None;

	// send the command
	NetworkerReturnCode sendCommandResult = NW_NOTHING_TO_SEND;
	do
	{
		sendCommandResult = client.networker.send<ServerCommand>(&action.command, sizeof(ServerCommand));
	} while (sendCommandResult == NW_NOTHING_TO_SEND);

	if (sendCommandResult == NW_OK)
	{
		// wait for the acknowledgewent for the receipt of the command
		NetworkerReturnCode selectResult = client.networker.selectRead(TIMEOUT_INTERVAL);
		if (selectResult == NW_OK)
		{
			cMessage = None;
			NetworkerReturnCode recvAckResult = client.networker.receive<ClientMessage>(&cMessage, sizeof(ClientMessage));
			if (recvAckResult == NW_OK && cMessage == ClientAck)
			{
				// Ack obtained, server successfully received the client message -> now send the data
				NetworkerReturnCode sendResultData = NW_NOTHING_TO_SEND;
				do
				{
					sendResultData = client.networker.send<DrawingData>(&(action.data), sizeof(DrawingData));
				} while (sendCommandResult == NW_NOTHING_TO_SEND);

				if (sendResultData == NW_OK)
				{
					// wait for ack to the receipt of the data
					selectResult = client.networker.selectRead(TIMEOUT_INTERVAL);
					if (selectResult == NW_OK)
					{
						cMessage = None;
						recvAckResult = client.networker.receive<ClientMessage>(&cMessage, sizeof(ClientMessage));
						if (recvAckResult == NW_OK && cMessage == ClientAck)
						{
							return true;
						}
						// At the moment, the branches below are simply placeholders and could be used to return specific error 
						// codes based on the error that occurred. At the moment they are not used.
						else
						{
						}
					}
					else
					{
					}
				}
				else
				{
				}
			}
			else
			{
			}
		}
		else
		{
		}
	}
	else
	{
	}
	return false;
}


// return an id used to identify clients
int ClientCommunicator::getClientId()
{
	static int id = 0;
	return id++;
}

// return an id used to identify created space ships
int ClientCommunicator::getSpaceShipId()
{
	static int id = 0;
	// as space ships are destroyed before new ones are created, it is ok to reset the id values after some time
	if (id > 1000)
	{
		id = 0;
	}
	return id++;
}

// fill the passed drawing data structure with appropriate initial values
void ClientCommunicator::createDrawingData(DrawableObject objectType, DrawingData& data)
{
	switch (objectType)
	{
	case SpaceShip:
		data.id = getSpaceShipId();
		data.origin = 0; // 0 indicates that object was created by server
		data.object = SpaceShip; // type of the object
		data.posX = 0.0f; // position of the object (first client will change the x coordinate to an appropriate starting value)

		// find a free flight track (indicated by -1) and use the associated value as y position
		bool found = false;
		map<float, int>::iterator it = flightTracks.begin();
		while (it != flightTracks.end() && !found)
		{
			if ((*it).second == -1)
			{
				data.posY = (*it).first;
				found = true;
			}
			++it;
		}

		data.posZ = 0.0f; // currently not used

		// the extent to which the object will be moved each frame
		data.moveX = (rand() % 60) * 0.01f - 0.3f;
		// add some minimal speed
		if (data.moveX > 0)
		{
			data.moveX += 0.05f;
		}
		else
		{
			data.moveX -= 0.05f;
		}
		data.moveY = 0.0f; // currently not used
		data.moveZ = 0.0f; // currently not used

		// rotation of the object
		data.rotX = static_cast<float>(rand() % 90) - 45.0f;
		if (data.moveX < 0)
		{
			data.rotY = 180.0f;
		}
		else
		{
			data.rotY = 0.0f;
		}
		data.rotZ = 0.0f;

		// size of the object
		data.sizeX = 10.0f;
		data.sizeY = 2.3f;
		data.sizeZ = 6.1f;
		// value required by client
		data.wasSent = false;
		break;
	}
}
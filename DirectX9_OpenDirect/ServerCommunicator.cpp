#include "ServerCommunicator.h"

const long ServerCommunicator::TIMEOUT_INTERVAL = 5000;

ServerCommunicator::ServerCommunicator(void)
{
}

ServerCommunicator::~ServerCommunicator(void)
{
}

// communicate with the server under the given ip address
void ServerCommunicator::operator()(ClientSharedData* sharedData, string ipAddress)
{

	if ((networker.setup() == NW_OK) && (networker.connectToServer(ipAddress.c_str(), 8888) == NW_OK) && (networker.setNonBlockingMode(true) == NW_OK))
	{
		// notify main thread that the connection has been successfully established
		sharedData->connectionEstablished = true;
		sharedData->connectionEstablishedCondition.notify_one();

		//-----------------------------------------------------------------------------------
		// receive an ID from the server from the server, if anything goes wrong, terminate the client

		// wait for the server to send the id or for the function call to timeout
		NetworkerReturnCode selectResult = networker.selectRead(TIMEOUT_INTERVAL);
		switch (selectResult)
		{
		case NW_OK:
		{
			int newId = -1;
			NetworkerReturnCode recvResult = networker.receive<int>(&newId, sizeof(int));
			if (recvResult == NW_OK)
			{
				// acknowledge the receipt of the id
				NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
				ClientMessage message = ClientAck;
				do
				{
					sendMessageResult = networker.send<ClientMessage>(&message, sizeof(ClientMessage));
				} while (sendMessageResult == NW_NOTHING_TO_SEND);

				if (sendMessageResult == NW_OK)
				{
					id = newId;
				}
				else
				{
					// an error occurred
					sharedData->isWorkerThreadRunning = false;
				}
			}
			else
			{
				sharedData->isWorkerThreadRunning = false;
			}
			break;
		}
		case NW_TIMEOUT:
			sharedData->isWorkerThreadRunning = false;
			break;
		case NW_ERROR:
			sharedData->isWorkerThreadRunning = false;
			break;
		}

		bool doSend = false;

		//-----------------------------------------------------------------------------------------------------------------
		// keep running until there is a notification to close from either the server or the main thread
		while (sharedData->isWorkerThreadRunning && sharedData->doRun == true)
		{

			ServerCommand command = DoNothing;
			ClientMessage message = None;


			//-----------------------------------------------------------------------------------
			// update containers holding objects leaving this client and objects being destroyed

			// get objects that have just reached the end of the screen
			{
				lock_guard<std::mutex> newSendObjectsLock(sharedData->newSendObjectsMutex);

				while (!(sharedData->newSendObjects.empty()))
				{
					SendQueueData data;
					data.message = ObjectMoved;
					data.drawObject = sharedData->newSendObjects.front();
					sendQueue.push(data);
					sharedData->newSendObjects.pop();
				}
			}

			// get the objects that have just been destroyed
			{
				lock_guard<std::mutex> newDestroySendObjectsLock(sharedData->newDestroySendObjectsMutex);
				while (!(sharedData->newDestroySendObjects.empty()))
				{
					SendQueueData data;
					data.message = ObjectDestroyed;
					data.drawObject = sharedData->newDestroySendObjects.front();
					sendQueue.push(data);
					sharedData->newDestroySendObjects.pop();
				}
			}


			//-----------------------------------------------------------------------------------
			// check if there is something to receive from the server

			NetworkerReturnCode recvResult = NW_NOTHING_TO_RECEIVE;

			// try to receive a command from the server (not waiting here as there simply may be no command issued)
			recvResult = networker.receive<ServerCommand>(&command, sizeof(ServerCommand));

			switch (recvResult)
			{
			case NW_OK:
			{
				// if a command was successfully received, act accordingly
				switch (command)
				{
				case Draw:
				{
					// acknowledge the receipt of the command
					NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
					message = ClientAck;
					do
					{
						sendMessageResult = networker.send<ClientMessage>(&message, sizeof(ClientMessage));
					} while (sendMessageResult == NW_NOTHING_TO_SEND);

					if (sendMessageResult == NW_OK)
					{
						// ack for command was sent successfully back to the server

						// receive drawing data of the object to draw from server
						DrawingData rcvData;

						// wait for the server to send the object that should be drawn
						NetworkerReturnCode selectResult = networker.selectRead(TIMEOUT_INTERVAL);
						switch (selectResult)
						{
						case NW_OK:
						{
							NetworkerReturnCode recvResult = networker.receive<DrawingData>(&rcvData, sizeof(DrawingData));
							if (recvResult == NW_OK)
							{
								// acknowledge the receipt of the data
								NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
								message = ClientAck;
								do
								{
									sendMessageResult = networker.send<ClientMessage>(&message, sizeof(ClientMessage));
								} while (sendMessageResult == NW_NOTHING_TO_SEND);

								if (sendMessageResult == NW_OK)
								{
									// add new object to queue of new draw objects
									{
										lock_guard<std::mutex> doDrawLock(sharedData->newDrawObjectsMutex);
										sharedData->newDrawObjects.push(rcvData);
									}
								}
								else
								{
									// an error occurred
									sharedData->isWorkerThreadRunning = false;
								}
							}
							else
							{
								sharedData->isWorkerThreadRunning = false;
							}
							break;
						}
						case NW_TIMEOUT:
							sharedData->isWorkerThreadRunning = false;
							break;
						case NW_ERROR:
							sharedData->isWorkerThreadRunning = false;
							break;
						}
					}
					else
					{
						// an error occurred
						sharedData->isWorkerThreadRunning = false;
					}
					break;
				}
				case Destroy:
				{
					// acknowledge the receipt of the command
					NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
					message = ClientAck;
					do
					{
						sendMessageResult = networker.send<ClientMessage>(&message, sizeof(ClientMessage));
					} while (sendMessageResult == NW_NOTHING_TO_SEND);

					if (sendMessageResult == NW_OK)
					{
						// receive id of object to be destroyed
						DrawingData rcvData;

						// wait for the server to send the object that should be drawn
						NetworkerReturnCode selectResult = networker.selectRead(TIMEOUT_INTERVAL);
						switch (selectResult)
						{
						case NW_OK:
						{
							NetworkerReturnCode recvResult = networker.receive<DrawingData>(&rcvData, sizeof(DrawingData));
							if (recvResult == NW_OK)
							{
								// acknowledge the receipt of the data
								NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
								message = ClientAck;
								do
								{
									sendMessageResult = networker.send<ClientMessage>(&message, sizeof(ClientMessage));
								} while (sendMessageResult == NW_NOTHING_TO_SEND);

								if (sendMessageResult == NW_OK)
								{
									// add new object to queue of DrawingData objects to be destroyed
									{
										lock_guard<std::mutex> newDestroyObjectsLock(sharedData->newDestroyObjectsMutex);
										sharedData->newDestroyObjects.push(rcvData);
									}
								}
								else
								{
									// an error occurred
									sharedData->isWorkerThreadRunning = false;
								}
							}
							else
							{
								sharedData->isWorkerThreadRunning = false;
							}
							break;
						}
						case NW_TIMEOUT:
							sharedData->isWorkerThreadRunning = false;
							break;
						case NW_ERROR:
							sharedData->isWorkerThreadRunning = false;
							break;
						}
					}
					else
					{
						// an error occurred
						sharedData->isWorkerThreadRunning = false;
					}
				}
				break;
				case Send:
				{
					if (sendQueue.empty())
					{
						// there's no data to be sent to the server, but ack the server command
						NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
						message = ClientAck;
						do
						{
							sendMessageResult = networker.send<ClientMessage>(&message, sizeof(ClientMessage));
						} while (sendMessageResult == NW_NOTHING_TO_SEND);

						if (sendMessageResult == NW_OK)
						{
							doSend = false;
						}
						else
						{
							// an error occurred
							sharedData->isWorkerThreadRunning = false;
						}
					}
					else
					{
						// inform server that data is ready and will be sent from this client
						NetworkerReturnCode sendMessageResult = NW_NOTHING_TO_SEND;
						message = DataReady;
						do
						{
							sendMessageResult = networker.send<ClientMessage>(&message, sizeof(ClientMessage));
						} while (sendMessageResult == NW_NOTHING_TO_SEND);

						if (sendMessageResult == NW_OK)
						{
							doSend = true;
						}
						else
						{
							// an error occurred
							sharedData->isWorkerThreadRunning = false;
						}
					}
				}
				break;
				}
				break;
			}
			case NW_ERROR:
				sharedData->isWorkerThreadRunning = false;
				break;
			case NW_CONNECTION_CLOSED:
				sharedData->isWorkerThreadRunning = false;
				break;
			case NW_NOTHING_TO_RECEIVE:
			{

			}
			break;
			}

			//--------------------------------------------------------------------------------------
			// send something to send to the server if possible and requested

			// notify the server of objects that are either leaving this client's screen or have been destroyed on this client's screen;
			// only continue if the server is still running (wasn't shut down during the receive phase before)
			if (doSend && (sharedData->isWorkerThreadRunning) && (!sendQueue.empty()))
			{
				NetworkerReturnCode sendResultMessage = NW_NOTHING_TO_SEND;
				do
				{
					sendResultMessage = networker.send<ClientMessage>(&sendQueue.front().message, sizeof(ClientMessage));
				} while (sendResultMessage == NW_NOTHING_TO_SEND);

				if (sendResultMessage == NW_OK)
				{
					selectResult = networker.selectRead(TIMEOUT_INTERVAL);
					switch (selectResult)
					{
					case NW_OK:
					{
						command = DoNothing;
						NetworkerReturnCode recvAckResult = networker.receive<ServerCommand>(&command, sizeof(ServerCommand));
						if (recvAckResult == NW_OK && command == ServerAck)
						{
							// Ack obtained, server successfully received the client message -> now send the data

							NetworkerReturnCode sendResultData = NW_NOTHING_TO_SEND;
							do
							{
								sendResultData = networker.send<DrawingData>(&(sendQueue.front().drawObject), sizeof(DrawingData));
							} while (sendResultData == NW_NOTHING_TO_SEND);

							if (sendResultData == NW_OK)
							{
								// wait for ack
								selectResult = networker.selectRead(TIMEOUT_INTERVAL);
								switch (selectResult)
								{
								case NW_OK:
								{
									command = DoNothing;
									recvAckResult = networker.receive<ServerCommand>(&command, sizeof(ServerCommand));
									if (recvAckResult == NW_OK && command == ServerAck)
									{
										sendQueue.pop();
									}
									else
									{
										sharedData->isWorkerThreadRunning = false;
									}
									break;
								}
								case NW_TIMEOUT:
									sharedData->isWorkerThreadRunning = false;
									break;
								case NW_ERROR:
									sharedData->isWorkerThreadRunning = false;
									break;
								}
							}
							else
							{
								// an error occurred while trying to send the data
								sharedData->isWorkerThreadRunning = false;
							}
						}
						else
						{
							sharedData->isWorkerThreadRunning = false;
						}
						break;
					}
					case NW_TIMEOUT:
						sharedData->isWorkerThreadRunning = false;
						break;
					case NW_ERROR:
						sharedData->isWorkerThreadRunning = false;
						break;
					}
				}
				else
				{
					// an error occurred while trying to send the message
					sharedData->isWorkerThreadRunning = false;
				}

				doSend = false;
			}

		}

		// terminate the worker thread.
		// If server is still running, it will account for cleanly removing the client from the active ones (no explicit message
		// from client required)

		networker.shutDown();
		Networker::cleanUp();
	}
	else
	{
		sharedData->isWorkerThreadRunning = false;
	}
}

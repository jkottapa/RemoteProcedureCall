#include "server_handler.h"
#include "common.h"
#include "rpc_message.h"
#include <stdlib.h>
#include <iostream>
#include <pthread.h>

using namespace std;


void* pthread_routine_wrapper( void* args ){

	ServerHandler *handler = (ServerHandler*) args;
	handler->WorkerRoutine();
	return 0;
}

ServerHandler::ServerHandler() : TCP()
{
	doExit = true;
	pthread_mutex_init( &qMutex, NULL );
	pthread_attr_init( &attr );
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
}

ServerHandler::~ServerHandler()
{
	pthread_attr_destroy( &attr );
	pthread_mutex_destroy( &qMutex );
}

/*
	returns:
	SUCCESS
	ERR_* when errors occur
*/

int ServerHandler::Init()
{	
	char *binderAddr = getenv("BINDER_ADDRESS");
	char *bportNum = getenv("BINDER_PORT");

	if ( binderAddr == NULL || bportNum == NULL )
		return ERR_BINDER_ADDRESS_MISSING;

	if ( this->Connect( binderAddr, bportNum) != 0 )
		return ERR_BINDER_UNREACHABLE;

	if ( this->Listen() != 0 )
		return ERR_SOCKET_LISTENING_FAILED;
	
	return SUCCESS;
}

int ServerHandler::Register(char* functionName, int* argTypes, skeleton f)
{
	int ret = SUCCESS;
	char* serverName;
	int portNum;
	RPCMessage sendMessage, recvMessage;

	if ( isConnectionUp() == false )
		return ERR_NOT_INITIALIZED;

	serverName = this->getHostName();
	portNum = this->getPortNum();

	sendMessage.CreateRegisterMessage(serverName, portNum,functionName, argTypes);

	if( (ret = this->Send(sendMessage)) < 0 )
		return ERR_COMMUNICATION_FAILED;

	if( (ret = this->Recv(recvMessage)) < 0)
		return ERR_COMMUNICATION_FAILED;

	if( ret == 0 )
		return ERR_BINDER_SHUTDOWN;

	ret = recvMessage.ParseErrorMessage().reasonCode;
	if ( recvMessage.msgType == REQ_REGISTER_FAILURE )
		return ret;
	
	functionDB.AddFunctionEntry(functionName, argTypes, serverName, portNum, -1, f);

	return ret;
}

bool ServerHandler::DoExit()
{
	bool ret = terminateReceived && maxFd == listeningFd;
	if ( ret )
	{
		/* close connection to binder */
		this->Disconnect();
	}
	return ret;
}

int ServerHandler::Execute()
{
	if ( isListening() == false )
		return ERR_NOT_INITIALIZED;

	if (functionDB.Size() == 0 )
		return ERR_NO_REGISTERED_FUNCITONS;

	return this->ListenForConnections();
}

int ServerHandler::ProcessExecute(RPCMessage& recvMessage, RPCMessage& sendMessage, int socketFd)
{

	Execute_Msg msg = recvMessage.ParseExecuteMessage();
	skeleton f = functionDB.GetSkeleton(msg.name, msg.argTypes);
	int ret = f(msg.argTypes, msg.args);

	if ( ret < 0 )
	{
		sendMessage.CreateFailureMessage(ERR_SERVER_FUNCTION_RETURNED_ERROR, REQ_EXECUTE_FAILURE);
	}
	else
	{
		/*
		Copy recvMessage to sendMessage since the message
		content are same for EXECUTE msg. Only the args
		can change so therefore copy new args to sendMessage
		*/
		
		sendMessage.Copy(recvMessage);
		void *sendMessageArgsLocation = (sendMessage.payload + FUNC_NAME_MAX + 
			(argTypes_Length(msg.argTypes) *  sizeof(int)));

		args_Copy(msg.argTypes, msg.args, sendMessageArgsLocation);
		sendMessage.msgType = REQ_EXECUTE_SUCCESS;
	}
	
	return ret;
}

void ServerHandler::HandleMessage(SocketMessage msg)
{
	pthread_t worker_thread;
	EnqueueMessage(msg);
	pthread_create( &worker_thread, &attr, pthread_routine_wrapper, this);
}

void ServerHandler::EnqueueMessage(SocketMessage msg)
{
	pthread_mutex_lock(&qMutex);
	messageQueue.push(msg);
	pthread_mutex_unlock(&qMutex);	
}

SocketMessage ServerHandler::DequeueMessage()
{
	pthread_mutex_lock(&qMutex);
	SocketMessage msg = messageQueue.front();
	messageQueue.pop();
	pthread_mutex_unlock(&qMutex);
	return msg;
}

int ServerHandler::ProcessMessage(RPCMessage* recvMessage, RPCMessage* sendMessage, int socketFd)
{

	/*
	Check if terminate message is set
	and is coming from binder socketFd.
	If it is then immediately set 
	terminateionReceived flag
	*/

	if ( recvMessage->msgType == REQ_TERMINATE && socketFd == connectionFd )
	{
		terminateReceived = true;
	}

	/*
	If its not a terminate message, handle that request 
	in a separate thread.
	one.
	*/

	if ( recvMessage->msgType != REQ_TERMINATE )
		HandleMessage(SocketMessage(recvMessage, sendMessage, socketFd));

	return 0;
}

void ServerHandler::WorkerRoutine( ){

	/*
	If worker routine is called,
	we know there is atleast one 
	message in the queue.
	*/

	SocketMessage msg = DequeueMessage();
	RPCMessage *recvMessage = msg.recvMessage;
	RPCMessage *sendMessage = msg.sendMessage;
	int socketFd = msg.socketFd;
	int err = 0;

	switch(recvMessage->msgType)
	{
		case REQ_EXECUTE:
			err = ProcessExecute(*recvMessage, *sendMessage, socketFd);
			this->Send(*sendMessage, socketFd);
			break;
		default:
			break;
	}

	delete recvMessage;
	delete sendMessage;
	pthread_exit( NULL );
}


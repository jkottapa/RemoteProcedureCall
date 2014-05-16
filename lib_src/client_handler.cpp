#include "client_handler.h"
#include "common.h"
#include "rpc_message.h"
#include "utility.h"
#include "database.h"

#include <stdlib.h>
#include <iostream>
#include <sstream>

using namespace std;

ClientHandler::ClientHandler() : TCP()
{
	binderAddr = getenv("BINDER_ADDRESS");
	bportNum = getenv("BINDER_PORT");
}


int ClientHandler::Call(char* name, int *argTypes, void** args)
{
	int ret = 0;
	RPCMessage sendMessage, recvMessage;
	RPCMessage execMessage, execResponse;
	stringstream convert;
	string portNumStr;
	Location_Rsp_Msg msg;
	char* serverId;		
	char* portNum;

	if ( binderAddr == NULL || bportNum == NULL )
		return ERR_BINDER_ADDRESS_MISSING;

	// Connect to binder
	if( this->Connect( binderAddr, bportNum ) == -1 )
		return ERR_BINDER_UNREACHABLE;

	sendMessage.CreateLocRequestMessage(name, argTypes);
	ret = this->Send( sendMessage );

	if( ret == -1 ){
		this->Disconnect();
		return ERR_COMMUNICATION_FAILED;
	}

	ret = this->Recv( recvMessage );
	this->Disconnect();
	
	if( ret == -1 ) {		
		return ERR_COMMUNICATION_FAILED;
	}
	else if( ret == 0 ){
		return ERR_BINDER_SHUTDOWN;
	}	

	if( recvMessage.msgType == REQ_LOC_FAILURE){
		return recvMessage.ParseErrorMessage().reasonCode;
	}
	
	msg = recvMessage.ParseLocationRspMessage();
	serverId = msg.serverId;
	convert << msg.portNum;
	convert >> portNumStr;
	portNum = (char*) portNumStr.c_str();

#ifdef DEBUG
	cout << serverId << endl;
	cout << portNum << endl;
#endif

	if ( this->Connect(serverId, portNum) == -1 )
		return ERR_SERVER_UNREACHABLE;

	execMessage.CreateExecuteMessage(REQ_EXECUTE, name, argTypes, args);
	ret = this->Send( execMessage );

	if( ret == -1 ){
		this->Disconnect();
		return ERR_COMMUNICATION_FAILED;
	}

	ret = this->Recv( execResponse );
	this->Disconnect();

	if( ret == -1 ){
		return ERR_COMMUNICATION_FAILED;
	}
	else if( ret == 0){
		return ERR_SERVER_SHUTDOWN;
	}
	
	if( execResponse.msgType == REQ_EXECUTE_FAILURE){
		return execResponse.ParseErrorMessage().reasonCode;
	}

	// Copy out the args from execResponse msg buffer
	Execute_Msg e = execResponse.ParseExecuteMessage();
	copy_Args(e.argTypes, args, e.args);

	return SUCCESS;
}

int ClientHandler::CacheCall(char* name, int* argTypes, void** args)
{
	int ret = 0;
	RPCMessage sendMessage, recvMessage;
	RPCMessage execMessage, execResponse;
	stringstream convert;
	string portNumStr;
	char* serverId;		
	char* portNum;
	ServerInfo *server;

	if ( functionCache.GetServer(name, argTypes, server) != SUCCESS )
	{

		if ( binderAddr == NULL || bportNum == NULL )
			return ERR_BINDER_ADDRESS_MISSING;

		// Connect to binder
		if( this->Connect( binderAddr, bportNum ) == -1 )
			return ERR_BINDER_UNREACHABLE;

		sendMessage.CreateCacheLocRequestMessage(name, argTypes);
		ret = this->Send( sendMessage );

		if( ret == -1 ){
			this->Disconnect();
			return ERR_COMMUNICATION_FAILED;
		}

		ret = this->Recv( recvMessage );
		this->Disconnect();
		
		if( ret == -1 ) {
			return ERR_COMMUNICATION_FAILED;
		}
		else if( ret == 0 ){
			return ERR_BINDER_SHUTDOWN;
		}	

		if( recvMessage.msgType == REQ_CACHE_LOC_FAILURE ){
			return recvMessage.ParseErrorMessage().reasonCode;
		}

		Location_Cache_Rsp_Msg msg = recvMessage.ParseLocationCacheRspMessage();
		ServerDetails *serverList = msg.serverList;

		for( int i = 0; i < msg.serverCount; i++)
		{
			functionCache.AddFunctionEntry(name, argTypes, serverList[i].serverName, serverList[i].portNum);
		}

		/*
		By this time there should be at least one server entry
		if we don't that means we would have returned an error
		before even hitting this
		*/

		functionCache.GetServer(name, argTypes, server);
	}
	
	serverId = server->serverDetails->serverName;
	convert << server->serverDetails->portNum;
	convert >> portNumStr;
	portNum = (char*) portNumStr.c_str();

	if ( this->Connect(serverId, portNum) == -1 )
	{
		functionCache.RemoveServerByNamePort(serverId, server->serverDetails->portNum);
		/*
		In theory rpcCacheCall will never return ERR_SERVER_UNREACHABLE
		this is so because it is in a loop to find next available server
		and try it. If all fails, it gets the new list from binder.
		If the binder for some reason still holds the entries for 
		non-available servers (this shouldn't happen), there will be an
		infinite loop. But as expected, binder will remove disconnected
		servers. When this happens, ERR_NO_SERVER_AVAILABLE is likely to be 
		returned from binder, which will be returned from this call.
		*/
		return CacheCall(name, argTypes, args);
	}

	execMessage.CreateExecuteMessage(REQ_EXECUTE, name, argTypes, args);
	ret = this->Send( execMessage );

	if( ret == -1 ){
		this->Disconnect();
		return ERR_MESSAGE_SEND_FAILED;
	}

	ret = this->Recv( execResponse );
	this->Disconnect();

	if( ret == -1 ){
		return ERR_MESSAGE_RECV_FAILED;
	}
	else if( ret == 0){
		functionCache.RemoveServerByNamePort(serverId, server->serverDetails->portNum);
		return ERR_SERVER_SHUTDOWN;
	}
	
	if( execResponse.msgType == REQ_EXECUTE_FAILURE ){

		/*
		If one server has failed to fulfill the request
		remove its entry for that particular function
		*/
		ret = execResponse.ParseErrorMessage().reasonCode;
		if ( ret == ERR_SERVER_FUNCTION_RETURNED_ERROR )
			return ret;
		else
			functionCache.RemoveServerEntryByFunction(name, argTypes, serverId, server->serverDetails->portNum);

		/*
		Recursively call CacheCall which will try next server
		in case if non-exists, it will try to pull from binder
		if binder returns non, error code from binder will be 
		returned 
		*/

		return CacheCall(name, argTypes, args);
	}

	// Copy out the args from execResponse msg buffer
	Execute_Msg e = execResponse.ParseExecuteMessage();
	copy_Args(e.argTypes, args, e.args);

	return SUCCESS;
}

int ClientHandler::Terminate() {
	
	int ret = 0;
	RPCMessage terminateMessage;

	if ( this->Connect( binderAddr, bportNum ) == -1 )
		return ERR_BINDER_UNREACHABLE;

	terminateMessage.CreateTerminateMessage();
	ret = this->Send( terminateMessage );
	this->Disconnect();

	if( ret == -1 )
	{
		return ERR_MESSAGE_SEND_FAILED;
	}
	else if ( ret == 0 )
	{
		return ERR_BINDER_SHUTDOWN;
	}

	return SUCCESS;
}
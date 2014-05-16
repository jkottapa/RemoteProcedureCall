#include "binder.h"
#include "../lib_src/protocol.h"
#include <iostream>
#include "../lib_src/rpc_message.h"
#include "../lib_src/utility.h"
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

using namespace std;

Binder::Binder() : TCP() {

	int ret = this->Listen();
	FD_ZERO(&this->serverFds);

	if (ret >= 0)
	{
		cout << "BINDER_ADDRESS " << this->getHostName() << endl;
		cout << "BINDER_PORT " << this->getPortNum() << endl;
	}
}

Binder::~Binder() {
	// do nothing as of now
}

void Binder::Disconnect(int fd)
{
	if ( FD_ISSET(fd, &serverFds) )
	{
		/*
		This means that there is at least 
		one function registered by this server
		so remove this server from database
		*/
		functionDB.RemoveServerByFd(fd);
		FD_CLR(fd, &serverFds);
	}
	TCP::Disconnect(fd);
}

int Binder::ProcessReqLocation(RPCMessage& recvMessage, RPCMessage& sendMessage)
{
	Location_Req_Msg msg = recvMessage.ParseLocationReqMessage();
	ServerInfo *serverInfo;
	int ret = functionDB.GetServer(msg.name, msg.argTypes, serverInfo);
	if ( ret == SUCCESS )
	{
		sendMessage.CreateLocResponseMessage(serverInfo->serverDetails->serverName, 
											serverInfo->serverDetails->portNum);
		sendMessage.msgType = REQ_LOC_SUCCESS;
	}
	else
	{
		sendMessage.CreateFailureMessage(ERR_NO_SERVER_AVAILABLE, REQ_LOC_FAILURE);
	}
	return 0;
}

int Binder::ProcessReqCacheLocation(RPCMessage& recvMessage, RPCMessage& sendMessage)
{
	Location_Req_Msg msg = recvMessage.ParseLocationReqMessage();
	vector<ServerInfo *>* servers;
	int ret = functionDB.GetAllServers(msg.name, msg.argTypes, servers);

	if ( ret == SUCCESS )
	{
		sendMessage.CreateCacheLocResponseMessage(*servers);
		sendMessage.msgType = REQ_CACHE_LOC_SUCCESS;
	}
	else
	{
		sendMessage.CreateFailureMessage(ERR_NO_SERVER_AVAILABLE, REQ_CACHE_LOC_FAILURE);
	}
	return 0;
}

int Binder::ProcessRegister(RPCMessage& recvMessage, RPCMessage& sendMessage, int fd)
{
	Register_Msg msg = recvMessage.ParseRegisterMessage();
	int ret = functionDB.AddFunctionEntry(msg.name, msg.argTypes, msg.serverId, msg.portNum, fd);

	/*
	We will need this fds to send Terminate message
	*/

	if ( FD_ISSET(fd, &serverFds) == false )
	{
		FD_SET(fd, &serverFds);
	}

	if ( ret >= 0 )
	{
		sendMessage.CreateFailureMessage(ret, REQ_REGISTER_SUCCESS);
	}
	else
	{
		sendMessage.CreateFailureMessage(ERR_BINDER_REGISTRATION_FAILED, REQ_REGISTER_FAILURE);
	}

	return 0;
}

bool Binder::DoExit()
{
	return terminateReceived && maxFd == this->listeningFd;
}

int Binder::ProcessTerminate(RPCMessage& recvMessage, RPCMessage& sendMessage)
{
	RPCMessage terminateMessage;
	terminateMessage.CreateTerminateMessage();
	terminateReceived = true;
	
	for(int i = 0; i <= maxFd; i++)
	{
		if ( FD_ISSET(i, &readFds) && FD_ISSET(i, &serverFds) )
		{
			this->Send(terminateMessage, i);
		}
	}

	FD_ZERO(&serverFds);
	return 0;
}

int Binder::ProcessMessage(RPCMessage* recvMessage, RPCMessage* sendMessage, int fd) {
	
	int err = 0;
	switch( recvMessage->msgType ){
		case REQ_REGISTER:
			err = this->ProcessRegister(*recvMessage, *sendMessage, fd);
			break;
		case REQ_LOC_REQUEST:
			err = this->ProcessReqLocation(*recvMessage, *sendMessage);
			break;
		case REQ_CACHE_LOC_REQUEST:
			err = this->ProcessReqCacheLocation(*recvMessage, *sendMessage);
			break;
		case REQ_TERMINATE:
			err = this->ProcessTerminate(*recvMessage, *sendMessage);
			break;
		default:
			break;
	}

	if ( recvMessage->msgType != REQ_TERMINATE )
		this->Send(*sendMessage, fd);

	delete recvMessage;
	delete sendMessage;
	return err;
}


int main() {
	Binder b;
	b.ListenForConnections();
	return 1;
}
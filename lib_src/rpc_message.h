#ifndef _RPC_PACKET_H_
#define _RPC_PACKET_H_

#include "common.h"
#include "../rpc_src/rpc.h"
#include "database.h"
#include "protocol.h"
#include <vector>

using namespace std;

class RPCMessage {

	public:
		unsigned int payloadLength;
		int msgType;
		char *payload;

	public:
		RPCMessage() : payloadLength(0), payload(NULL){};

		~RPCMessage()
		{
			FreePayload();
		}

		// functions to create messages
		void CreateExecuteMessage(int msgType, char *name, int *argTypes, void **args );
		void CreateFailureMessage( int reasonCode, int msgType );
		void CreateLocRequestMessage(char* name, int* argTypes);
		void CreateLocResponseMessage(char *serverId, int portNum);
		void CreateCacheLocRequestMessage(char* name, int* argTypes);
		void CreateCacheLocResponseMessage(vector<ServerInfo *>& servers);
		void CreateRegisterMessage(char* serverId, int portNum, char* name, int* argTypes);
		void CreateTerminateMessage();

		// function to parse messages
		Register_Msg ParseRegisterMessage();
		Error_Msg ParseErrorMessage();
		Location_Req_Msg ParseLocationReqMessage();
		Location_Rsp_Msg ParseLocationRspMessage();
		Location_Cache_Rsp_Msg ParseLocationCacheRspMessage();
		Execute_Msg ParseExecuteMessage();

		void Copy(RPCMessage& msg);
		void FreePayload()
		{
			if ( this->payload != NULL )
			{
				delete[] this->payload;
				this->payload = NULL;
			}
		}

};

/*

This struct holds memory pointers to
recv and send messages. It also holds
the socket file descriptor from which
the message was received.

*/
struct SocketMessage
{
	RPCMessage* recvMessage;
	RPCMessage* sendMessage;
	int socketFd;
	
	public:
		SocketMessage(RPCMessage* recvMessage, RPCMessage* sendMessage, int socketFd)
		{
			this->recvMessage = recvMessage;
			this->sendMessage = sendMessage;
			this->socketFd = socketFd;
		}
};

#endif //_RPC_PACKET_H_

#ifndef _TCP_CONNECTION_
#define _TCP_CONNECTION_

#include "rpc_message.h"
#include "common.h"
#include <sys/param.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netdb.h>
#include <limits.h>

class TCP {
	protected:
		int listeningFd;
		int connectionFd;
		char myHostName[MAXHOSTNAMELEN];
		int myPortNum, maxFd;
		fd_set readFds, tempFds;
		bool terminateReceived, doExit;
		
		virtual int ProcessMessage(RPCMessage* recvMessage, RPCMessage* sendMessage, int i);
		virtual bool DoExit();
		virtual void Disconnect(int fd);

	public:
	
		// client calls
		int Connect(char* hostName, char* portNum);
		void Disconnect();

		int ListenForConnections();

		// server calls
		int Listen();
		int CloseConnections();

		// common calls
		int Recv(RPCMessage& RPCMessage, int fd);
		int Send(RPCMessage& RPCMessage, int fd);

		int Recv(RPCMessage& RPCMessage);
		int Send(RPCMessage& RPCMessage);

		int Accept();

		bool isConnectionUp()
		{ 
			return connectionFd >= 0; 
		}

		bool isListening()
		{
			return listeningFd >= 0;
		}

		char* getHostName()
		{
			return myHostName;
		}

		int getPortNum()
		{
			return myPortNum;
		}

		TCP();
		virtual ~TCP();

};

#endif //_TCP_CONNECTION_

#include "tcp.h"

#include <cstdlib>
#include <iostream>
#include <cerrno>

using namespace std;

TCP::TCP() {
	connectionFd = -1;
	listeningFd = -1;
	terminateReceived = false;
	FD_ZERO(&readFds);
}

TCP::~TCP() {

}

int TCP::Connect(char* hostName, char* portNum)
{
	addrinfo temp, *serverAddress;
	int connectionFd;
	int val;

	bzero( (void *)&temp, sizeof(temp) );
	temp.ai_family = AF_INET;
	temp.ai_socktype = SOCK_STREAM;
	temp.ai_flags = 0;
	temp.ai_protocol = 0;

	val = getaddrinfo( hostName, portNum, &temp, &serverAddress );
	if ( val != 0)
	{

		freeaddrinfo(serverAddress);
		return -1;
	}

	connectionFd = socket(serverAddress->ai_family, serverAddress->ai_socktype, serverAddress->ai_protocol);
	if ( connectionFd < 0 )
	{

		freeaddrinfo(serverAddress);
		return -1;
	}

	if ( connect(connectionFd, serverAddress->ai_addr, serverAddress->ai_addrlen) < 0 )
	{

		freeaddrinfo(serverAddress);
		return -1;
	}

	this->connectionFd = connectionFd;
	FD_SET(this->connectionFd, &readFds);
	freeaddrinfo(serverAddress);
	return 0;
}

int TCP::Listen()
{
	struct sockaddr_in serverAddress;
	socklen_t sockLen = sizeof(serverAddress);
	int listeningFd;
	errno = 0;

	listeningFd = socket(AF_INET, SOCK_STREAM, 0);
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(0);

	CHECKERROR( bind( listeningFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)),  GENERAL_CHECK)
	CHECKERROR( listen(listeningFd, MAX_CLIENTS),  GENERAL_CHECK)
	CHECKERROR( getsockname(listeningFd, (struct sockaddr *)&serverAddress, &sockLen ),  GENERAL_CHECK )
	CHECKERROR( gethostname(this->myHostName, sizeof(this->myHostName)),  GENERAL_CHECK )

	this->myPortNum = ntohs(serverAddress.sin_port);
	this->listeningFd = listeningFd;
	FD_SET(this->listeningFd, &readFds);
	
	return 0;
}

bool TCP::DoExit()
{
	// override in child class
	return false;
}

int TCP::ListenForConnections()
{		
	int newFd, ret;
	RPCMessage* recvMessage, *sendMessage;

	FD_ZERO(&tempFds);
	maxFd = max(this->listeningFd, this->connectionFd);

	while(true)
	{
		tempFds = readFds;

		/*
		If terminate is set, and listeningFd is the only
		fd left in fd set, that means all server sockets
		are closed and we should also close fd
		*/

		if ( DoExit() )
			break;

		ret = select(maxFd + 1, &tempFds, NULL, NULL, NULL);
		if (ret)
		{
			for ( int i = 0; i <= maxFd; i++ )
			{
				if ( FD_ISSET(i, &tempFds) )
				{
					/*
					If terminateReceived is set, do not accept new 
					connections
					*/
					if ( i == this->listeningFd && !terminateReceived)
					{
						newFd = this->Accept();
						FD_SET(newFd, &readFds);
						maxFd = max(maxFd, newFd);
					}
					else
					{
						recvMessage = new RPCMessage();
						sendMessage = new RPCMessage();

						ret = this->Recv(*recvMessage, i);
						if ( ret == 0 )
						{
							this->Disconnect(i);
							delete recvMessage;
							delete sendMessage;
						}
						else if (ret > 0)
						{
							// Also do clean up in ProcessMessage
							this->ProcessMessage(recvMessage, sendMessage, i);
						}
					}
				}
			}
		}
	}

	this->Disconnect(this->listeningFd);
	return SUCCESS;
}

void TCP::Disconnect() {
	return this->Disconnect(this->connectionFd);
}

void TCP::Disconnect(int fd) {
	shutdown(fd, SHUT_RDWR);
	close(fd);
	FD_CLR(fd, &readFds);
	if ( fd == maxFd )
	{
		while( FD_ISSET(maxFd, &readFds) == false )
		{
			maxFd--;
			if ( maxFd == 0 )
				break;
		}
	}
	if ( fd == connectionFd )
		connectionFd = -1;
	else if ( fd == listeningFd )
		listeningFd = -1;
}

int TCP::CloseConnections() {
	int ret = 0;
	for(int i = 0; i <= maxFd; i++){
		shutdown( i, SHUT_RDWR );
		ret = close( i );
		if( ret < 0 ) {
			return ret;
		}
		FD_CLR(i, &readFds );
	}
	return 0;
}

int TCP::ProcessMessage(RPCMessage* recvMessage, RPCMessage* sendMessage, int i)
{
	// overload this in child class
	return 0;
}

// returns the size of bytes received or error
int TCP::Recv(RPCMessage& message, int fd) {
	
	int ret = 0;

	CHECK_BYTES(recv(fd, (void *)&(message.payloadLength), sizeof(message.payloadLength), 0))
	CHECK_BYTES(recv(fd, (void *)&(message.msgType), sizeof(message.msgType), 0))
	if ( message.payloadLength > 0 )
	{
		char *buff = new char[message.payloadLength];
		ret = recv(fd, (void *)buff, message.payloadLength, 0);
		if ( ret < 0 || ret == 0)
		{
			delete[] buff;
			return ret;
		}
		message.payload = buff;
	}
	else
	{
		message.payload = NULL;
	}
	
	return ret;
}

// sends the size of bytes sent, or error
int TCP::Send(RPCMessage& message, int fd){
		
	int sendBytes = 0;
	int ret = 0;

	CHECKERROR( (ret = send(fd, &message.payloadLength, sizeof(int), 0)), SEND)
	sendBytes += ret;

	CHECKERROR( (ret = send(fd, &message.msgType, sizeof(int), 0)), SEND )
	sendBytes += ret;

	CHECKERROR( (ret = send(fd, message.payload, message.payloadLength, 0)), SEND );
	sendBytes += ret;

	return sendBytes;
}

int TCP::Recv(RPCMessage& message)
{
	return this->Recv(message, this->connectionFd);
}

int TCP::Send(RPCMessage& message)
{
	return this->Send(message, this->connectionFd);
}

// accepts the fd and returns
int TCP::Accept() {
	return accept( this->listeningFd, (sockaddr*)NULL, NULL);
}
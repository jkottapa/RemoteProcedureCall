#ifndef _SERVER_HANDLER_H_
#define _SERVER_HANDLER_H_

#include "tcp.h"
#include "database.h"
#include <pthread.h>
#include <queue>
#include <semaphore.h>

using namespace std;

class ServerHandler : public TCP
{
	private:
		Database functionDB;

		pthread_attr_t attr;
		pthread_mutex_t qMutex;
		queue<SocketMessage> messageQueue;
		bool DoExit();
		
		void WorkerRoutine();
		void HandleMessage(SocketMessage msg);
		void EnqueueMessage(SocketMessage msg);
		SocketMessage DequeueMessage();

	protected:
		int ProcessMessage(RPCMessage* recvMessage, RPCMessage* sendMessage, int socketFd);
		int ProcessExecute(RPCMessage& recvMessage, RPCMessage& sendMessage, int socketFd);

	public:
		ServerHandler();
		~ServerHandler();
		int Init();
		int Execute();
		int Register(char *functioName, int* argTypes, skeleton f);
		
		friend void* pthread_routine_wrapper(void* args);
};

#endif

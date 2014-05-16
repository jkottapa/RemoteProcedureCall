#ifndef _CLIENT_HANDLER_H_
#define _CLIENT_HANDLER_H_

#include "tcp.h"
#include "database.h"

class ClientHandler : public TCP
{
	private:
		char* binderAddr;
		char* bportNum;

		Database functionCache;
		
	public:
		ClientHandler();
		int Call(char* name, int* argTypes, void** args);
		int CacheCall(char *name, int* argTypes, void** args);
		int Terminate();
};

#endif

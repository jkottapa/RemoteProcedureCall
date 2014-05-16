#include "rpc.h"
#include "../lib_src/tcp.h"
#include "../lib_src/rpc_message.h"
#include "../lib_src/server_handler.h"
#include "../lib_src/client_handler.h"
#include <iostream>

using namespace std;

ServerHandler serverHandler;
ClientHandler clientHandler;

int rpcInit() 
{
	return serverHandler.Init();
}

int rpcCall( char* name, int *argTypes, void** args)
{
	return clientHandler.Call(name, argTypes, args);
}

int rpcCacheCall(char* name, int* argTypes, void** args)
{
	return clientHandler.CacheCall(name, argTypes, args);
}

int rpcRegister(char *name, int *argTypes, skeleton f)
{
	return serverHandler.Register(name, argTypes, f);
}

int rpcExecute() {
	return serverHandler.Execute();
}

int rpcTerminate() {
	// HAVE TO AUTHENTICATE THE TERMINATE REQUEST
	return clientHandler.Terminate();
}
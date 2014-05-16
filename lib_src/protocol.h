#ifndef _PROTOCOL_
#define _PROTOCOL_

#include "database.h"
#include <sys/param.h>
#include <stdlib.h>

// Message data structures
struct Register_Msg {
	char* serverId;
	int portNum;
	char *name;
	int *argTypes;
};

struct Location_Req_Msg {
	char *name;
	int *argTypes;
};

struct Location_Rsp_Msg {
	char* serverId;
	int portNum;	
};

struct Execute_Msg {
	char *name;
	int *argTypes;
	void **args;
public:
	Execute_Msg()
	{
		this->args = NULL;
	}
	~Execute_Msg()
	{
		if ( this->args != NULL )
		{
			delete[] this->args;
		}
	}
};

struct Error_Msg {
	int reasonCode;
};

struct Location_Cache_Rsp_Msg {
	int serverCount;
	ServerDetails *serverList;
};

#endif // _PROTOCOL_


#ifndef _Database_H_
#define _Database_H_

#include "common.h"
#include "utility.h"
#include <map>
#include <vector>
#include <sys/param.h>
#include <iostream>

using namespace std;


class ServerDetails
{

public:
	char serverName[MAXHOSTNAMELEN];
	int portNum;

	static ServerDetails * Create(char* serverName, int portNum)
	{
		ServerDetails* serverDetails = new ServerDetails();
		strncpy(serverDetails->serverName, serverName, MAXHOSTNAMELEN);
		serverDetails->portNum = portNum;
		return serverDetails;
	}

	bool Compare(char* serverName, int portNum)
	{
		return (strncmp(this->serverName, serverName, MAXHOSTNAMELEN) == 0)
					&& (this->portNum == portNum);
	}
};

class ServerInfo{
public:
	ServerDetails *serverDetails;
	int fd;
	skeleton f;

	ServerInfo(){};
	static ServerInfo * Create(ServerDetails* serverDetails, int fd, skeleton f)
	{
		ServerInfo *serverInfo = new ServerInfo();
		serverInfo->serverDetails = serverDetails;
		serverInfo->f = f;
		serverInfo->fd = fd;
		return serverInfo;
	}

	~ServerInfo(){};
};

class ServerData{
public:
	int *argTypes;
	vector<ServerInfo *> servers;
	ServerData(){};
	static ServerData * Create(int* argTypes)
	{
		ServerData * serverData = new ServerData();
		serverData->argTypes = argTypes_Copy(argTypes);
		return serverData;
	}

	~ServerData()
	{
		delete[] argTypes;
		for(unsigned int i = 0; i < servers.size(); i++)
		{
			delete servers[i];
		}
	};
};

class Database{

public:

	typedef bool (*matchFunc)(const ServerInfo* first, const ServerInfo* second);

	int AddFunctionEntry(char* functionName, int* argTypes, char* serverName, 
							int portNum, int fd = -1, skeleton f = NULL);
	int GetServer(char* functionName, int* argTypes, ServerInfo *& serverInfo);
	int GetAllServers(char* functionName, int* argTypes, vector<ServerInfo *>*& serversInfo);
	int FindFunction(char * functioName, int* argTypes, ServerData *& serverData);
	int RemoveServer(const ServerInfo* info, matchFunc func);
	int RemoveServerByFd(int fd);
	int RemoveServerByNamePort(char* serverName, int portNum);
	int RemoveServerEntryByFunction(char* functionName, int* argTypes, char* serverName, int portNum);
	int Size(){ return database.size(); }

	skeleton GetSkeleton(char* functionName, int* argTypes);

	Database();
	~Database();

private:

	static bool matchServerNameAndPort(const ServerInfo* first, const ServerInfo* second)
	{
		ServerDetails *one = first->serverDetails;
		ServerDetails *two = second->serverDetails;

		return (strncmp(one->serverName, two->serverName, MAXHOSTNAMELEN) == 0)
				&& (one->portNum == two->portNum);
	}

	static bool matchFd(const ServerInfo* first, const ServerInfo* second)
	{
		return (first->fd == second->fd);
	}

	struct CompareString
	{
	   bool operator()(char const *a, char const *b)
	   {
	      return std::strcmp(a, b) < 0;
	   }
	};

	typedef map<char* , vector<ServerData *> *, CompareString> DBMap;


	DBMap database;
	vector<ServerDetails *> servers;
	int nextServer;

	ServerDetails* FindServer(char *serverName, int portNum);
	void RemoveServerEntry(char *serverName, int portNum);
};


#endif
#include "database.h"
#include "utility.h"
#include "common.h"
#include <iostream>

using namespace std;

Database::Database()
{
	nextServer = 0;
}

Database::~Database()
{
	for(DBMap::iterator it = database.begin();
			it != database.end(); it++)
	{
		delete[] it->first;

		for(unsigned int i = 0; i < (it->second)->size(); i++)
			delete (*(it->second))[i];

		delete it->second;
	}

	for(vector<ServerDetails *>::iterator it = servers.begin();
		it != servers.end(); it++)
	{
		delete *it;
	}

}

/*

	returns following:
	-1 : function name not found
	0 : function name found and argTypes also found, sets serverData
	1 : function name found but argTypes not found

*/
int Database::FindFunction(char * functioName, int* argTypes, ServerData *& serverData)
{
	vector<ServerData *>* serverDataList;
	DBMap::iterator it = database.find(functioName);
	int ret = -1;

	if ( it != database.end() )
	{
		// found function name
		serverDataList = it->second;
		ret = 1;
		for(unsigned int i = 0; i < serverDataList->size(); i++)
		{
			if ( argTypes_Compare((*(serverDataList))[i]->argTypes, argTypes) )
			{
				// args matched
				serverData = (*(serverDataList))[i];
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

ServerDetails* Database::FindServer(char* serverName, int portNum)
{
	for(vector<ServerDetails *>::iterator it = servers.begin();
		it != servers.end(); it++)
	{
		if ( (*it)->Compare(serverName, portNum) )
		{
			return *it;
		}
	}
	return NULL;
}

void Database::RemoveServerEntry(char* serverName, int portNum)
{
	for(vector<ServerDetails *>::iterator it = servers.begin();
		it != servers.end(); it++)
	{
		if ( (*it)->Compare(serverName, portNum) )
		{
			delete *it;
			servers.erase(it);
			break;
		}
	}
}

int Database::RemoveServerEntryByFunction(char* functionName, int* argTypes, char* serverName, int portNum)
{
	DBMap::iterator it = database.find(functionName);
	vector<ServerData *>* serverDataList;
	vector<ServerInfo *>* serversInfo;
	ServerInfo serverInfo;
	ServerDetails *serverDetails = FindServer(serverName, portNum);

	if ( serverDetails == NULL )
	{
		return SUCCESS;
	}

	if ( it != database.end() )
	{
		serverInfo.serverDetails = serverDetails;
		serverDataList = it->second;
		vector<ServerData *>::iterator vt = serverDataList->begin();
		while ( vt != serverDataList->end() )
		{
			if ( argTypes_Compare((*vt)->argTypes, argTypes) )
			{
				serversInfo = &((*vt)->servers);
				vector<ServerInfo *>::iterator st = serversInfo->begin();
				while ( st != serversInfo->end() )
				{
					if ( matchServerNameAndPort(*st, &serverInfo) )
					{
						// Found a server entry, therefore remove it
						delete *st;
						serversInfo->erase(st);
						break;
					}
					st++;
				}

				if ( serversInfo->size() == 0 )
				{
					delete *vt;
					serverDataList->erase(vt);
				}

				break;
			}
			vt++;
		}

		if ( serverDataList->size() == 0 )
		{
			delete serverDataList;
			database.erase(it);
		}
	}

	return SUCCESS;
}

int Database::RemoveServer(const ServerInfo* info, matchFunc func)
{
	vector<ServerData *>* serverDataList;
	vector<ServerInfo *>* serverInfo;
	DBMap::iterator it = database.begin();

	while( it != database.end() )
	{
		serverDataList = it->second;
		vector<ServerData *>::iterator vt = serverDataList->begin();
		while ( vt != serverDataList->end() )
		{
			serverInfo = &((*vt)->servers);
			vector<ServerInfo *>::iterator st = serverInfo->begin();
			while ( st != serverInfo->end() )
			{
				if ( func(*st, info) )
				{
					// Found a server entry, therefore remove it
					delete *st;
					serverInfo->erase(st);
				}
				else
				{
					st++;
				}
			}

			if ( serverInfo->size() == 0 )
			{
				delete *vt;
				serverDataList->erase(vt);
			}
			else
			{
				vt++;
			}
		}

		if ( serverDataList->size() == 0 )
		{
			delete serverDataList;
			database.erase(it);
		}
		
		it++;
	}

	if (info->serverDetails != NULL )
		RemoveServerEntry(info->serverDetails->serverName, info->serverDetails->portNum);

	return SUCCESS;
}

int Database::RemoveServerByNamePort(char* serverName, int portNum)
{
	ServerInfo serverInfo;
	serverInfo.serverDetails = FindServer(serverName, portNum);
	if ( serverInfo.serverDetails == NULL )
		return SUCCESS;

	return RemoveServer(&serverInfo, &matchServerNameAndPort);
}

int Database::RemoveServerByFd(int fd)
{
	ServerInfo serverInfo;
	serverInfo.serverDetails = NULL;
	serverInfo.fd = fd;
	return RemoveServer(&serverInfo, &matchFd);
}

int Database::AddFunctionEntry(char* functionName, int* argTypes, char* serverName, int portNum, int fd, skeleton f)
{
	ServerData* serverData;
	ServerDetails* serverDetails = FindServer(serverName, portNum);
	int retFF = this->FindFunction(functionName, argTypes, serverData);
	int ret = SUCCESS;

	if ( serverDetails == NULL )
	{
		serverDetails = ServerDetails::Create(serverName, portNum);
		servers.push_back(serverDetails);
	}

	if ( retFF == 0 )
	{
		vector<ServerInfo *>& servers = serverData->servers;
		bool found = false;
		for(unsigned int i = 0; i < servers.size(); i++)
		{
			if ( servers[i]->serverDetails->Compare(serverName, portNum ) )
			{
				found = true;
				servers[i]->f = f;
				ret = WARN_DUPLICATION_FUNCTION_REGISTRATION;
				break;
			}
		}

		if ( found == false )
		{
			ServerInfo * serverInfo = ServerInfo::Create(serverDetails, fd, f);
			servers.push_back(serverInfo);
		}
	}
	else if ( retFF == 1 )
	{
		DBMap::iterator it = database.find(functionName);
		vector<ServerData *>* functions = it->second;
		ServerData * serverData = ServerData::Create(argTypes);
		serverData->servers.push_back(ServerInfo::Create(serverDetails, fd, f));
		functions->push_back(serverData);
	}
	else
	{
		char* functionNameTemp = new char[FUNC_NAME_MAX];
		strncpy(functionNameTemp, functionName, FUNC_NAME_MAX);
		vector<ServerData *>* functions = new vector<ServerData *>;
		ServerData * serverData = ServerData::Create(argTypes);
		serverData->servers.push_back(ServerInfo::Create(serverDetails, fd, f));
		functions->push_back(serverData);
		database[functionNameTemp] = functions;
	}

	return ret;
}

/*

	returns following:
	ERR_NO_SERVER_AVAILABLE : if function name or argtypes are missing
	SUCCESS : if successful 


*/

int Database::GetServer(char* functionName, int* argTypes, ServerInfo *& serverInfo)
{
	vector<ServerInfo *>* serversInfo;
	int retFF = GetAllServers(functionName, argTypes, serversInfo);
	int ret = ERR_NO_SERVER_AVAILABLE;

	if ( retFF == SUCCESS )
	{
		/*
		This means there should be at least one server
		entry
		*/
		for(unsigned int i =0; i < servers.size(); i++ )
		{
			for(vector<ServerInfo *>::iterator it = serversInfo->begin();
				it != serversInfo->end(); it++)
			{
				if ( servers[nextServer]->Compare((*it)->serverDetails->serverName,
												(*it)->serverDetails->portNum) )
				{
					nextServer = (nextServer + 1) % servers.size();
					serverInfo = *it;
					return SUCCESS;
				}
			}
			nextServer = (nextServer + 1) % servers.size();
		}
	}

	return ret;
}

/*

	returns following:
	ERR_NO_SERVER_AVAILABLE : if function name or argtypes are missing
	SUCCESS : if successful 

*/

int Database::GetAllServers(char* functionName, int* argTypes, vector<ServerInfo *>*& serversInfo)
{
	ServerData* serverData;
	int retFF = this->FindFunction(functionName, argTypes, serverData);
	int ret = ERR_NO_SERVER_AVAILABLE;

	if ( retFF == 0 )
	{
		serversInfo = &(serverData->servers);
		ret = SUCCESS;
	}

	return ret;	
}

skeleton Database::GetSkeleton(char* functionName, int* argTypes)
{
	ServerData* serverData;
	int retFF = this->FindFunction(functionName, argTypes, serverData);

	if ( retFF == 0 && serverData->servers.size() > 0)
	{
		return serverData->servers[0]->f;
	}
	return NULL;
}
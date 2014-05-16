#include <sys/param.h>
#include <sstream>
#include <string>
#include "common.h"
#include "protocol.h"
#include "rpc_message.h"
#include "utility.h"
#include "database.h"
#include <vector>
#include <iostream>

using namespace std;

void RPCMessage::CreateFailureMessage(int reasonCode, int msgType){
	FreePayload();
	this->payloadLength = sizeof(reasonCode);
	this->msgType = msgType;
	this->payload = new char[ this->payloadLength ];
	*(int *)(this->payload) = reasonCode;
}

void RPCMessage::CreateLocResponseMessage(char *serverId, int portNum ){
	FreePayload();
	this->payloadLength = MAXHOSTNAMELEN + sizeof(portNum);
	this->msgType = REQ_LOC_SUCCESS;
	this->payload = new char[ this->payloadLength ];
	strncpy(this->payload, serverId, MAXHOSTNAMELEN );
	*(int *)(this->payload + MAXHOSTNAMELEN ) = portNum;
}

void RPCMessage::CreateLocRequestMessage(char *name, int *argTypes){
	int argTypesLength = argTypes_Length(argTypes);

	FreePayload();
	this->payloadLength = FUNC_NAME_MAX + (argTypesLength * sizeof(int));
	this->msgType = REQ_LOC_REQUEST;
	this->payload = new char[ this->payloadLength ];
	strncpy(this->payload, name, FUNC_NAME_MAX);
	argTypes_Copy(argTypes, (int *)(this->payload + FUNC_NAME_MAX));
}

void RPCMessage::CreateCacheLocRequestMessage(char* name, int* argTypes)
{
	this->CreateLocRequestMessage(name, argTypes);
	this->msgType = REQ_CACHE_LOC_REQUEST;
}

void RPCMessage::CreateCacheLocResponseMessage(vector<ServerInfo *>& servers)
{
	char *temp;

	FreePayload();
	this->payloadLength = sizeof(int) + (sizeof(ServerDetails) * servers.size());
	this->payload = new char[this->payloadLength];
	*(int *)this->payload = servers.size();
	temp = this->payload + sizeof(int);

	for(vector<ServerInfo *>::iterator it = servers.begin();
		it != servers.end(); it++)
	{
		memcpy(temp, (*it)->serverDetails->serverName, MAXHOSTNAMELEN);
		temp = temp + MAXHOSTNAMELEN;
		memcpy(temp, (void *)(&(*it)->serverDetails->portNum), sizeof(int));
		temp = temp + sizeof(int);
	}
}

void RPCMessage::CreateRegisterMessage(char* serverId, int portNum, char* name, int* argTypes)
{
	int argTypesLength = argTypes_Length(argTypes);

	FreePayload();
	this->payloadLength = MAXHOSTNAMELEN + sizeof(portNum) + FUNC_NAME_MAX + (argTypesLength * sizeof(int));
	this->msgType = REQ_REGISTER;
	this->payload = new char[ this->payloadLength ];
	strncpy(this->payload, serverId, MAXHOSTNAMELEN);
	*(int *)(this->payload + MAXHOSTNAMELEN) = portNum;
	strncpy(this->payload + MAXHOSTNAMELEN + sizeof(portNum), name, FUNC_NAME_MAX);
	argTypes_Copy(argTypes, (int *)(this->payload + MAXHOSTNAMELEN + sizeof(portNum) + FUNC_NAME_MAX));
}

void RPCMessage::CreateExecuteMessage(int msgType, char *name, int *argTypes, void **args ){
	int argTypesLength = argTypes_Length( argTypes ) * sizeof(int);
	int argsLength = args_Length( argTypes );

	FreePayload();
	this->payloadLength = FUNC_NAME_MAX + argTypesLength + argsLength;
	this->msgType = msgType;
	this->payload = new char[ this->payloadLength ];

	strncpy(this->payload, name, FUNC_NAME_MAX);
	argTypes_Copy(argTypes, (int*)(this->payload + FUNC_NAME_MAX));
	args_Copy(argTypes, args, (void *)(this->payload + FUNC_NAME_MAX + argTypesLength));
}

void RPCMessage::CreateTerminateMessage() {

	FreePayload();
	this->payloadLength = 0;
	this->msgType = REQ_TERMINATE;
	this->payload = NULL;
}

Register_Msg RPCMessage::ParseRegisterMessage()
{
	Register_Msg msg;
	msg.serverId = this->payload;
	msg.portNum = *((int *)(this->payload + MAXHOSTNAMELEN));
	msg.name = this->payload + MAXHOSTNAMELEN + sizeof(int);
	msg.argTypes = (int *)(msg.name + FUNC_NAME_MAX);
	return msg;
}

Error_Msg RPCMessage::ParseErrorMessage()
{
	Error_Msg msg;
	if ( this->payloadLength >= sizeof(int))
		msg.reasonCode = *((int *)this->payload);
	return msg;
}

Location_Req_Msg RPCMessage::ParseLocationReqMessage() {

	Location_Req_Msg msg;
	msg.name = this->payload;
	msg.argTypes = (int *)(msg.name + FUNC_NAME_MAX);
	return msg;
}

Location_Rsp_Msg RPCMessage::ParseLocationRspMessage() {

	Location_Rsp_Msg msg;
	msg.serverId = this->payload;
	msg.portNum = *((int *)(msg.serverId + MAXHOSTNAMELEN));
	return msg;
}

Location_Cache_Rsp_Msg RPCMessage::ParseLocationCacheRspMessage()
{
	Location_Cache_Rsp_Msg msg;
	msg.serverCount = *(int *)this->payload;
	if ( msg.serverCount == 0 )
		msg.serverList = NULL;
	else
		msg.serverList = (ServerDetails *)(this->payload + sizeof(int));
	return msg;
}

Execute_Msg RPCMessage::ParseExecuteMessage() {

	Execute_Msg msg;

	msg.name = this->payload;
	msg.argTypes = (int *)(msg.name + FUNC_NAME_MAX);
	msg.args = get_Args(msg.argTypes, msg.argTypes + argTypes_Length(msg.argTypes));

	return msg;
}

void RPCMessage::Copy(RPCMessage& msg)
{
	this->payloadLength = msg.payloadLength;
	this->msgType = msg.msgType;
	this->payload = new char[msg.payloadLength];
	memcpy(this->payload, msg.payload, msg.payloadLength);
}
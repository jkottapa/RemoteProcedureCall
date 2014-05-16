#ifndef _COMMON_H_
#define _COMMON_H_

#include <limits.h>

#define MAX_CLIENTS 5

// buffer constants
#define FUNC_NAME_MAX 100
#define PORT_MAX 5

#define CHECK_BYTES(check)\
	ret = check;\
	if ( ret <= 0 )\
		return ret;


#define CHECKERROR(a, flag)\
	if (a < 0)\
	{\
		if(flag == SEND){\
			return ERR_MESSAGE_SEND_FAILED;\
		}\
		else if(flag == RECV){\
			return ERR_MESSAGE_RECV_FAILED;\
		}\
		return -1;\
	}

#define CHECKARGOUTPUT(arg)\
	(!!((arg) & 1 << ARG_OUTPUT))

#define CHECK(var, cond, value, error)\
	if (var cond value )\
		return error;
	
#define MODULO(x, n) (x%n + n)%n

//Binder request types
// binder/server
#define REQ_REGISTER 1
#define REQ_REGISTER_SUCCESS 2
#define REQ_REGISTER_FAILURE 3
// binder/client
#define REQ_LOC_REQUEST 4
#define REQ_LOC_SUCCESS 5
#define REQ_LOC_FAILURE 6
// server/client
#define REQ_EXECUTE 7
#define REQ_EXECUTE_SUCCESS 8
#define REQ_EXECUTE_FAILURE 9

#define REQ_TERMINATE 10 // get it? Kill - 9

// binder/client cache calls
#define REQ_CACHE_LOC_REQUEST 11
#define REQ_CACHE_LOC_SUCCESS 12
#define REQ_CACHE_LOC_FAILURE 13

//------ ERRORS ----------//

#define SUCCESS 0

// Server init errors
#define ERR_BINDER_ADDRESS_MISSING -300
#define ERR_BINDER_UNREACHABLE -301
#define ERR_SOCKET_LISTENING_FAILED -302

// Server register errors
#define ERR_MESSAGE_SEND_FAILED -310
#define ERR_MESSAGE_RECV_FAILED -311
#define ERR_BINDER_REGISTRATION_FAILED -312
#define ERR_NOT_INITIALIZED -313
#define ERR_COMMUNICATION_FAILED -314
#define ERR_NO_REGISTERED_FUNCITONS -315

// Client call errors
#define ERR_NO_SERVER_AVAILABLE -320
#define ERR_SERVER_UNREACHABLE -321
#define ERR_BINDER_SHUTDOWN -322
#define ERR_SERVER_SHUTDOWN -323

// Server execute errors
#define ERR_SERVER_FUNCTION_RETURNED_ERROR -330

//------ WARNINGS -------//
// Server init warnings
#define WARN_ALREADY_INITIALIZED 300

// Server register warnings
#define WARN_DUPLICATION_FUNCTION_REGISTRATION 310

// Client call warnings

// Server terminate warning
#define WARN_UNAUTHORIZED_TERMINATE_REQUEST 320

// Send and Recv message flags
#define SEND 1
#define RECV 2

#define GENERAL_CHECK 0

#endif //_COMMON_H_


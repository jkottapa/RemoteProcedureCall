#ifndef _BINDER_
#define _BINDER_

#include "../lib_src/tcp.h"
#include "../lib_src/database.h"

class Binder : public TCP {

	protected:

		int ProcessMessage(RPCMessage* recvMessage, RPCMessage* sendMessage, int fd);

		int ProcessReqLocation(RPCMessage& recvMessage, RPCMessage& sendMessage);
		int ProcessReqCacheLocation(RPCMessage& recvMessage, RPCMessage& sendMessage);
		int ProcessRegister(RPCMessage& recvMessage, RPCMessage& sendMessage, int fd);
		int ProcessTerminate(RPCMessage& recvMessage, RPCMessage& sendMessage);

		bool DoExit();
		void Disconnect(int fd);

	private:
		Database functionDB;
		fd_set serverFds;

	public:
		Binder();
		// make sure all deep copy memory is removed
		virtual ~Binder();		
};

#endif //_BINDER_

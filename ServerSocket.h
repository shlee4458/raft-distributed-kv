#ifndef __SERVERSOCKET_H__
#define __SERVERSOCKET_H__

#include <memory>

#include "Socket.h"

class ServerSocket: public Socket {
public:
	ServerSocket() {}
	~ServerSocket() {}

	ServerSocket(int fd, bool nagle_on = NAGLE_ON);

	bool Init(int port);
	std::shared_ptr<ServerSocket> Accept();
};


#endif // end of #ifndef __SERVERSOCKET_H__

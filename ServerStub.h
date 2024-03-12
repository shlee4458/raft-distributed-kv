#ifndef __SERVER_STUB_H__
#define __SERVER_STUB_H__

#include <memory>

#include "ServerSocket.h"
#include "ClientSocket.h"
#include "Messages.h"

class ServerStub {
private:
	std::shared_ptr<ServerSocket> socket;
	
public:
	ServerStub();

	void Init(std::shared_ptr<ServerSocket> socket);

	CustomerRequest ReceiveRequest();
	int ShipLaptop(LaptopInfo info);
	int ReturnRecord(std::shared_ptr<CustomerRecord> record);

	ReplicationRequest ReceiveReplication() const;
	int IdentifySender() const;
	int RespondToPrimary() const;
	int SendIsLeader(int is_leader);
	int SendLeaderInfo(LeaderInfo info);
};

#endif // end of #ifndef __SERVER_STUB_H__

#include "ServerStub.h"

#include <iostream>
#include <deque>

#define ACK 1
#define DEBUG 0

ServerStub::ServerStub() {}

void ServerStub::Init(std::shared_ptr<ServerSocket> socket) {
	this->socket = std::move(socket);
}

CustomerRequest ServerStub::ReceiveRequest() {
	char buffer[32];
	CustomerRequest request;
	if (socket->Recv(buffer, request.Size(), 0)) {
		request.Unmarshal(buffer);
	}
	return request;
}

int ServerStub::ShipLaptop(LaptopInfo info) {
	char buffer[32];
	info.Marshal(buffer);
	return socket->Send(buffer, info.Size(), 0);
}

int ServerStub::ReturnRecord(std::shared_ptr<CustomerRecord> record) {
	char buffer[32];
	record->Marshal(buffer);
	return socket->Send(buffer, record->Size(), 0);
}

int ServerStub::IdentifySender() const {
	// return 1 if it is pfa, 2 if it is customer
	char buffer[4];
	auto identifier = std::shared_ptr<Identifier>(new Identifier());
	if (socket->Recv(buffer, sizeof(int), 0)) {
		identifier->Unmarshal(buffer);
		return identifier->GetIdentifier();
	}
	return 0; // identification failed
}

ReplicationRequest ServerStub::ReceiveReplication() const {
	char buffer[32];
	ReplicationRequest request;
	
	int size = request.Size();
	if (socket->Recv(buffer, size, 0)) {
		if (DEBUG) {
			std::cout << "Replication Received!!!!" << std::endl;
		}
		request.Unmarshal(buffer);
	}
	return request;
}

int ServerStub::RespondToPrimary() const {
	char buffer[4];
	Identifier identifier;
	int size = identifier.Size();
	identifier.SetIdentifier(ACK);
	identifier.Marshal(buffer);
	return socket->Send(buffer, size, 0);
}
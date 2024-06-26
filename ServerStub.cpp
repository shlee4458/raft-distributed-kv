#include "ServerStub.h"

#include <iostream>
#include <deque>

#define ACK 1

#define DEBUG 1

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
	char buffer[4];
	auto identifier = std::shared_ptr<Identifier>(new Identifier());
	if (socket->Recv(buffer, sizeof(int), 0)) {
		identifier->Unmarshal(buffer);
		return identifier->GetIdentifier();
	}
	return 0; // identification failed
}

int ServerStub::SendIsLeader(int is_leader) {
	char buffer[4];
	Identifier identifier;
	int size = identifier.Size();
	identifier.SetIdentifier(is_leader + 1);
	identifier.Marshal(buffer);
	std::cout << "Is leader sent!" << std::endl;
	return socket->Send(buffer, size, 0);	
}

int ServerStub::SendLeaderInfo(LeaderInfo info) {
	char buffer[32];
	info.Marshal(buffer);
	std::cout << "Leader into sent!" << std::endl;
	return socket->Send(buffer, info.Size(), 0);
}

RequestVoteMessage ServerStub::RecvRequestVote() {
	char buffer[32];
	RequestVoteMessage msg;
	if (socket->Recv(buffer, msg.Size(), 0)) {
		msg.Unmarshal(buffer);
		return msg;
	}
	return msg;
}

int ServerStub::SendVoteResponse(RequestVoteResponse res) {
	char buffer[32];
	res.Marshal(buffer);
	int size = res.Size();
	return socket->Send(buffer, size, 0);
}

int ServerStub::IdentifyRPC() {
	char buffer[4];
	Identifier identifier;
	int size = identifier.Size();
	socket->Recv(buffer, size, 0);
	// if (!socket->Recv(buffer, size, 0)) { // if the server dies
	// 	return -1;
	// }
	identifier.Unmarshal(buffer);
	return identifier.GetIdentifier();
}

LogRequest ServerStub::RecvLogRequest() {
	char buffer[64];
	LogRequest request;
	
	int size = request.Size();
	if (socket->Recv(buffer, size, 0)) {
		request.Unmarshal(buffer);
	}
	return request;
}

int ServerStub::SendLogResponse(LogResponse log_res) {
	char buffer[32];
	log_res.Marshal(buffer);
	int size = log_res.Size();
	return socket->Send(buffer, size, 0);
}
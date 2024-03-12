#include "ClientStub.h"

ClientStub::ClientStub() {}

int ClientStub::Init(std::string ip, int port) {
	return socket.Init(ip, port);	
}

LaptopInfo ClientStub::Order(CustomerRequest request) {
	LaptopInfo info;
	char buffer[32];
	int size;
	request.Marshal(buffer);
	size = request.Size();
	if (socket.Send(buffer, size, 0)) {
		size = info.Size();
		if (socket.Recv(buffer, size, 0)) {
			info.Unmarshal(buffer);
		} 
	}
	return info;
}

CustomerRecord ClientStub::ReadRecord(CustomerRequest request) {
	CustomerRecord record;
	char record_buffer[32];
	int record_size;
	request.Marshal(record_buffer);
	record_size = request.Size();

	if (socket.Send(record_buffer, record_size, 0)) {
		record_size = record.Size();
		if (socket.Recv(record_buffer, record_size, 0)) { // receive the record
			record.Unmarshal(record_buffer);
		}
	}
	return record;
}

int ClientStub::SendIdentifier(Identifier identifier) {
	char buffer[4];
	char leader_buffer[4];
	Identifier is_leader;

	identifier.Marshal(buffer);
	if (socket.Send(buffer, identifier.Size(), 0)) {
		if (socket.Recv(leader_buffer, 4, 0)) {
			is_leader.Unmarshal(leader_buffer);
			return is_leader.GetIdentifier();
		}
	}
}

LeaderInfo ClientStub::RecvLeaderInfo() {
	char buffer[32];
	LeaderInfo info;
	int size = info.Size();
	if (socket.Recv(buffer, size, 0)) {
		info.Unmarshal(buffer);
		return info;
	}
}

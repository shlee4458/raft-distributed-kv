#include "ClientThread.h"

#include <iostream>

#define UPDATE_REQUEST 1
#define READ_REQUEST 2
#define DEBUG 3
#define CLIENT_IDENTIFIER 2
#define LAPTOP_DEFAULT -1
#define RECORD_DEFAULT -2
#define FOLLOWER 1
#define LEADER 2

ClientThreadClass::ClientThreadClass() {}

void ClientThreadClass::
ThreadBody(std::string ip, int port, int customer_id, int num_requests, int request_type) {
	CustomerRequest request;
	LaptopInfo laptop;
	CustomerRecord record;
	Identifier identifier;
	int is_leader;

	this->customer_id = customer_id;
	this->num_requests = num_requests;
	this->request_type = request_type;

	if (!stub.Init(ip, port)) {
		std::cout << "Thread " << customer_id << " failed to connect" << std::endl;
		return;
	}

	// send the one-time identifier first
	identifier.SetIdentifier(CLIENT_IDENTIFIER);
	stub.SendIdentifier(identifier);
	is_leader = stub.RecvIsLeader();
	switch (is_leader)
	{
		case LEADER:
			break;
		case FOLLOWER:
			// receive the ip, port from the follower
			LeaderInfo info;
			info = stub.RecvLeaderInfo();
			ip = info.GetIp();
			port = info.GetPort();

			if (!stub.Init(ip, port)) {
				std::cout << "Thread " << customer_id << " failed to connect" << std::endl;
				return;
			}			
			break;
		default:
			std::cout << "identifier not sent" << std::endl;
			break;
	}

	for (int i = 0; i < num_requests; i++) {
		timer.Start();
		// based on the request_type, call different RPC
		switch (request_type) {
			case UPDATE_REQUEST:

				request.SetRequest(customer_id, i, UPDATE_REQUEST);
				laptop = stub.Order(request);

				// Primary server failure; exit gracefully
				if (laptop.GetCustomerId() == LAPTOP_DEFAULT) {
					std::cout << "Primary server went down, graceuflly exiting" << std::endl;
					return;
				}
				break;
			case READ_REQUEST:
			case DEBUG:
				// change the request record to 2
				request.SetRequest(i, -1, READ_REQUEST);
				record = stub.ReadRecord(request);
				if (request_type == DEBUG && record.IsValid()) {
					record.Print();
				}

				// Backup server failure; exit gracefully
				if (record.GetCustomerId() == RECORD_DEFAULT) {
					std::cout << "Server went down, graceuflly exiting" << std::endl;
					return;
				}
				break;
			default:
				break;
		}
		timer.EndAndMerge();
	}
}

ClientTimer ClientThreadClass::GetTimer() {
	return timer;	
}
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
	LeaderInfo info;
	int is_leader;
	int exit_requested = false;

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
	std::cout << "Is it leader?: "<< is_leader << std::endl;

	// if the request type is update and is not leader, reopen the socket with the leader
	if (is_leader == FOLLOWER) {
		std::cout << "It is not the leader!" << std::endl;
		info = stub.RecvLeaderInfo();
		ip = info.GetIp();
		port = info.GetPort();

		// if it is update request, reroute to the new server
		if (request_type == UPDATE_REQUEST) {
			if (!stub.Init(ip, port)) {
				std::cout << "Thread " << customer_id << " failed to connect" << std::endl;
				return;
			}
			stub.SendIdentifier(identifier);
			is_leader = stub.RecvIsLeader();
			std::cout << "Connected with the new leader!" << std::endl;
		}
		
	} else if (is_leader == 0) {
		std::cout << "There is no leader! If your order is update, send request again in 1 minute.." << std::endl;
	}
	
	for (int i = 0; i < num_requests; i++) {
		timer.Start();
		// based on the request_type, call different RPC
		switch (request_type) {
			case UPDATE_REQUEST:
				if (!is_leader) {
					return;
				}

				std::cout << "Sending Update Request" << std::endl;
				request.SetRequest(customer_id, i, UPDATE_REQUEST);
				laptop = stub.Order(request);
				laptop.Print();

				// Primary server failure; exit gracefully
				if (laptop.GetCustomerId() == LAPTOP_DEFAULT) {
					std::cout << "Primary server went down, graceuflly exiting" << std::endl;
					exit_requested = true;
				}
				break;
			case READ_REQUEST:
			case DEBUG:
				// change the request record to 2
				std::cout << "Sending the read request!" << std::endl;
				request.SetRequest(i, -1, READ_REQUEST);
				record = stub.ReadRecord(request);
				if (request_type == DEBUG && record.IsValid()) {
					record.Print();
				}

				// Backup server failure; exit gracefully
				if (record.GetCustomerId() == RECORD_DEFAULT) {
					std::cout << "Server went down, graceuflly exiting" << std::endl;
					exit_requested = true;
				}
				break;
			default:
				break;
		}
		timer.EndAndMerge();

		if (exit_requested) {
			return;
		}
	}
}

ClientTimer ClientThreadClass::GetTimer() {
	return timer;	
}
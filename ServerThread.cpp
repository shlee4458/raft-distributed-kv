#include <iostream>
#include <memory>
#include <map>
#include <random>

#include "ServerThread.h"

#define SERVER_IDENTIFIER 3
#define CUSTOMER_IDENTIFIER 2

#define UPDATE_REQUEST 1
#define READ_REQUEST 2

#define FOLLOWER 1
#define LEADER 2
#define CANDIDATE 3
#define HEARTBEAT_TIME 100

#define REQUESTVOTE_RPC 0
#define APPENDLOG_RPC 1

#define DEBUG 1

LaptopInfo LaptopFactory::
GetLaptopInfo(CustomerRequest request, int engineer_id) {
	LaptopInfo laptop;
	laptop.CopyRequest(request);
	laptop.SetEngineerId(engineer_id);
	laptop.SetAdminId(-1);
	return laptop;
}

LaptopInfo LaptopFactory::
CreateLaptop(CustomerRequest request, int engineer_id, std::shared_ptr<ServerStub> stub) {
	LaptopInfo laptop;
	laptop.CopyRequest(request);
	laptop.SetEngineerId(engineer_id);

	std::promise<LaptopInfo> prom;
	std::future<LaptopInfo> fut = prom.get_future();

	std::shared_ptr<PrimaryAdminRequest> req = 
		std::shared_ptr<PrimaryAdminRequest>(new PrimaryAdminRequest);
	req->laptop = laptop;
	req->prom = std::move(prom);
	req->stub = stub;

	erq_lock.lock();
	erq.push(std::move(req));
	erq_cv.notify_one();
	erq_lock.unlock();

	laptop = fut.get();
	return laptop;
}

/**
 * Entrance to the engineer thread.
*/
void LaptopFactory::
EngineerThread(std::shared_ptr<ServerSocket> socket, 
				int engieer_id, 
				std::shared_ptr<ServerMetadata> metadata) {
	
	int sender;
	this->metadata = metadata;
	auto stub = std::make_shared<ServerStub>(); // stub is only destroyed when the factory goes out of scope
	stub->Init(std::move(socket));
	sender = stub->IdentifySender();

	{
		std::unique_lock<std::mutex> sl(stub_lock);
		stubs.push_back(stub);
	}
	
	while (true) {
		switch (sender) {
			case SERVER_IDENTIFIER:
				if (DEBUG) {
					std::cout << "Received a message from another server!!" << std::endl;
				}
				ServerHandler(std::move(stub));
				break;
			case CUSTOMER_IDENTIFIER:
				if (DEBUG) {
					std::cout << "I have received a message from a customer!" << std::endl;
				}
				CustomerHandler(engieer_id, std::move(stub));
				if (DEBUG) {
					std::cout << "CONNECTION WITH THE CLIENT HAS BEEN TERMINATED" << std::endl;
				}
				break;
			default:
				break;
		}
	}
}

void LaptopFactory::ServerHandler(std::shared_ptr<ServerStub> stub) {
	int rpc;
	while (true) {
		// check if the RPC is vote request or append log request
		rpc = stub->IdentifyRPC();
		switch (rpc)
		{
			case REQUESTVOTE_RPC:
				std::cout << "Candidate Vote RPC Received!" << std::endl;
				CandidateVoteHandler(stub);
				break;
			
			case APPENDLOG_RPC:
				std::cout << "Append log RPC Received!" << std::endl;
				AppendLogHandler(stub);
				break;
		}
	}
}

void LaptopFactory::CandidateVoteHandler(std::shared_ptr<ServerStub> stub) {

	std::unique_lock<std::mutex> ml(meta_lock, std::defer_lock);

	// receive the request vote message
	RequestVoteMessage msg;
	msg = stub->RecvRequestVote();

	// get the vote response to send
	ml.lock();
	RequestVoteResponse res = metadata->GetVoteResponse(msg);
	ml.unlock();

	// send vote response
	stub->SendVoteResponse(res);
}

int LaptopFactory::AppendLogHandler(std::shared_ptr<ServerStub> stub) {

	// get the LogRequest instance from the leader
	LogRequest request;
	while (true) {
		request = stub->RecvLogRequest();
		
		// check if the log request received is valid, and get LogRequestResponse with the info

		std::shared_ptr<FollowerRequest> follower_req = 
			std::shared_ptr<FollowerRequest>(new FollowerRequest);

		follower_req->log_request = request;
		follower_req->stub = stub;

		rep_lock.lock();
		req.push(std::move(follower_req));
		rep_cv.notify_one();
		rep_lock.unlock();
	}
}

void LaptopFactory::CustomerHandler(int engineer_id, std::shared_ptr<ServerStub> stub) {

	std::unique_lock<std::mutex> ml(meta_lock, std::defer_lock);
	std::shared_ptr<CustomerRecord> entry;
	CustomerRequest request;
	LaptopInfo laptop;
	int request_type, customer_id, order_num;

	// let the customer know if leader or not
	stub->SendIsLeader(metadata->IsLeader());

	if (!metadata->IsLeader()) {
		// tell the client that the order to be sent to this
		// INVARIABLE: there is always a leader when the client is sending an update request
		std::string ip = metadata->GetLeaderIp();
		int port = metadata->GetLeaderPort();
		LeaderInfo info;
		info.SetLeaderInfo(ip, port);
		stub->SendLeaderInfo(info);
	}

	while (true) {
		request = stub->ReceiveRequest();
		if (!request.IsValid()) {
			return;
		}
		request_type = request.GetRequestType();
		switch (request_type) {
			case UPDATE_REQUEST:
				laptop = CreateLaptop(request, engineer_id, stub);
				stub->ShipLaptop(laptop);
				break;
			case READ_REQUEST:
				laptop = GetLaptopInfo(request, engineer_id);
				customer_id = laptop.GetCustomerId();
				if (DEBUG) {
					std::cout << "Received a READ REQUEST for: " << customer_id << std::endl;
				}
				order_num = ReadRecord(customer_id);
				entry = std::shared_ptr<CustomerRecord>(new CustomerRecord());
				entry->SetRecord(customer_id, order_num);
				entry->Print();
				stub->ReturnRecord(std::move(entry));
				break;
			default:
				std::cout << "Undefined Request: "
					<< request_type << std::endl;
		}
	}
}

int LaptopFactory::
ReadRecord(int customer_id) {
	// no synchronization issue; one thread for the client read operation
	return metadata->GetValue(customer_id);
}

void LaptopFactory::LeaderThread(int id) {
	std::unique_lock<std::mutex> ul(erq_lock, std::defer_lock), 
								 ml(meta_lock, std::defer_lock);
	std::shared_ptr<ServerStub> stub;
	int customer_id, order_num;

	while (true) {
		ul.lock();
		if (erq.empty()) {
			erq_cv.wait(ul, [this]{ return !erq.empty(); });
		}
		auto req = std::move(erq.front());
		erq.pop();
		ul.unlock();

		// get the customer_id and order_num from the request
		customer_id = req->laptop.GetCustomerId();
		order_num = req->laptop.GetOrderNumber();
		stub = req->stub;
		
		// update the record and set the adminid
		req->laptop.SetAdminId(id);
		req->prom.set_value(req->laptop);

		ml.lock();
		LeaderMaintainLog(customer_id, order_num, stub); 
		ml.unlock();
	}
}

void LaptopFactory::FollowerThread() {
	std::unique_lock<std::mutex> rl(rep_lock, std::defer_lock), 
								 ml(meta_lock, std::defer_lock),
								 tl(timeout_lock, std::defer_lock);
	std::shared_ptr<ServerStub> stub;
	LogResponse log_res;

	int req_leader_id, req_current_term, req_prefix_length, req_prefix_term;
	int req_commit_length, req_op_term, req_op_arg1, req_op_arg2;
	int current_term = metadata->GetCurrentTerm();
	int factory_id = metadata->GetFactoryId();

	bool can_log, included;

	while (true) {
		rl.lock();
		if (req.empty()) {
			rep_cv.wait(rl, [this]{ return !req.empty(); });
		}
		if (DEBUG) {
			std::cout << "Successfully received the replication request" << std::endl;
		}
		auto request = std::move(req.front());
		req.pop();
		rl.unlock();

		// get the log response based on the log request
		ml.lock();
		log_res = metadata->GetLogResponse(request->log_request);
		ml.unlock();

		// send the log response to the leader
		stub = request->stub;
		stub->SendLogResponse(log_res);

		if (DEBUG) {
			std::cout << "I have responded to the primary!" << std::endl;
		}
	}
}

void LaptopFactory::TimeoutThread() {
	int timeout, current_term;
	std::unique_lock<std::mutex> tl(timeout_lock, std::defer_lock);

	// if the current state is;
	while (true) {
		switch (metadata->GetStatus())
			{
				current_term = metadata->GetCurrentTerm();
				timeout = GetRandomTimeout();
				case FOLLOWER:
					std::cout << "Current term: " << current_term << " " << "- " << "Follower" << std::endl;
					timeout_cv.wait_for(tl, std::chrono::milliseconds(timeout), [&]{ return metadata->GetHeartbeat(); });

					if (heartbeat) {
						std::cout << "Heartbeat was received!" << std::endl;
						metadata->SetHeartbeat(false);
					} else {
						std::cout << "I became a candidate!" << std::endl;
						metadata->SetStatus(CANDIDATE);
					}
					break;

				case CANDIDATE:
					std::cout << "Current term: " << current_term << " " << "- " << "Candidate" << std::endl;
					metadata->RequestVote();
					timeout_cv.wait_for(tl, std::chrono::milliseconds(timeout), // vote time outs
											[&]{ return metadata->GetStatus() == LEADER // elected as the leader
													 || metadata->GetStatus() == FOLLOWER; }); // found leader
				case LEADER:
					// for every 100ms send replicatelog
					std::cout << "Current term: " << current_term << " " << "- " << "Leader" << std::endl;
					metadata->ReplicateLog();
					timeout_cv.wait_for(tl, std::chrono::milliseconds(HEARTBEAT_TIME), 
											[&]{ return metadata->GetStatus() == FOLLOWER; });
					break;
			}
			
			// follower; give random 150~300ms timeout
				// if LogRequest was received, reset the timer

			// candidate; give random 150~300ms timeout
				// if LogRequest was received and it is valid
				// change the current state to the follower

				// if timeout without change of state
				// send RequestVoteRPC

			// leader; send heartbeat to all the followers
				// if LogRequest was received and it is valid
				// change the current state to the follower

				// Send heartbeat in every 100ms
	}
}

int LaptopFactory::GetRandomTimeout() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(150, 300);
	return dist(gen);
}

void LaptopFactory::
LeaderMaintainLog(int customer_id, int order_num, const std::shared_ptr<ServerStub>& stub) {
	
	int valid_replicate, prev_last_idx, prev_commited_idx, current_term;
	MapOp op;
	current_term = metadata->GetCurrentTerm();
	op.arg1 = customer_id;
	op.arg2 = order_num;
	
	// // CONSIDER: if it was a follower, should I execute the last?; no

	// append the record(message, current term) to the log
	metadata->AppendLog(current_term, customer_id, order_num);

	// set the ack to the log_size
	metadata->SetAckLength(-1, metadata->GetLogSize());

	// send replicate log message to all of the neighbor nodes
	valid_replicate = metadata->ReplicateLog();
	return;
}
#include <iostream>
#include <memory>
#include <map>
#include <random>

#include "ServerThread.h"

#define PFA_IDENTIFIER 1
#define CUSTOMER_IDENTIFIER 2
#define CANDIDATE_IDENTIFIER 3
#define UPDATE_REQUEST 1
#define READ_REQUEST 2

#define FOLLOWER 1
#define LEADER 2
#define CANDIDATE 3
#define HEARTBEAT_TIME 100

#define DEBUG 0

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
			case PFA_IDENTIFIER:
				if (DEBUG) {
					std::cout << "I have received a message from the Primary server!" << std::endl;
				}
				PfaHandler(std::move(stub));
				if (DEBUG) {
					std::cout << "CONNECTION WITH THE SERVER HAS BEEN TERMINATED" << std::endl;
				}
				return;
				break;
			case CUSTOMER_IDENTIFIER:
				if (DEBUG) {
					std::cout << "I have received a message from a customer!" << std::endl;
				}
				CustomerHandler(engieer_id, std::move(stub));
				if (DEBUG) {
					std::cout << "CONNECTION WITH THE CLIENT HAS BEEN TERMINATED" << std::endl;
				}
				return;
				break;
			case CANDIDATE_IDENTIFIER:
				if (DEBUG) {
					std::cout << "Received a vote request from a Candidate!!" << std::endl;
				}
				CandidateVoteHandler(std::move(stub));
			default:
				return;
				break;
		}
	}
}

void LaptopFactory::CandidateVoteHandler(std::shared_ptr<ServerStub> stub) {

	std::unique_lock<std::mutex> ml(meta_lock, std::defer_lock);

	// wait for the message to be sent
	RequestVoteMessage msg;
	msg = stub->RecvRequestVote();
	int cand_id = msg.GetId();
	int cand_current_term = msg.GetCurrentTerm();
	int cand_log_size = msg.GetLogSize();
	int cand_last_term = msg.GetLastTerm();

	// if more higher term candidate vote is received
	ml.lock();
	if (cand_current_term > metadata->GetCurrentTerm()) {
		metadata->SetCurrentTerm(cand_current_term);
		metadata->SetStatus(FOLLOWER);
		metadata->SetVotedFor(cand_id);
	}
	
	int last_term = metadata->GetLastTerm();
	int log_size = metadata->GetLogSize();
	int current_term = metadata->GetCurrentTerm();
	bool voted_for = metadata->GetVotedFor();
	
	bool valid = (cand_last_term > last_term) || 
				((cand_last_term == last_term) && cand_log_size >= log_size);

	RequestVoteResponse res;

	if (valid && cand_current_term == current_term && voted_for) {
		res.SetRequestVoteResponse(metadata->GetFactoryId(), current_term, true);
	} else {
		res.SetRequestVoteResponse(metadata->GetFactoryId(), current_term, false);
	}
	ml.unlock();

	stub->SendVoteResponse(res);

	// if the vote received is a valid, 
		// change the status to the follower
		// set voted for to the id

	// if not valid,
		// vote against
}

bool LaptopFactory::PfaHandler(std::shared_ptr<ServerStub> stub) {

	ReplicationRequest request;
	while (true) {
		request = stub->ReceiveReplication();
		
		// Primary Failure: set the leader_id to -1
		if (!request.IsValid()) {
			metadata->SetLeaderId(-1);
			std::cout << "Primary Server went down, gracefully exiting" << std::endl;
			return 0;
		}

		std::shared_ptr<IdleAdminRequest> idle_req = 
			std::shared_ptr<IdleAdminRequest>(new IdleAdminRequest);

		idle_req->repl_request = request;
		idle_req->stub = stub;

		rep_lock.lock();
		req.push(std::move(idle_req));
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

void LaptopFactory::PrimaryAdminThread(int id) {
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
		PrimaryMaintainLog(customer_id, order_num, stub); 
		ml.unlock();
	}
}

void LaptopFactory::IdleAdminThread(int id) {
	std::unique_lock<std::mutex> rl(rep_lock, std::defer_lock), 
								 ml(meta_lock, std::defer_lock);
	std::shared_ptr<ServerStub> stub;

	int last_idx, committed_idx, leader_id;
	int customer_id, order_num;

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

		// get the information
		last_idx = request->repl_request.GetLastIdx();
		committed_idx = request->repl_request.GetCommitedIdx();
		leader_id = request->repl_request.GetLeaderId();
		customer_id = request->repl_request.GetArg1();
		order_num = request->repl_request.GetArg2();
		stub = request->stub;
		if (DEBUG) {
			std::cout << request << std::endl;
		}
		
		// check if the current server was the primary
		bool was_primary = metadata->IsLeader();

		// update the metadata; commited index, last index
		if (was_primary) {
			metadata->SetLeaderId(leader_id);
			if (DEBUG) {
				std::cout << "I have set the primary id to: " << leader_id << std::endl;
			}
		}
		ml.lock();
		IdleMaintainLog(customer_id, order_num, last_idx, committed_idx, was_primary);
		ml.unlock();

		// send to the primary that the log update is complete
		stub->RespondToPrimary();
		if (DEBUG) {
			std::cout << "I have responded to the primary!" << std::endl;
		}
	}
}


void LaptopFactory::TimeoutThread() {
	int timeout;
	std::unique_lock<std::mutex> tl(timeout_lock, std::defer_lock);

	// if the current state is;
	while (true) {
		switch (metadata->GetStatus())
			{
				timeout = GetRandomTimeout();
				case FOLLOWER:
					timeout_cv.wait_for(tl, std::chrono::milliseconds(timeout), [&]{ return heartbeat; });

					if (heartbeat) {
						std::cout << "Heartbeat was received!" << std::endl;
						heartbeat = false;
					} else {
						metadata->SetStatus(CANDIDATE);
					}
					break;

				case CANDIDATE:
					// vote for itself
					// request vote
					metadata->RequestVote();
					timeout_cv.wait_for(tl, std::chrono::milliseconds(timeout), // vote time outs
											[&]{ return metadata->GetStatus() == LEADER // elected as the leader
													 || metadata->GetStatus() == FOLLOWER; }); // found leader
				case LEADER:
					// for every 100ms send replicatelog
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
PrimaryMaintainLog(int customer_id, int order_num, const std::shared_ptr<ServerStub>& stub) {
	
	int response_received, prev_last_idx, prev_commited_idx;
	MapOp op;
	op.term = 1;
	op.arg1 = customer_id;
	op.arg2 = order_num;
	prev_last_idx = metadata->GetLastIndex();
	prev_commited_idx = metadata->GetCommittedIndex();

	// if it was previously an idle server, execute the last log
	if (prev_last_idx != prev_commited_idx) {
		metadata->ExecuteLog(prev_last_idx);
	}

	metadata->AppendLog(op);
	response_received = metadata->SendReplicationRequest(op);

	// if the number of response_received does not match the neighbor size
	if (!response_received) {
		return; // return without executing the log
	}

	// execute log at the last index, and update the committed_index
	metadata->ExecuteLog(metadata->GetLastIndex());
	return;
}

void LaptopFactory::
IdleMaintainLog(int customer_id, int order_num, int req_last, int req_committed, bool was_primary) {

	MapOp op;
	op.term = 1;
	op.arg1 = customer_id;
	op.arg2 = order_num;

	// append the new log and update the last_index
	metadata->AppendLog(op);

	// if it is not the case that the server was primary or no mapop is found
	if (req_committed >= 0 && !was_primary) {
		metadata->ExecuteLog(req_committed);
	}
}
#include <iostream>
#include <memory>
#include <map>
#include <random>

#include "ServerThread.h"

#define SERVER_IDENTIFIER 1
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

LaptopFactory::LaptopFactory(std::shared_ptr<ServerMetadata> metadata) {
	this->metadata = metadata;
}

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

	std::shared_ptr<LeaderRequest> req = 
		std::shared_ptr<LeaderRequest>(new LeaderRequest);
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
				int engieer_id) {
	
	int sender;
	auto stub = std::make_shared<ServerStub>(); // stub is only destroyed when the factory goes out of scope
	stub->Init(std::move(socket));
	sender = stub->IdentifySender(); // A

	{
		std::unique_lock<std::mutex> sl(stub_lock);
		stubs.push_back(stub);
	}
	
	while (true) {
		switch (sender) {
			case SERVER_IDENTIFIER:
				if (DEBUG) {
					// std::cout << "Received a message from another server!!" << std::endl;
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
				return;
				break;
			default:
				break;
		}
	}
}

void LaptopFactory::ServerHandler(std::shared_ptr<ServerStub> stub) {
	std::unique_lock<std::mutex> ml(meta_lock, std::defer_lock);
	int rpc;
	while (true) {
		// check if the RPC is vote request or append log request
		rpc = stub->IdentifyRPC();
		switch (rpc)
		{
			case REQUESTVOTE_RPC:
				std::cout << "Candidate Vote RPC Received!" << std::endl;
				ml.lock();
				CandidateVoteHandler(stub);
				ml.unlock();
				break;
			
			case APPENDLOG_RPC:
				// std::cout << "Append log RPC Received!" << std::endl;
				ml.lock();
				AppendLogHandler(stub);
				ml.unlock();
				break;
		}
	}
}

void LaptopFactory::CandidateVoteHandler(std::shared_ptr<ServerStub> stub) {

	// receive the request vote message
	RequestVoteMessage msg;
	msg = stub->RecvRequestVote();
	// msg.Print();

	// get the vote response to send
	RequestVoteResponse res = metadata->GetVoteResponse(msg);

	// send vote response
	stub->SendVoteResponse(res);
	std::cout << "Vote Response sent" << std::endl;
}

int LaptopFactory::AppendLogHandler(std::shared_ptr<ServerStub> stub) {

	LogRequest request;
	LogResponse log_res;
	
	request = stub->RecvLogRequest();
	log_res = metadata->GetLogResponse(request);
	stub->SendLogResponse(log_res);
	return 1;
}

void LaptopFactory::CustomerHandler(int engineer_id, std::shared_ptr<ServerStub> stub) {

	std::unique_lock<std::mutex> ml(meta_lock, std::defer_lock);
	std::shared_ptr<CustomerRecord> entry;
	CustomerRequest request;
	LaptopInfo laptop;
	int request_type, customer_id, order_num;

	// let the customer know if leader or not
	stub->SendIsLeader(metadata->IsLeader()); // B

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
			std::cout << "I sent log response to the leader!" << std::endl;
		}
	}
}

void LaptopFactory::TimeoutThread() {
	int timeout, current_term;
	bool heartbeat;
	std::unique_lock<std::mutex> tl(timeout_lock, std::defer_lock),
								 ml(meta_lock, std::defer_lock);

	// if the current state is;
	while (true) {
		current_term = metadata->GetCurrentTerm();
		timeout = GetRandomTimeout();
		switch (metadata->GetStatus())
			{
				case FOLLOWER:
					// std::cout << "Current term: " << current_term << " - " << "Follower" << std::endl;
					tl.lock();
					timeout_cv.wait_for(tl, std::chrono::milliseconds(timeout), [&]{ return metadata->GetHeartbeat(); });
					tl.unlock();

					ml.lock();
					heartbeat = metadata->GetHeartbeat();
					if (heartbeat) {
						// std::cout << "Heartbeat was received!" << std::endl;
						metadata->SetHeartbeat(false);
					} else {
						std::cout << "I became a candidate!" << std::endl;
						metadata->SetStatus(CANDIDATE);
					}
					ml.unlock();
					break;

				case CANDIDATE:
					std::cout << "Current term: " << current_term << " - " << "Candidate" << std::endl;
					ml.lock();
					metadata->RequestVote();
					ml.unlock();
					tl.lock();
					timeout_cv.wait_for(tl, std::chrono::milliseconds(timeout), // vote time outs
											[&]{ return metadata->GetStatus() == LEADER // elected as the leader
													 || metadata->GetStatus() == FOLLOWER; }); // found leader
					tl.unlock();
					break;
				case LEADER:
					// for every 100ms send replicatelog
					// std::cout << "Current term: " << current_term << " - " << "Leader" << std::endl;
					ml.lock();
					metadata->ReplicateLog(true);
					ml.unlock();
					tl.lock();
					timeout_cv.wait_for(tl, std::chrono::milliseconds(HEARTBEAT_TIME), 
											[&]{ return metadata->GetStatus() == FOLLOWER; });
					tl.unlock();
					break;
			}
	}
}

void LaptopFactory::
LeaderMaintainLog(int customer_id, int order_num, const std::shared_ptr<ServerStub>& stub) {
	
	int valid_replicate, current_term;
	current_term = metadata->GetCurrentTerm();
	
	// append the record(message, current term) to the log
	metadata->AppendLog(current_term, customer_id, order_num);

	// set the ack to the log_size
	metadata->SetAckLength(-1, metadata->GetLogSize());

	// send replicate log message to all of the neighbor nodes
	valid_replicate = metadata->ReplicateLog(false);
	if (!valid_replicate) {
		std::cout << "It was not a valid replicate!" << std::endl;
	}
	return;
}

int LaptopFactory::GetRandomTimeout() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(150, 300);
	return dist(gen);
}

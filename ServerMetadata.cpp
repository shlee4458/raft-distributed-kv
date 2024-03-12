#include "ServerMetadata.h"
#include "Messages.h"

#include <string.h>
#include <iostream>

#define PFA_IDENTIFIER 1
#define CANDIDATE_IDENTIFIER 3
#define DEBUG 0

ServerMetadata::ServerMetadata() 
: last_idx(-1), committed_idx(-1), leader_id(-1), factory_id(-1), neighbors(), voted_for(-1), current_term(0) { }

int ServerMetadata::GetLeaderId() {
    return leader_id;
}

int ServerMetadata::GetFactoryId() {
    return factory_id;
}

int ServerMetadata::GetCommittedIndex() {
    return committed_idx;
}

int ServerMetadata::GetLastIndex() {
    return last_idx;
}

std::vector<std::shared_ptr<ServerNode>> ServerMetadata::GetNeighbors() {
    return neighbors;
}

int ServerMetadata::GetPeerSize() {
    return neighbors.size();
}

std::deque<std::shared_ptr<ClientSocket>> ServerMetadata::GetNeighborSockets() {
    return neighbor_sockets;
}

std::vector<MapOp> ServerMetadata::GetLog() {
    return smr_log;
}

MapOp ServerMetadata::GetOp(int idx) {
    return smr_log[idx];
}

int ServerMetadata::GetValue(int customer_id) {
    auto it = customer_record.find(customer_id);
    if (it != customer_record.end()) { // found the key, return the value
        return customer_record[customer_id];
    } else { // key not found, return -1
        std::cout << "Key not found!" << std::endl;
        return -1; 
    }
}

ReplicationRequest ServerMetadata::GetReplicationRequest(MapOp op) {
    int op_code = op.term;
    int op_arg1 = op.arg1;
    int op_arg2 = op.arg2;
    return ReplicationRequest(last_idx, committed_idx, leader_id, op_code, op_arg1, op_arg2);
}

void ServerMetadata::SetLeaderId(int id) {
	leader_id = id;
    return;
}

void ServerMetadata::SetFactoryId(int id) {
    factory_id = id;
    return;
}

void ServerMetadata::UpdateLastIndex(int idx) {
    last_idx = idx;
    return;
}

void ServerMetadata::UpdateCommitedIndex(int idx) {
    committed_idx = idx;
    return;
}

void ServerMetadata::AppendLog(MapOp op) {
    smr_log.push_back(op);
    last_idx++;
    return;
}

void ServerMetadata::ExecuteLog(int idx) {
    int customer_id, order_num;
    
    MapOp op = GetOp(idx);
    customer_id = op.arg1;
    order_num = op.arg2;

    customer_record[customer_id] = order_num;
    if (DEBUG) {
        std::cout << "Record Updated for client: " << customer_id 
            << " Order Num: " << order_num << std::endl;
    } 
    committed_idx++;
    return;
}

bool ServerMetadata::IsLeader() {
    return leader_id == factory_id;
}

void ServerMetadata::AddNeighbors(std::shared_ptr<ServerNode> node) {
    neighbors.push_back(std::move(node));
}

void ServerMetadata::InitNeighbors() {

    // corner case: primary -> idle -> primary; empty the sockets and failed
    neighbor_sockets.clear();

	std::string ip;
	int port;

	for (const auto& node : GetNeighbors()) {
		port = node->port;
		ip = node->ip;
		std::shared_ptr<ClientSocket> socket = std::make_shared<ClientSocket>();
		if (socket->Init(ip, port)) { // if connection is successful
            socket_node[socket] = node;
            neighbor_sockets.push_back(std::move(socket));
        }
	}
}

int ServerMetadata::SendReplicationRequest(MapOp op) {

	char buffer[32];
    int size;

    // get replication request object
	ReplicationRequest request = GetReplicationRequest(op);
	request.Marshal(buffer);
	size = request.Size();

	int total_response = 0;
	char response_buffer[4];
	Identifier identifier;
    std::deque<std::shared_ptr<ClientSocket>> new_primary_sockets;

    // iterate over all the neighbor nodes, and send the replication request
	for (auto const& socket : neighbor_sockets) {

		if (socket->Recv(response_buffer, sizeof(identifier), 0)) {
			identifier.Unmarshal(response_buffer);
			total_response += identifier.GetIdentifier();
		}
        new_primary_sockets.push_back(socket);
	}

    // update with the sockets excluding failed sockets
    neighbor_sockets = new_primary_sockets; 
    if (DEBUG) {
        std::cout << request << std::endl;
    }
    
    // check if the message received matches the size of the neighbors
	if (total_response != GetPeerSize()) {
        if (DEBUG) {
            // std::cout << GetPeerSize() << " " << total_response << std::endl;
            std::cout << "Some neighbor has not updated the log, so I am not executing the log!" << std::endl;
        }
		return 0;
	}

    return 1;
}

std::shared_ptr<ServerNode> ServerMetadata::GetLeader() {
    for (std::shared_ptr<ServerNode> nei: GetNeighbors()) {
        if (nei->id == GetLeaderId()) {
            return nei;
        }
    }
}

std::string ServerMetadata::GetLeaderIp() {
    std::shared_ptr<ServerNode> leader = GetLeader();
    return leader->ip;
}

int ServerMetadata::GetLeaderPort() {
    std::shared_ptr<ServerNode> leader = GetLeader();
    return leader->port;
}

int ServerMetadata::GetStatus() {
    return status;
}

void ServerMetadata::SetStatus(int status) {
    this->status = status;
}

void ServerMetadata::ReplicateLog() {

}

void ServerMetadata::RequestVote() {
    int current_status, voted, voter_term, voter_id;

    voted_for = factory_id;
    vote_received.insert(factory_id);
    current_term++;

    int log_size = GetLogSize();
    int last_term = GetLastTerm();

    // create the RequestVoteMessage
    RequestVoteMessage msg;
    RequestVoteResponse res;

    char buffer[32];
    int size = msg.Size();
    msg.SetRequestVoteMessage(factory_id, current_term, log_size, last_term);
    msg.Marshal(buffer);

    // request vote to all the neighbors
    for (std::shared_ptr<ClientSocket> nei : GetNeighborSockets()) {

        // send the identifier that it is the request vote
        SendIdentifier(CANDIDATE_IDENTIFIER, nei);

        // send the request vote message
        nei->Send(buffer, size, 0);

        // receive the vote
        res = RecvVoteResponse(nei);

        // check if the vote is valid, and update the voted
        voted = res.GetVoted();
        voter_term = res.GetCurrentTerm();
        voter_id = res.GetId();

        if (current_term == voter_term && voted) {
            vote_received.insert(voter_id);


        }
        


    }
}

void ServerMetadata::InitLeader() {
    size = GetPeerSize();
    this->sent_length[]
    for ()
}

int ServerMetadata::SendIdentifier(int identifier, std::shared_ptr<ClientSocket> nei) {
	Identifier iden;
    iden.SetIdentifier(identifier);
    char buffer[4];
    int size = iden.Size();
    iden.Marshal(buffer);
    return nei->Send(buffer, size, 0);
}

int ServerMetadata::GetVoteReceivedSize() {
    return vote_received.size();
}

bool ServerMetadata::WonElection() {
    return GetVoteReceivedSize() > GetPeerSize() * 2;
}

int ServerMetadata::GetCurrentTerm() {
    return current_term;
}

void ServerMetadata::SetCurrentTerm(int term) {
    current_term = term;
}

void ServerMetadata::SetVotedFor(int id) {
    voted_for = id;
}

int ServerMetadata::GetLogSize() {
    return smr_log.size();
}

int ServerMetadata::GetLastTerm() {
    int last_term = 0;
    int log_size = GetLogSize();
    if (log_size) {
        last_term = smr_log[log_size - 1].term;
    }
    return last_term;
}

bool ServerMetadata::GetVotedFor() {
    return voted_for;
}

RequestVoteResponse RecvVoteResponse(std::shared_ptr<ClientSocket> nei) {
    RequestVoteResponse res;
    char buffer[32];
    int size = res.Size();
    nei->Recv(buffer, size, 0);
    res.Unmarshal(buffer);
    return res;
}


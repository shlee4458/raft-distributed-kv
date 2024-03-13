#include "ServerMetadata.h"
#include "Messages.h"

#include <string.h>
#include <iostream>

#define PFA_IDENTIFIER 1
#define FOLLOWER 1
#define SERVER_IDENTIFIER 3
#define DEBUG 0

#define REQUESTVOTE_RPC 0
#define APPENDLOG_RPC 1

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

int ServerMetadata::GetTermIdx(int idx) {
    return smr_log[idx].term;
}

int ServerMetadata::GetCommitLength() {
    return commit_length;
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
            SendIdentifier(SERVER_IDENTIFIER, socket); // first tell the server that it is server speaking
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

        // send the identifier that it is request vote rpc
        SendIdentifier(REQUESTVOTE_RPC, nei);

        // send the request vote message
        nei->Send(buffer, size, 0);

        // receive the vote
        res = RecvVoteResponse(nei);
        voted = res.GetVoted();
        voter_term = res.GetCurrentTerm();
        voter_id = res.GetId();

        // check if the vote is valid, and update the voted
        if (current_term == voter_term && voted) {
            vote_received.insert(voter_id);

            // if the vote is majority
            if (WonElection()) {
                InitLeader();
                return;
            }
        
        // found another node with higher term
        } else if (voter_term > current_term) {
            current_term = voter_term;
            status = FOLLOWER;
            voted_for = -1;
            return;
        }
    }
    return; // split vote happened
}

void ServerMetadata::InitLeader() {
    int size = GetPeerSize() + 1;
    int log_size = GetLogSize() + 1;
    this->sent_length[size];
    this->ack_length[size];
    for (int i = 0; i < size; i++) {
        sent_length[i] = log_size;
        ack_length[i] = 0;
    }
    return;
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

RequestVoteResponse ServerMetadata::RecvVoteResponse(std::shared_ptr<ClientSocket> nei) {
    RequestVoteResponse res;
    char buffer[32];
    int size = res.Size();
    nei->Recv(buffer, size, 0);
    res.Unmarshal(buffer);
    return res;
}

void ServerMetadata::SetAckLength(int node_idx, int size) {
    // if the node_idx is -1, get the last idx
    if (node_idx == -1) {
        node_idx = GetPeerSize();
    } 
    
    // set the ack_length at the node_idx with size
    ack_length[node_idx] = size;
}

int ServerMetadata::ReplicateLog() {

    // for each of the neighbors
        // get the length of the item sent to the neighbor
    int log_size, prefix_length, prefix_term, op_term, op_arg1, op_arg2;
    std::shared_ptr<ClientSocket> socket;
    LogRequest log_req;
    LogResponse log_res;
    MapOp op;
    int log_size = last_idx;
    int j;

    for (int i = 0; i < GetPeerSize(); i++) {
        socket = neighbor_sockets[i];
        prefix_length = sent_length[i];
        prefix_term = 0;
        if (prefix_length > 0) {
            prefix_term = smr_log[prefix_length - 1].term;
        }

        // for all the unsent op, send to the followers
        while (prefix_length < log_size) {
            op_term = smr_log[j].term;
            op_arg1 = smr_log[j].arg1;
            op_arg2 = smr_log[j].arg2;
            log_req.SetLogRequest(factory_id, current_term, prefix_length, prefix_term,
                             commit_length, op_term, op_arg1, op_arg2);
            
            // send the log to the follower
            SendLog(log_req, socket);
            
            // update the prefix_length logRequest accordingly with the response
            log_res = RecvLogResponse(socket);

            // TODO: ADD LOGIC AT 8
            
        }

    }

}

int ServerMetadata::SendLog(LogRequest log_req, std::shared_ptr<ClientSocket> socket) {
    SendIdentifier(APPENDLOG_RPC, socket);
	char buffer[64];
    int size;
    log_req.Marshal(buffer);
    size = log_req.Size();
    return socket->Send(buffer, size, 0);
}

LogResponse ServerMetadata::RecvLogResponse(std::shared_ptr<ClientSocket> socket) {
    LogResponse log_res;
    char buffer[32];
    int size = log_res.Size();
    socket->Recv(buffer, size, 0);
    log_res.Unmarshal(buffer);
    return log_res;
}

void ServerMetadata::DropUncommittedLog(int log_size, int req_prefix_length) {
    for (int log_size; log_size < req_prefix_length; log_size++) {
        smr_log.pop_back();
    }
}
#include "ServerMetadata.h"
#include "Messages.h"

#include <string.h>
#include <iostream>
#include <random>

#define FOLLOWER 1
#define LEADER 2
#define CANDIDATE 3

#define SERVER_IDENTIFIER 1

#define REQUESTVOTE_RPC 0
#define APPENDLOG_RPC 1

#define DEBUG 0

ServerMetadata::ServerMetadata() 
: leader_id(-1), 
  factory_id(-1), 
  current_term(0), 
  status(1), 
  commit_length(0), 
  log_size(0), 
  voted_for(-1), 
  heartbeat(false),
  server_index_map(),
  neighbors() { }

int ServerMetadata::GetLeaderId() {
    return leader_id;
}

int ServerMetadata::GetFactoryId() {
    return factory_id;
}

std::vector<std::shared_ptr<ServerNode>> ServerMetadata::GetNeighbors() {
    return neighbors;
}

int ServerMetadata::GetPeerSize() {
    return neighbors.size();
}

std::map<std::shared_ptr<ServerNode>, std::shared_ptr<ClientSocket>> ServerMetadata::GetNodeSocket() {
    return node_socket;
}

std::shared_ptr<ServerNode> ServerMetadata::GetLeader() {
    for (std::shared_ptr<ServerNode> nei: GetNeighbors()) {
        if (nei->id == GetLeaderId()) {
            return nei;
        }
    }
    return nullptr;
}

std::string ServerMetadata::GetLeaderIp() {
    std::shared_ptr<ServerNode> leader = GetLeader();
    return leader->ip;
}

int ServerMetadata::GetLeaderPort() {
    std::shared_ptr<ServerNode> leader = GetLeader();
    return leader->port;
}

int ServerMetadata::GetServerIndex(int id) {
    return server_index_map[id];
}

std::vector<MapOp> ServerMetadata::GetLog() {
    return smr_log;
}

MapOp ServerMetadata::GetOp(int idx) {
    std::cout << "Op index was: " << idx << std::endl;
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

int ServerMetadata::GetTermAtIdx(int idx) {
    if (idx < 0) {
        return -1;
    }
    return smr_log[idx].term;
}

int ServerMetadata::GetCommitLength() {
    return commit_length;
}

int ServerMetadata::GetStatus() {
    return status;
}

int ServerMetadata::GetVoteReceivedSize() {
    return vote_received.size();
}

int ServerMetadata::GetCurrentTerm() {
    return current_term;
}

int ServerMetadata::GetLogSize() {
    return log_size;
}

int ServerMetadata::GetLastTerm() {
    int last_term = 0;
    if (log_size != 0) {
        last_term = smr_log[log_size - 1].term;
    }
    return last_term;
}

bool ServerMetadata::GetVotedFor() {
    return voted_for;
}

bool ServerMetadata::GetHeartbeat() {
    return heartbeat;
}

int ServerMetadata::GetPrefixTerm(int prefix_length) {
    int prefix_term = 0;
    if (prefix_length > 0) {
        prefix_term = GetTermAtIdx(prefix_length - 1);
    }
    return prefix_term;
}

void ServerMetadata::SetFactoryId(int id) {
    factory_id = id;
    return;
}

void ServerMetadata::SetLeaderId(int id) {
	leader_id = id;
    return;
}

void ServerMetadata::SetCurrentTerm(int term) {
    current_term = term;
}

void ServerMetadata::SetVotedFor(int id) {
    voted_for = id;
}

void ServerMetadata::SetStatus(int status) {
    this->status = status;
}

void ServerMetadata::EmptyVoteReceived() {
    vote_received.clear();
}

void ServerMetadata::SetHeartbeat(bool heartbeat) {
    this->heartbeat = heartbeat;
}

int ServerMetadata::IsLeader() {
    if (leader_id != -1) { // there is a leader
        return leader_id == factory_id; // return value: 0 or 1

    } else { // there is no leader
        return -1;
    }
    return leader_id == factory_id;
}

void ServerMetadata::AddNeighbors(std::shared_ptr<ServerNode> node, int idx) {
    server_index_map[node->id] = idx;
    neighbors.push_back(std::move(node));
}

void ServerMetadata::InitNeighbors() {

    node_socket.clear();
    failed_neighbors.clear();

	std::string ip;
	int port;

	for (const auto& node : GetNeighbors()) {
		port = node->port;
		ip = node->ip;
		std::shared_ptr<ClientSocket> socket = std::make_shared<ClientSocket>();
		if (socket->Init(ip, port)) { // if connection is successful
            SendIdentifier(SERVER_IDENTIFIER, socket); // first tell the server that it is server speaking
            node_socket[node] = socket;
        }
	}
}

int ServerMetadata::SendIdentifier(int identifier, std::shared_ptr<ClientSocket> nei) {
	Identifier iden;
    iden.SetIdentifier(identifier);
    char buffer[4];
    int size = iden.Size();
    iden.Marshal(buffer);
    return nei->Send(buffer, size, 0);
}

bool ServerMetadata::WonElection() {
    return GetVoteReceivedSize() * 2 >= GetPeerSize();
}

void ServerMetadata::InitLeader() {
    int size = GetPeerSize() + 1;
    this->sent_length = new int[size];
    this->ack_length = new int[size];
    for (int i = 0; i < size; i++) {
        sent_length[i] = log_size;
        ack_length[i] = 0;
    }
    leader_id = factory_id;
    status = LEADER;
    std::cout << "Set itself as the leader!" << std::endl;
    return;
}

void ServerMetadata::SetAckLength(int node_idx, int size) {
    // if the node_idx is -1, get the last idx
    if (node_idx == -1) {
        node_idx = GetPeerSize();
    }
    
    // set the ack_length at the node_idx with size
    ack_length[node_idx] = size;
}

/**
 * Request Vote RPC 
*/

void ServerMetadata::RequestVote() {

    int voted, voter_term, voter_id;
    std::shared_ptr<ServerNode> server_node;
    std::shared_ptr<ClientSocket> socket;
     
    // vote for itself, increase the current term
    voted_for = factory_id;
    vote_received.insert(factory_id);
    current_term++;

    // create the RequestVoteMessage
    RequestVoteResponse res;

    // request vote to all the neighbors
    for (auto it = node_socket.begin(); it != node_socket.end(); ++it) {
        server_node = it->first;
        socket = it->second;

        SendIdentifier(REQUESTVOTE_RPC, socket);

        // send the request vote message
        SendRequestVote(socket);

        // receive the vote response
        res = RecvVoteResponse(socket);
        voted = res.GetVoted();
        voter_term = res.GetCurrentTerm();
        voter_id = res.GetId();

        // res.Print();

        // check if the vote is valid, and update the voted
        if ((current_term == voter_term) && voted) {
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
            EmptyVoteReceived();
            return;
        }
    }

    // if there are servers, and didn't receive any vote other than voting itself,
        // set the current server to follower, and reset the current term
    // if (node_socket.size() > 0 && vote_received.size() == 1) {
    //     status = FOLLOWER;
    //     voted_for = -1;
    //     EmptyVoteReceived();
    //     std::cout << "I have not received any vote as the candidate: demoting to follower." << std::endl;
    // }

    return; // split vote happened
}

RequestVoteResponse ServerMetadata::GetVoteResponse(RequestVoteMessage msg) {
    
    int cand_id = msg.GetId();
    int cand_current_term = msg.GetCurrentTerm();
    int cand_log_size = msg.GetLogSize();
    int cand_last_term = msg.GetLastTerm();
    int last_term = GetLastTerm();

    // if more higher term candidate vote is received
    if (cand_current_term > current_term) {
        SetCurrentTerm(cand_current_term);
        SetStatus(FOLLOWER);
        EmptyVoteReceived();
        SetVotedFor(cand_id);
    }
    
    bool valid = (cand_last_term > last_term) || 
                ((cand_last_term == last_term) && cand_log_size >= log_size);

    RequestVoteResponse res;
    if (valid && cand_current_term == current_term && voted_for == cand_id) {
        res.SetRequestVoteResponse(factory_id, current_term, true);
    } else {
        res.SetRequestVoteResponse(factory_id, current_term, false);
    }

    SetHeartbeat(true);
    return res;
}

int ServerMetadata::SendRequestVote(std::shared_ptr<ClientSocket> socket) {
    RequestVoteMessage msg;
    char buffer[32];
    int size = msg.Size();
    int last_term = GetLastTerm();
    msg.SetRequestVoteMessage(factory_id, current_term, log_size, last_term);
    msg.Marshal(buffer);
    // msg.Print();
    return socket->Send(buffer, size, 0);
}

RequestVoteResponse ServerMetadata::RecvVoteResponse(std::shared_ptr<ClientSocket> nei) {
    RequestVoteResponse res;
    char buffer[32];
    int size = res.Size();
    nei->Recv(buffer, size, 0);
    res.Unmarshal(buffer);
    return res;
}

/**
 * Replicate Log RPC
*/

int ServerMetadata::ReplicateLog() {

    TryReconnect();

    int prefix_length, prefix_term, op_term, op_arg1, op_arg2, i;
    std::shared_ptr<ClientSocket> socket;
    LogRequest log_req;
    LogResponse log_res;
    std::map<std::shared_ptr<ServerNode>, std::shared_ptr<ClientSocket>> new_node_socket;

    int term, ack, success;
    int updated = true;
    int num_change = 0;
    bool first_round = true;

    int rand_num;
    std::random_device rd;
    std::mt19937 gen(rd());
    // std::uniform_int_distribution<> dis(0, 9);
    std::uniform_int_distribution<> dis(1, 1);

    while (updated) {
        rand_num = dis(gen);        
        for (auto it = node_socket.begin(); it != node_socket.end(); ++it) {
            i = server_index_map[it->first->id];
            socket = it->second;
            prefix_length = sent_length[i];
            prefix_term = GetPrefixTerm(prefix_length);

            if (log_size == prefix_length) {
                if (first_round || rand_num == 1) { // experimenting with the randomization of sending heartbeat
                    log_req.SetLogRequest(factory_id, current_term, prefix_length, prefix_term,
                                    commit_length, -1, -1, -1);
                    
                    // send the log to the follower
                    if (!SendIdentifier(APPENDLOG_RPC, socket)) { // follower failure
                        CleanNodeState(i);
                        failed_neighbors.push_back(it->first);
                        continue;
                    }
                    if (!SendLogRequest(log_req, socket)) { // follower failure
                        CleanNodeState(i);
                        failed_neighbors.push_back(it->first);
                        continue;
                    }
                    log_res = RecvLogResponse(socket);

                    if (log_res.GetFollowerId() == -1) { // follower failure
                        CleanNodeState(i);
                        failed_neighbors.push_back(it->first);
                        continue;
                    }
                }

            } else {
                // update the prefix_term
                prefix_term = GetPrefixTerm(prefix_length);

                op_term = smr_log[prefix_length].term;
                op_arg1 = smr_log[prefix_length].arg1;
                op_arg2 = smr_log[prefix_length].arg2;
                log_req.SetLogRequest(factory_id, current_term, prefix_length, prefix_term,
                                commit_length, op_term, op_arg1, op_arg2);

                // std::cout << log_req << std::endl;
                
                // send the log to the follower
                if (!SendIdentifier(APPENDLOG_RPC, socket)) {
                    CleanNodeState(i);
                    failed_neighbors.push_back(it->first);
                    continue;
                }
                if (!SendLogRequest(log_req, socket)) {
                    CleanNodeState(i);
                    failed_neighbors.push_back(it->first);
                    continue;
                }

                // update the prefix_length logRequest accordingly with the response
                log_res = RecvLogResponse(socket);
                if (log_res.GetFollowerId() == -1) {
                    CleanNodeState(i);
                    std::cout << "Log Response was not valid!!" << std::endl;
                    failed_neighbors.push_back(it->first);
                    continue;
                }

                term = log_res.GetCurrentTerm();
                ack = log_res.GetAck();
                success = log_res.GetSuccess();
                // std::cout << "Term: " << term << std::endl;
                // std::cout << "Ack: " << ack << std::endl;
                // std::cout << "Success: " << success << std::endl;

                if (term == current_term && status == LEADER) {
                    if (success) {
                        sent_length[i] = ack;
                        ack_length[i] = ack;
                        prefix_length++;
                        if (ack >= ack_length[i]) { // in case leader can commit
                            CommitLog();
                        }

                    } else if (sent_length[i] > 0) { // send the previous log
                        sent_length[i]--;
                        prefix_length--;
                    }

                } else if (term > current_term) { // demote to the follower
                    current_term = term;
                    status = FOLLOWER;
                    voted_for = -1;
                    EmptyVoteReceived();
                    return 0;
                }
                num_change++;
            }
            new_node_socket[it->first] = it->second;
        }

        if (!num_change) {
            updated = false;
        }
        first_round = false;
        num_change = 0;
        // std::cout << "Num of socket in the new node_socket: " << node_socket.size() << std::endl;
        node_socket = new_node_socket;
    }
    return 1;
}

void ServerMetadata::CleanNodeState(int idx) {
    ack_length[idx] = 0;
    sent_length[idx] = 0;
    std::cout << "follower at idx: " << idx << " has failed" << std::endl;
}

void ServerMetadata::TryReconnect() {

    // if there is no failed servers, return
    if (failed_neighbors.empty()) {
        return;
    }

	std::string ip;
	int port;

    // update the failed server queue
    std::deque<std::shared_ptr<ServerNode>> new_failed_neighbors;

    // iterate over all the failed servers, and try reconnecting 
	for (const auto& node : failed_neighbors) {
		port = node->port;
		ip = node->ip;
		std::shared_ptr<ClientSocket> socket = std::make_shared<ClientSocket>();
		if (socket->Init(ip, port)) { // if connection is successful
            SendIdentifier(SERVER_IDENTIFIER, socket); // first tell the server that it is server speaking
            node_socket[node] = socket;
            continue;
        }
        socket->Close();
        new_failed_neighbors.push_back(node); // if unsucessful, keep the failed neighbors
	}
    failed_neighbors = new_failed_neighbors;
}

LogResponse ServerMetadata::GetLogResponse(LogRequest log_req) {
    
    int req_leader_id, req_current_term, req_prefix_length, req_prefix_term;
    int req_commit_length, req_op_term, req_op_arg1, req_op_arg2;
    bool included, can_log;
    LogResponse log_res;

    // get the information
    req_leader_id = log_req.GetLeaderId();
    req_current_term = log_req.GetCurrentTerm();
    req_prefix_length = log_req.GetPrefixLength();
    req_prefix_term = log_req.GetPrefixTerm();
    req_commit_length = log_req.GetCommitLength();
    req_op_term = log_req.GetOpTerm();
    req_op_arg1 = log_req.GetOpArg1();
    req_op_arg2 = log_req.GetOpArg2();

    // compare the req_term with the server term
    if (req_current_term >= current_term || status == FOLLOWER) {
        SetCurrentTerm(req_current_term);
        SetStatus(FOLLOWER);
        SetLeaderId(req_leader_id);
        EmptyVoteReceived();
        SetVotedFor(-1);
        heartbeat = true; // reset the timeout
    } 

    included = (log_size > req_prefix_length);
    can_log = (log_size >= req_prefix_length) && 
                (req_prefix_length == 0 || GetTermAtIdx(req_prefix_length - 1) == req_prefix_term);
    // std::cout << "Log_size: " << log_size << std::endl;
    // std::cout << "req_prefix_length: " << req_prefix_length << std::endl;
    // std::cout << "Term at idx: " << GetTermAtIdx(req_prefix_length - 1) << std::endl;
    // std::cout << "req_prefix_term: " << req_prefix_term << std::endl;

    if ((current_term == req_current_term) && can_log) {
        if (!included) {
            // drop the uncommitted log
            if (log_size > req_prefix_length) {
                DropUncommittedLog(log_size, req_prefix_length);
            }

            // append the log
            if (req_op_term != -1) { // if it is not the heartbeat message
                AppendLog(req_op_term, req_op_arg1, req_op_arg2);
            }

            // commit the appended log
            if (req_commit_length > commit_length) {

                // commit a single log at the current prefix length + 1
                ExecuteLog(commit_length);
            }
        }
        log_res.SetLogResponse(factory_id, current_term, req_prefix_length + 1, 1); // set yes reponse

    } else {
        log_res.SetLogResponse(factory_id, current_term, 0, 0); // set no response
    }

    SetHeartbeat(true);
    return log_res;
}

void ServerMetadata::AppendLog(int op_term, int customer_id, int order_num) {
    MapOp op;
    op.term = op_term;
    op.arg1 = customer_id;
    op.arg2 = order_num;
    std::cout << "Appended Log: " << op.term << ", " << op.arg1 << ", " << op.arg2 << std::endl;

    smr_log.push_back(op);
    log_size++;
    return;
}

void ServerMetadata::ExecuteLog(int idx) {
    int customer_id, order_num;
    
    MapOp op = GetOp(idx); // get op at idx
    std::cout << "found the op log!" << std::endl;
    customer_id = op.arg1;
    order_num = op.arg2;

    customer_record[customer_id] = order_num;
    std::cout << "Record Updated for client: " << customer_id 
        << " Order Num: " << order_num << std::endl;
    commit_length++;
    return;
}

void ServerMetadata::CommitLog() {
    // from the commit_length to log_size, find the maximum index that has majority of the vote received
    int commit_until = 0;
    int count = 0;

    for (int i = commit_length; i < log_size + 1; i++) {
        count = 0;
        for (int j = 0; j < GetPeerSize() + 1; j++) {
            if (ack_length[j] >= i) {
                count++;
            }
        }
        if (count * 2 > GetPeerSize() + 1) { // ack by the majority, commit
            commit_until = i;
        }
    }

    // no more item to commit
    if (!commit_until) {
        return;
    }

    // if there exists log to commit
    for (int i = commit_length; i < commit_until; i++) {
        ExecuteLog(i);
    }

    commit_length = commit_until;
    return;
}

int ServerMetadata::SendLogRequest(LogRequest log_req, std::shared_ptr<ClientSocket> socket) {
	char buffer[64];
    int size = log_req.Size();
    log_req.Marshal(buffer);
    return socket->Send(buffer, size, 0);
}

LogResponse ServerMetadata::RecvLogResponse(std::shared_ptr<ClientSocket> socket) {
    LogResponse log_res;
    char buffer[32];
    int size = log_res.Size();
    if (!socket->Recv(buffer, size, 0)) {
        std::cout << "Follower has failed before sending the log response" << std::endl;
        return log_res;
    }
    log_res.Unmarshal(buffer);
    return log_res;
}

void ServerMetadata::DropUncommittedLog(int size, int req_prefix_length) {
    for (; size < req_prefix_length; size++) {
        smr_log.pop_back();
    }
}

std::deque<std::shared_ptr<ClientSocket>> ServerMetadata::GetNeighborSockets() {
    return neighbor_sockets;
}

int ServerMetadata::GetNeighborSocketSize() {
    return neighbor_sockets.size();
}
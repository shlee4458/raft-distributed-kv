#include "ServerMetadata.h"
#include "Messages.h"

#include <string.h>
#include <iostream>

#define FOLLOWER 1
#define LEADER 2
#define CANDIDATE 3

#define SERVER_IDENTIFIER 1

#define REQUESTVOTE_RPC 0
#define APPENDLOG_RPC 1

#define DEBUG 0

ServerMetadata::ServerMetadata() 
: leader_id(-1), factory_id(-1), neighbors(), voted_for(-1), current_term(0), log_size(0), status(1) { }

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

int ServerMetadata::GetTermIdx(int idx) {
    return smr_log[idx].term;
}

int ServerMetadata::GetCommitLength() {
    return commit_length;
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
    if (log_size) {
        last_term = smr_log[log_size - 1].term;
    }
    return last_term;
}

bool ServerMetadata::GetVotedFor() {
    return voted_for;
}

void ServerMetadata::SetLeaderId(int id) {
	leader_id = id;
    return;
}

void ServerMetadata::SetFactoryId(int id) {
    factory_id = id;
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


void ServerMetadata::AppendLog(MapOp op) {
    smr_log.push_back(op);
    log_size++;
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
    commit_length++;
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

int ServerMetadata::SendIdentifier(int identifier, std::shared_ptr<ClientSocket> nei) {
	Identifier iden;
    iden.SetIdentifier(identifier);
    char buffer[4];
    int size = iden.Size();
    iden.Marshal(buffer);
    return nei->Send(buffer, size, 0);
}

bool ServerMetadata::WonElection() {
    return GetVoteReceivedSize() > GetPeerSize() * 2;
}

void ServerMetadata::InitLeader() {
    int size = GetPeerSize() + 1;
    this->sent_length[size];
    this->ack_length[size];
    for (int i = 0; i < size; i++) {
        sent_length[i] = log_size;
        ack_length[i] = 0;
    }
    status = FOLLOWER;
    return;
}

/**
 * Request Vote RPC 
*/

void ServerMetadata::RequestVote() {
    int current_status, voted, voter_term, voter_id, last_term;

    voted_for = factory_id;
    last_term = GetLastTerm();
    vote_received.insert(factory_id);
    current_term++;

    // create the RequestVoteMessage
    RequestVoteMessage msg;
    RequestVoteResponse res;

    char buffer[32];
    int size = msg.Size();
    msg.SetRequestVoteMessage(factory_id, current_term, log_size, last_term);
    msg.Marshal(buffer);

    // request vote to all the neighbors
    for (std::shared_ptr<ClientSocket> nei : GetNeighborSockets()) {

        // TODO: consider having thread pool of size peersize - 1 collect votes
        // send the identifier that it is request vote rpc
        SendIdentifier(REQUESTVOTE_RPC, nei);

        // send the request vote message
        nei->Send(buffer, size, 0);

        // receive the vote response
        res = RecvVoteResponse(nei);
        voted = res.GetVoted();
        voter_term = res.GetCurrentTerm();
        voter_id = res.GetId();

        // check if the vote is valid, and update the voted
        if (current_term == voter_term && voted) {
            vote_received.insert(voter_id);

            // if the vote is majority
            if (WonElection()) {
                std::cout << "Won the election!" << std::endl;
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
    return res;
}

RequestVoteResponse ServerMetadata::RecvVoteResponse(std::shared_ptr<ClientSocket> nei) {
    RequestVoteResponse res;
    char buffer[32];
    int size = res.Size();
    nei->Recv(buffer, size, 0);
    res.Unmarshal(buffer);
    return res;
}

int ServerMetadata::ReplicateLog() {

    // for each of the neighbors
        // get the length of the item sent to the neighbor
    int prefix_length, prefix_term, op_term, op_arg1, op_arg2;
    std::shared_ptr<ClientSocket> socket;
    LogRequest log_req;
    LogResponse log_res;
    MapOp op;
    int j;

    int follower_id, term, ack, success;
    int cur_prefix_length;

    for (int i = 0; i < GetPeerSize(); i++) {
        socket = neighbor_sockets[i];
        prefix_length = sent_length[i];
        prefix_term = 0;
        if (prefix_length > 0) {
            prefix_term = smr_log[prefix_length - 1].term;
        }

        // for all the unsent op, send to the followers
        cur_prefix_length = prefix_length;
        while (cur_prefix_length < log_size) {
            op_term = smr_log[j].term;
            op_arg1 = smr_log[j].arg1;
            op_arg2 = smr_log[j].arg2;
            log_req.SetLogRequest(factory_id, current_term, cur_prefix_length, prefix_term,
                             commit_length, op_term, op_arg1, op_arg2);
            
            // send the log to the follower
            SendLog(log_req, socket);
            
            // update the prefix_length logRequest accordingly with the response
            log_res = RecvLogResponse(socket);

            // TODO: ADD LOGIC AT 8
            follower_id = log_res.GetFollowerId();
            term = log_res.GetCurrentTerm();
            ack = log_res.GetAck();
            success = log_res.GetSuccess();

            if (term == current_term && status == LEADER) {
                if (success && ack > ack_length[i]) {
                    sent_length[i] = ack;
                    ack_length[i] = ack;
                    CommitLog();
                } else if (sent_length[i] > 0) { // send the previous log
                    sent_length[i]--;
                    cur_prefix_length--;
                    continue;
                }
            } else if (term > current_term) { // demote to the follower
                current_term = term;
                status = FOLLOWER;
                voted_for = -1;
                return 0;
            }

            j++;
        }
    }
    return 1;
}



void ServerMetadata::SetAckLength(int node_idx, int size) {
    // if the node_idx is -1, get the last idx
    if (node_idx == -1) {
        node_idx = GetPeerSize();
    } 
    
    // set the ack_length at the node_idx with size
    ack_length[node_idx] = size;
}

void ServerMetadata::CommitLog() {
    // from the commit_length to log_size, find the maximum index that has majority of the vote received
    int commit_until, count;
    for (int i = commit_length; i < GetLogSize(); i++) {
        count = 0;
        for (int j = 0; j < GetPeerSize(); j++) {
            if (ack_length[j] >= i) {
                count++;
            }
        }
        if (count * 2 > GetPeerSize()) { // ack by the majority, commit
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
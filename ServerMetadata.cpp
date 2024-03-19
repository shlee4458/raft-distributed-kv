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

void ServerMetadata::SetHeartbeat(bool heartbeat) {
    this->heartbeat = heartbeat;
}

int ServerMetadata::IsLeader() {
    return leader_id == factory_id;
}

void ServerMetadata::AddNeighbors(std::shared_ptr<ServerNode> node, int idx) {
    server_index_map[node->id] = idx;
    neighbors.push_back(std::move(node));
}

void ServerMetadata::InitNeighbors() {

    // corner case: primary -> idle -> primary; empty the sockets and failed
    node_socket.clear();

	std::string ip;
	int port;

	for (const auto& node : GetNeighbors()) {
		port = node->port;
		ip = node->ip;
		std::shared_ptr<ClientSocket> socket = std::make_shared<ClientSocket>();
		if (socket->Init(ip, port)) { // if connection is successful
            SendIdentifier(SERVER_IDENTIFIER, socket); // first tell the server that it is server speaking
            node_socket[node] = socket; // CLOSE THE SOCKET WHEN THE SERVER LEAVES
            // neighbor_sockets.push_back(std::move(socket));
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

    // if it does not have neighbor, set it as the leader.
    if (neighbors.size() < 2) {
        InitLeader();
        return;
    }

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
        // std::cout << "Sending vote request to servers" << std::endl;

        // TODO: consider having thread pool of size peersize - 1 collect votes
        // send the identifier that it is request vote rpc
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
        // std::cout << "Collecting vote from: " << server_node->id << std::endl;
        if ((current_term == voter_term) && voted) {
            // std::cout << "I have received the vote!" << std::endl;
            vote_received.insert(voter_id);

            // if the vote is majority
            if (WonElection()) {
                // std::cout << "Won the election!" << std::endl;
                InitLeader();
                return;
            }
        
        // found another node with higher term
        } else if (voter_term > current_term) {
            current_term = voter_term;
            status = FOLLOWER;
            voted_for = -1;
            vote_received.clear();
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

    // std::cout << "This is the vote request that I have received!" << std::endl;
    // msg.Print();

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

    SetHeartbeat(true);
    // std::cout << "#### Heartbeat was set to true!! ####" << std::endl;
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

int ServerMetadata::ReplicateLog(bool is_heartbeat) {

    // for each of the neighbors
        // get the length of the item sent to the neighbor
    int prefix_length, prefix_term, op_term, op_arg1, op_arg2, i;
    std::shared_ptr<ClientSocket> socket;
    LogRequest log_req;
    LogResponse log_res;

    int term, ack, success;
    int iteration = 0;

    for (auto it = node_socket.begin(); it != node_socket.end(); ++it) {
        i = server_index_map[it->first->id];
        socket = it->second;

        prefix_length = sent_length[i];
        prefix_term = 0;
        if (prefix_length > 0) {
            prefix_term = smr_log[prefix_length - 1].term;
        }

        if (log_size == prefix_length) { // send emtpy heartbeat
            log_req.SetLogRequest(factory_id, current_term, prefix_length, prefix_term,
                            commit_length, -1, -1, -1);
            
            // send the log to the follower
            // std::cout << "Sending a simple heartbeat to: " << it->first->id << std::endl;
            SendIdentifier(APPENDLOG_RPC, socket);
            SendLogRequest(log_req, socket);
            log_res = RecvLogResponse(socket);
        }

        else { // for all the unsent op, send to the followers
            while (prefix_length < log_size) {
                op_term = smr_log[prefix_length].term;
                op_arg1 = smr_log[prefix_length].arg1;
                op_arg2 = smr_log[prefix_length].arg2;
                log_req.SetLogRequest(factory_id, current_term, prefix_length, prefix_term,
                                commit_length, op_term, op_arg1, op_arg2);

                std::cout << log_req << std::endl;
                
                // send the log to the follower
                SendIdentifier(APPENDLOG_RPC, socket);
                SendLogRequest(log_req, socket);

                // update the prefix_length logRequest accordingly with the response
                log_res = RecvLogResponse(socket);
                // std::cout << "(" << iteration++ << " iteration) log request sent to: " << it->first->id 
                //           << ", prefix_length: " << prefix_length << std::endl;
                // std::cout << "ACK: " << log_res.GetAck() << std::endl;
                // std::cout << "Sucess: " << log_res.GetSuccess() << std::endl;

                term = log_res.GetCurrentTerm();
                ack = log_res.GetAck();
                success = log_res.GetSuccess();

                if (term == current_term && status == LEADER) {
                    // std::cout << "1st condition called!" << std::endl;
                    if (success && ack >= ack_length[i]) {
                        // std::cout << "2nd condition called!" << std::endl;
                        sent_length[i] = ack;
                        ack_length[i] = ack;
                        prefix_length++;
                        CommitLog();
                    } else if (sent_length[i] > 0) { // send the previous log
                        sent_length[i]--;
                        prefix_length--;
                    }
                } else if (term > current_term) { // demote to the follower
                    current_term = term;
                    status = FOLLOWER;
                    voted_for = -1;
                    vote_received.clear();
                    return 0;
                }
            }
            iteration = 0;
        }

    }
    return 1;
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

    // std::cout << log_req << std::endl;

    // compare the req_term with the server term
    if (req_current_term > current_term) {
        SetCurrentTerm(req_current_term);
        SetVotedFor(-1);
        heartbeat = true; // reset the timeout
    } 

    // if the current was candidate, update the status
    if (current_term == req_current_term) { // found the leader
        SetStatus(FOLLOWER);
        SetLeaderId(req_leader_id);
    }

    included = (log_size > req_prefix_length);
    can_log = (log_size >= req_prefix_length) && 
                (req_prefix_length == 0 || GetTermAtIdx(req_prefix_length - 1) == req_prefix_term);

    if ((current_term == req_current_term) && can_log && req_op_term != -1) {
        if (!included && req_op_term != -1) {
            // drop the uncommitted log
            if (log_size > req_prefix_length) {
                DropUncommittedLog(log_size, req_prefix_length);
            }

            // append the log
            AppendLog(req_op_term, req_op_arg1, req_op_arg2);
            // std::cout << "Appended Log!" << std::endl;

            // commit the appended log
            if (req_commit_length > commit_length) {
                ExecuteLog(req_prefix_length);
            }
        }
        log_res.SetLogResponse(factory_id, current_term, req_prefix_length + 1, 1); // set yes reponse

    } else {
        log_res.SetLogResponse(factory_id, current_term, 0, 0); // set no response
    }

    // log_res.Print();
    SetHeartbeat(true);
    return log_res;
}

void ServerMetadata::AppendLog(int op_term, int customer_id, int order_num) {
    MapOp op;
    op.term = op_term;
    op.arg1 = customer_id;
    op.arg2 = order_num;

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
    std::cout << "Record Updated for client: " << customer_id 
        << " Order Num: " << order_num << std::endl;
    commit_length++;
    return;
}

void ServerMetadata::CommitLog() {
    // from the commit_length to log_size, find the maximum index that has majority of the vote received
    int commit_until, count;
    for (int i = commit_length; i < log_size; i++) {
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
    for (int i = commit_length; i < commit_until + 1; i++) {
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
    socket->Recv(buffer, size, 0);
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
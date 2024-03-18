#ifndef __SERVERMETADATA_H__
#define __SERVERMETADATA_H__

#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include <string.h>
#include <deque>
#include <set>

#include "ClientSocket.h"
#include "Messages.h"

struct ServerNode {
    int id;
    std::string ip;
    int port;
};

struct MapOp {
	int term; // term of the op
	int arg1; // customer_id to apply the operation
	int arg2; // parameter for the operation
};

class ServerMetadata {
private:
    int leader_id; 
    int factory_id;
    int current_term;
    int status;
    int commit_length;
    int log_size;
    int voted_for;
    std::set<int> vote_received;
    int* sent_length; // reserve sent_length[size] for itself
    int* ack_length; // reserve ack_length[size] for itself
    bool heartbeat;

    std::map<int, int> server_index_map;
    std::vector<std::shared_ptr<ServerNode>> neighbors;
    std::deque<std::shared_ptr<ClientSocket>> neighbor_sockets; // socket to the backup nodes as a primary
    std::map<int, int> customer_record;
    std::vector<MapOp> smr_log;
    std::map<std::shared_ptr<ServerNode>, std::shared_ptr<ClientSocket>> node_socket;
    

public:
    ServerMetadata();

    int GetLeaderId();
    int GetFactoryId();
    int GetPeerSize();
    int GetStatus();
    int GetVoteReceivedSize();
    int GetCurrentTerm();
    int GetTermAtIdx(int idx);
    int GetCommitLength();
    int GetLogSize();
    int GetLastTerm();
    bool GetVotedFor();
    bool GetHeartbeat();
    int GetServerIndex(int id);
    std::shared_ptr<ServerNode> GetLeader();
    std::string GetLeaderIp();
    int GetLeaderPort();
    std::vector<MapOp> GetLog();
    MapOp GetOp(int idx);
    std::vector<std::shared_ptr<ServerNode>> GetNeighbors();
    std::deque<std::shared_ptr<ClientSocket>> GetNeighborSockets();
    int GetValue(int customer_id);
    ReplicationRequest GetReplicationRequest(MapOp op);
    int GetNeighborSocketSize();
    std::map<std::shared_ptr<ServerNode>, std::shared_ptr<ClientSocket>> GetNodeSocket();

    void SetFactoryId(int id);
    void SetLeaderId(int id);
    void SetStatus(int status);
    void SetCurrentTerm(int term);
    void SetVotedFor(int id);
    int SetCommitLength();
    void SetAckLength(int node_idx, int size);
    void SetHeartbeat(bool heartbeat);
    int SendRequestVote(std::shared_ptr<ClientSocket> socket);

    void AppendLog(int op_term, int customer_id, int order_num);
    void ExecuteLog(int idx);

    int IsLeader();
    bool WonElection();

    void AddNeighbors(std::shared_ptr<ServerNode> node, int idx);
    void InitNeighbors();
    void InitLeader();

    int SendIdentifier(int identifier, std::shared_ptr<ClientSocket> nei);

    // Request Vote RPC
    void RequestVote();
    RequestVoteResponse GetVoteResponse(RequestVoteMessage msg);
    RequestVoteResponse RecvVoteResponse(std::shared_ptr<ClientSocket> nei);

    // Replicate Log RPC
    int ReplicateLog();
    LogResponse GetLogResponse(LogRequest log_req);
    int SendLogRequest(LogRequest lr, std::shared_ptr<ClientSocket> socket);
    LogResponse RecvLogResponse(std::shared_ptr<ClientSocket> socket);
    void CommitLog();
    void DropUncommittedLog(int size, int req_prefix_length);
};

#endif
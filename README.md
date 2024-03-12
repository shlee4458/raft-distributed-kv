<h1 align="center"> Replicated Distributed Systems using Primary-Backup protocol </h1>

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
# Table of Contents

- [About](#About)
- [Libraries](#Libraries)
- [Getting Started](#getting-started)
- [Class Structure](#class-structure)
- [Implementation](#implementation)
- [Evaluation](#evaluation)
- [Further Works](#further-works)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->
<br>

# About

<i> Note: Distributed Systems - Multithreaded Factory is a console-based Server and Client Interface that was developed as part of the CS 7610 Foundation of Distributed Systems in Northeastern University.</i> 

This is a replicated distributed system that allows record update operation through the primary server, replicated to back up server nodes. Read operation is supported both by primary server and backup server.

<br>

# Libraries
```
No external libraries other than C++11 Standard Library was used for the program.
```
<br>

# Getting Started
<i>Note: when running the program in the local environment set ip address as '127.0.0.1' and use port number between 10000 and 65536.</i>

1. Clone the repository:

    ```bash
    git clone [repository_url]
    ```
2. Build the executable:

    ```bash
    make
    ```
3. Start a server in one terminal:

    ```bash
    ./server [port num] [unique ID] [num peers] (repeat [ID] [IP] [port num]
    ```
4. Start a client in another terminal:

    ```bash
    # request_type 1: write request
    # request_type 2: read request (without debug)
    # request_type 3: read request (with debug)
    ./client [ip addr] [port num] [num customers] [num orders] [request_type]
    ```

5. Sample command line arguments to simulate 3 server nodes and 1 client node
    ```bash
    # from node1 run;
    ./server 12345 0 2 1 10.200.125.72 12345 2 10.200.125.73 12345

    # from node2 run;
    ./server 12345 1 2 0 10.200.125.71 12345 2 10.200.125.73 12345

    # from node3 run;
    ./server 12345 2 2 0 10.200.125.71 12345 1 10.200.125.72 12345

    # from node4 run;
    ./client 10.200.125.71 12345 4 40000 1

    ```
<br>

# Class Structure
```
Server
- ServerMain: main entrance to the server program
- ServerStub: interface that engineer classes use to interact with serversocket to send data
- ServerSocket: communication channel between client
- ServerEngineer: class that interacts with server stub to send messages remotely to clients
- ServerMetadata: stores metadata of the server including;
    - primary_id: primary server's unique id
    - last_index: last index of the log that was executed
    - commited_index: last committed index within the log vector
    - smr_log: vector of server replication logs
    - neighbors: server node instance that encapsulates server id, ip, port
    - primary_sockets: clientsocket instance that is used to communicate with the neighbors
    - customer_record: key-value mapping of the customer number and order number

Client
- ClientMain: main entrance to the client program
- ClientSocket: communication channel between server
- ClientStub: instance included in a client thread that acts as an interface with client socket for communication
- ClientTimer: timer used to measure the latency of the request

Common
- Messages: message that is sent between server and clients including;
    - CustomerInfo: customer_id, order_num, request_type 
    - LaptopInfo: customer_id, order_number, request_type, engineer_id, admin_id
    - CustomerRecord: customer_id, last_order
    - Identifier: identifier - 1; Replication request, and ACK for replication request / 2; Customer Request
    - ReplicationRequest: last_idx, committed_idx, primary_id, op_code, op_arg1, op_arg2
```
<br>

# Implementation

## Primary-Backup Server logic
<i>Note: Primary server - Servers that receive the write request from the customer and handles the request. / Backup server - Servers that receive the replication request. </i>

Step 1: Client have 2 options for the requests; write request(type 1), read request(type 2: without debug message, type 3: with debug message). A server can receive request from both client and the primary server(replication request). Once a server receives a request from the customer, it will identify whether it is replication request or the customer request through unmarshalling one-time identifier sent by either primary server(identifier: 1) or the customer(identifier: 0). Each identifier will route the request to the PfaHandler and CustomerHandler, respectively.

Step 2 - replication request handling: PfaHandler in the backup server will wait to receive a replication request sent from the primary server. The replicatioin request includes last_index where the Map Operation object should be inserted in the log object that stores historic map operations, last committed_index from the primary server, and customer id and order number for the simple key value mapping. Once the information is unmarshalled, the backup server responds to the primary server with the acknowledge message containing 1. Primary server will process all the acknowledge messages that it has received by adding up all the message values, and only when all the backup servers have responded with the acknowledge message, it executed the map operation added at the current loop. 

Step 2 - customer request handling: CustomerHandler in both backup server and the primary server will wait to receive a customer request object. If it is a read request, the server communicates directly to the customer with local customer order information stored in the map object. If the order is a write request and the server was a primary server, it sends replication request to the backup servers. However, when the server was a backup server, it will adjust itself to the primary server by establishing a connection with other servers and updating the primary id stored in the metadata object.

Step 3 - If either primary server or client fails while sending the requests, the backup server gracefully exits out from the handler loops, and wait for the connection request from either client or server.

## Design choices for ownership of objects 
I decided to store the server information like factory id, primary id along with customer record related information in the metadata class. For simplicity, I included communication logic between servers -- replication request -- within the metadata class, instead of including all the communication logic within the ServerStub class. Metadata object is a singleton object that stays within the LaptopFactory object.

LaptopFactory object has an ownership of all ServerSockets that are open in the main thread. using a deque of shared_ptr of the stubs. Storing the stub object that has ServerSocket into a container was necessary to prevent server losing access to the ServerSocket object when the blocking socket recv function is called when the stub object goes out of scope and gets destroyed.

## Reparing server failure
For the simplicity of the problem, the program assumes that repairing of the failed backup server is done by primary server. Despite some or all the backup servers' failure, primary server continue to process the client's requests and send replication message to the backup server that are running. When the next client's request, either read or update request, is sent to the primary server, primary server will try to connect with all the failed server nodes that is stored in the metadata object. For all the backup servers that are recovered, primary server will iteratively send all the Map operation stored in the log vector object, sequentially. 

<br>

# Evaluation
The experiment was conducted to understand performance changes based on the number of servers. Each experiment was conducted 3 times to smooth out the affect of external factors. 4 client threads were used for 40,000 write orders per customer.

## Latency Graph
<i>Unit: latency - microseconds</i>
### Experiment
<img alt="Experiment 1 Table" title="Experiment 1 Table" src="img/latency_graph.png" width="500">

Increase in the number of servers resulted in higher latency. We can visualize the trade off of reliability and latency through replicated servers.

<br>

# Further works
The program assumes that all the messages from primary server is sent to back up nodes sequentially based on the TCP protocol. It would be interesting to modify logic to allow handling message sent through UDP protocol. It would also be interesting if we can also allow primary server to send the repair message to the backup nodes when the server comes back up while the clients are sending the message to the primary node, and when the primary and backup nodes are constantly changing.
//
// Created by 唐仁初 on 2022/9/14.
//

#ifndef GOSSIP_GOSSIPNODE_H
#define GOSSIP_GOSSIPNODE_H

#include "services/GossipService.h"
#include "services/GossipCliService.h"
#include "GossipSlot.h"
#include "GossipPeerNode.h"

#include <nlohmann/json.hpp>
#include <grpcpp/grpcpp.h>
#include <unordered_map>
#include <string>
#include <utility>
#include <random>

namespace gossip::server {

    /// @brief Gossip node server, handles client's and other node's request.
    /// @details Class GossipNode handles gossip client's and gossip peer node's rpc request, and maintains a K-V
    /// map in memory.
    class GossipNode {

    public:

        using RetryCall = GossipPeerNode::AsyncClientCall<google::protobuf::Any, google::protobuf::Any>;

        /// Constructor of GossipNode. Create gossip peer node listen port and gossip client listen port.
        /// \param name Name of gossip node. Used to identify different gossip slot.
        /// \param ex_addr Gossip client listen address. If is set "", means don't open client port.
        /// \param in_addr Gossip peer node listen address. If is set "", means don't open peer node port.
        /// \param token Gossip Client Entry token. If not set, this node will acquire no token.
        explicit GossipNode(std::string name, std::string ex_addr, std::string in_addr,std::string token = "");

        /// Destructor of GossipNode. Recycle the sources claimed.
        ~GossipNode();

        /// Get the gossip node address of this gossip node
        /// \return Gossip node address
        [[nodiscard]] std::string internalAddress() const {
            return internal_address_;
        }

        /// Get the gossip client address of this gossip node.
        /// \return Gossip client address
        [[nodiscard]] std::string externalAddress() const {
            return external_address_;
        }

        /// Run this gossip node.
        void run();

        /// Stop this gossip node.
        /// \return If stop succeed
        bool stop();

        /// Get the copy of local slot.
        /// \return GossipSlot object
        [[nodiscard]] const GossipSlot * localSlot() const {
            return local_slot_;
        }

        /// Get the name of this gossip node.
        /// \return The name of this gossip node
        [[nodiscard]] std::string NodeName() const {
            return name_;   // 这里不涉及写操作，不需要加锁
        }

        /// Get the token of this gossip node.
        /// \return The token of this gossip node
        [[nodiscard]] std::string Token() const {
            return token_;   // 这里不涉及写操作，不需要加锁
        }

        /// Get the status of this gossip node.
        /// \return The status of gossip node
        nlohmann::json getNodeStatus();

        /// Handle gossip client's insert or update request. Using the request handled time as
        /// slot version.
        /// \param key The key of message
        /// \param value The value of message
        /// \return Updated slot version
        SlotVersion insertOrUpdateMessage(const std::string &key, const std::string &value);

        /// Handle gossip client's delete request by key.
        /// \param key The key of message
        /// \return Updated slot version
        SlotVersion deleteMessage(const std::string &key);

        /// Handle gossip client's read request by key. This function only finds value in local slot.
        /// \param key The key of message
        /// \return Latest value
        std::string getMessage(const std::string &key);

        /// Handle gossip client's read request by key. This function will finds version in all slot. If latest is
        /// set true, this node will ask the owner of message to get latest version.
        /// \param key The key of message
        /// \param latest Is latest version needed
        /// \return Find result
        pf_gossip::SearchResult searchMessage(const std::string &key, bool latest);

        /// Handle gossip peer node's pull request. Node will first propagate message to another nodes and
        /// then compare and update this node's slot. If slot is updated, function returns new version; if not,
        /// function returns original version.
        /// \param slot Slot name
        /// \param values Slot values
        /// \param version Slot version
        /// \param origin Peer's request
        /// \return Updated slot version. If not updated, returns original version.
        SlotVersion handlePullRequest(const std::string &slot, const SlotValues &values, SlotVersion version,
                                      const pf_gossip::SlotUpdate *origin);

        /// Handle gossip peer node's heartbeat request. If peer node's version is higher than this or
        /// the same as this, function will do nothing. If peer node's version is less than this, function
        /// will put peer node to lower_slots and delayed pull this node's slot to that peer node.
        /// \param node_name Node name of peer node
        /// \param slot_name Slot name to compare
        /// \param version Version of peer node's slot
        /// \return This node's slot version
        SlotVersion
        handleHeartbeatRequest(const std::string &node_name, const std::string &slot_name, SlotVersion version);

        /// Handle gossip peer node's new node notify. If the new node is not connected, this node will connect
        /// to it. Otherwise, this node will do nothing.
        /// \param name The name of new node
        /// \param address The address of new node
        /// \param version The version of new node
        /// \param origin Received message
        /// \return If connect to new node successfully
        bool handleNewNodeNotify(const std::string &name, const std::string &address, SlotVersion version,
                                 const ::pf_gossip::GossipNodeInfo *origin);

        /// Handle gossip peer node's node delete notify. This node will set that node unhealthy and then check
        /// its real status.
        /// \param name The name of deleted node
        /// \param address The address of deleted node
        /// \param version The version of deleted node
        /// \param origin Received message
        void handleDeleteNodeNotify(const std::string &name, const std::string &address, SlotVersion version,
                                    const ::pf_gossip::GossipNodeInfo *origin);



        /// Send connect request to a gossip node, if succeed peer node will send notification to other
        /// gossip nodes that peer has connected to.
        /// \param address ip address of peer node
        /// \return Whether connection is established. If succeed, returns "". Otherwise, returns error information
        std::string startConnection(const std::string &address);

        /// Start a gossip transmit of heartbeat swap. This function randomly select a peer node to
        /// swap this node's local slot version.
        void startHeartBeat();

        /// Start a gossip transmit of node delete. This only happens when this node is down.
        /// Function randomly select some nodes to send notification rpc.
        void startDeleteNodeNotify();

        /// Start a gossip transmit of new node. This will happen only when this node received a new
        /// node's connection request. Function randomly select some nodes to send notification rpc.
        /// \param name New gossip node's name
        /// \param address New gossip node's ip address
        /// \param version New gossip node's version
        void startNewNodeNotify(const std::string &name, const std::string &address, SlotVersion version);

        /// Add a retry task in gossip retry tasks.
        /// \param call A rpc async object
        void addRetryTask(GossipPeerNode::AsyncClientCall<google::protobuf::Any, google::protobuf::Any> *call);

    private:

        /// Run daemon task of this gossip node.
        void DaemonTask();

        /// Check the status of unhealthy gossip peer node. Node that is not communicated more than 30
        /// seconds and is not attachable will be removed. This is to avoid some nodes ups and downs in a
        /// short time eval and received its information more than one time.
        void removeUnHealthyNode();

        /// Pull local slot to nodes whose version is lower than local version.
        void pullSlotToLowerNode();

        /// Check the health of peer gossip node. If peer is down, transfer it to unhealthy_peers list.
        void checkPeerHealth();

        /// Retry failed rpc.
        void doRetryTask();


        void doGossipTask();

        /// Random select a gossip node and returns its client's pointer.
        /// \return Ptr of selected GossipPeerNode
        GossipPeerNode *randomSelectGossipNode();

        /// Randomly select some gossip nodes that hasn't received message.
        /// \param passed_nodes Nodes that message has passed
        /// \param nums The num of nodes to be selected, default 3
        /// \return Selected gossip node list
        std::list<GossipPeerNode *>
        randomSelectGossipNodes(const google::protobuf::RepeatedPtrField<std::string> &passed_nodes, size_t nums = 3);

    private:

        std::string name_;  // 节点的名称
        GossipSlot *local_slot_;    // 本地slot指针
        std::string internal_address_, external_address_;   // 内外地址
        std::string token_;
        size_t mem_use_ = 0;    // 记录当前的数据库内存使用

        std::unordered_map<std::string, GossipSlot> slots_;    // gossip 节点所维护的数据表


        std::unordered_map<std::string, server::GossipPeerNode *> peers_;  // 用来存储已连接的 gossip node
        std::vector<GossipPeerNode *> peers_list_;  // 用于随机选择 node
        std::list<GossipPeerNode *> unhealthy_peers_;

        std::mutex ls_mtx;  // 用于保护lower_slots
        std::list<std::string> lower_slots;    // 通过心跳包可以看到有哪些节点的版本是较低的

        std::mutex mtx;
        std::list<RetryCall *> retry_list_;

        grpc::Service *internal_service_, *external_service_;  // 用于构建 gossip 服务
        std::unique_ptr<grpc::Server> internal_server, external_server;

        std::thread th1_, th2_;
        bool quit = false;
    };


}


#endif //GOSSIP_GOSSIPNODE_H

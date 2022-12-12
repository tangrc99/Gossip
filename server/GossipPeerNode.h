//
// Created by 唐仁初 on 2022/9/14.
//

#ifndef GOSSIP_GOSSIPPEERNODE_H
#define GOSSIP_GOSSIPPEERNODE_H

#include "Gossip.grpc.pb.h"
#include "GossipSlot.h"
#include <grpcpp/grpcpp.h>
#include <thread>

namespace gossip::server {

    using pf_gossip::Gossip;

    class GossipNode;

    /// @brief A client connecting to another gossip node
    /// @details Class GossipPeerNode is a async rpc client of Gossip Service and stores status of peer node.
    class GossipPeerNode {

    public:

        /// Constructor of GossipPeerNode.
        /// \param name The name of peer node
        /// \param address The ip address of peer node
        /// \param version The version of peer node
        /// \param node The owner node of this class
        explicit GossipPeerNode(std::string name, std::string address, SlotVersion version, GossipNode *node);

        /// Destructor of GossipPeerNode, it will release thread and grpc channel.
        ~GossipPeerNode();

        /// Get the name of peer node.
        /// \return The name of peer node
        std::string name() const {
            return name_;
        }

        /// Get the ip address of peer node.
        /// \return The ip address of peer node
        std::string address() {
            return address_;
        }

        /// Get the version of peer node.
        /// \return The version of peer node
        SlotVersion version() const {
            return version_;
        }

        /// Async. Send a pull request to connected peer node. The node will check the slot message, if sender has a
        /// higher version, it will update slot and then propagate pull request.
        /// If pull request is failed because of rpc disconnection, it will retry.
        /// \param origin Sender's slot information
        void pull(const pf_gossip::SlotUpdate *origin);


        /// Search message on peer node. if message is found, returns the owner slot of this message
        /// \param key The key of message
        /// \return The slot name of message
        std::string search(const std::string &key); //FIXME

        /// Sync. Send a echo request to connected peer node. Message to send is "request,hello",
        /// and received should be "echo,hello". If echo succeed, it will update peer node status.
        /// \return If echo succeed
        bool echo();

        /// Sync. Send a connect request to connected peer node. If connection is failed, this class
        /// should be released or set unhealthy.
        /// \return If connection is established, returns {}. Otherwise, returns error information
        std::string establishConnection();

        /// Async. Send a heartbeat request to connected peer node and share slot version.
        void heartBeat();

        /// Async. Send a new node notify request to connected peer node. Peer node will check
        /// and connect to the new node and then propagate this message.
        /// \param name The name of new node
        /// \param address The address of new node
        /// \param version The version of new node
        void newNodeNotify(const std::string &name, const std::string &address, SlotVersion version);

        /// Async. Send a new node notify request to connected peer node. Peer node will check
        /// and connect to the new node and then propagate this message.
        /// \param origin Received message
        void newNodeNotify(const pf_gossip::GossipNodeInfo *origin);

        /// Async. Send a node delete notify request to connected peer node. Peer node will check
        /// and delete this node and then propagate this message.
        /// \param name The name of deleted node
        /// \param address The address of deleted node
        /// \param version The version of deleted node
        void deleteNodeNotify(const std::string &name, const std::string &address, SlotVersion version);

        /// Async. Send a node delete notify request to connected peer node. Peer node will check
        /// and delete this node and then propagate this message.
        /// \param origin Received message
        void deleteNodeNotify(const pf_gossip::GossipNodeInfo *origin);

        /// Get last rpc timestamp of peer node.
        /// \return Last rpc timestamp
        time_t timestamp() const {
            return timestamp_;
        }

        /// Get alive status of peer node. If a rpc is not response, the peer node will be set not alive.
        /// \return Is peer node alive
        bool isAlive() const {
            return alive_;
        }

        /// Set peer node manually. The alive status will also be updated by this class automatically.
        void setNotAlive() {
            alive_ = false;
        }

        /// @brief GRPC Asynchronous invocation class.
        /// @details Handle the async rpc result in daemon thread. If rpc is succeed, the callback of AsyncClientCall
        /// will run. Otherwise, the retry function of AsyncClientCall will run.
        /// @note All AsyncClientCall should be deleted in this function.
        void receiveResult();

        /// Class AsyncClientCall is used to handle async rpc request. All objects used in async rpc is managed
        /// by this class.
        /// \tparam Input Input type of async rpc
        /// \tparam Output Output type of async rpc
        template<class Input, class Output>
        struct AsyncClientCall {
            /// The input of rpc
            Input *input_;
            /// The output of rpc
            Output *reply_;
            /// The context of rpc
            grpc::ClientContext context_;
            /// The status of rpc
            grpc::Status status_;
            /// rpc result reader
            std::unique_ptr<grpc::ClientAsyncResponseReader<Output>> reader_;
            /// function to handle rpc output
            std::function<void()> cb_ = {};
            /// retry function
            std::function<void(GossipPeerNode *)> retry_ = {};

            /// Constructor of AsyncClientCall. Construct all objects needed in async rpc.
            AsyncClientCall();

            /// Destructor of AsyncClientCall. Release all objects needed in async rpc.
            ~AsyncClientCall();

            /// Start async rpc.
            void start();

            /// Retry async rpc.
            void retry(GossipPeerNode *node) {
                auto func = retry_;
                reply_ = {};
                func(node);
            }
        };

    private:

        /// Update the rpc timestamp.
        void updateTimestamp() {
            alive_ = true;
            timestamp_ = time(nullptr);
        }

    private:

        std::string name_;      // peer_node_name
        std::string address_;    // peer_address
        bool alive_;    // 对方节点是否存活
        time_t timestamp_;  // 上次进行通信的时间戳

        // 为什么需要 version_？ 如果一个结点在上线后，又下线了那么可能会出现一些问题

        long version_;  // 如果一个结点反复地上下线，那么需要根据version来判断

        std::unique_ptr<pf_gossip::Gossip::Stub> stub_;
        grpc::CompletionQueue rpc_queue;
        std::thread async_th_;

        GossipNode *node_;
    };


}


#endif //GOSSIP_GOSSIPPEERNODE_H

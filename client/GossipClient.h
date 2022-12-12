//
// Created by 唐仁初 on 2022/9/16.
//

#ifndef GOSSIP_GOSSIPCLIENT_H
#define GOSSIP_GOSSIPCLIENT_H

#include "GossipCli.pb.h"
#include "GossipCli.grpc.pb.h"

#include <nlohmann/json.hpp>
#include <grpcpp/grpcpp.h>
#include <thread>
#include <utility>

namespace gossip::client {


    /// @brief A client connecting to a gossip node.
    /// @details Class GossipClient is the user client of GossipNode. Users can use this class to send CRUD request
    /// or gossip status command to a GossipNode Impl.
    class GossipClient {
    public:

        /// @brief Wrapper return value
        /// @details Class Result contains rpc status and rpc result. User can identify why an operation is failed.
        /// \tparam T The type of rpc result
        template<class T>
        class Result {
        public:

            /// Constructor of Result. If an operation succeed, error_text will be empty. Otherwise, value will
            /// be empty.
            /// \param suc Is operation succeeded
            /// \param error_text If operation failed, its failed reason
            /// \param value If operation succeeded, its returned value
            /// @note error_text and value shouldn't be both empty or not empty.
            explicit Result(bool suc, std::string error_text, T value = {}) : succeed_(suc),
                                                                              error_text_(std::move(error_text)),
                                                                              value_(value) {
            }

            /// Check is operation succeeded.
            /// \return Operation status
            [[nodiscard]] bool isSucceed() const {
                return succeed_;
            }

            /// Get the failed reason of operation.
            /// \return Failed reason
            [[nodiscard]] std::string errorText() const {
                return error_text_;
            }

            /// Get the result of operation.
            /// \return Operation result
            [[nodiscard]] T value() const {
                return value_;
            }

        private:
            T value_;
            std::string error_text_;
            bool succeed_{};
        };

    public:
        /// Constructor of GossipClient.
        /// \param address Ip address of gossip node to connect
        /// \param token Token to connect to gossip node
        explicit GossipClient(std::string address, std::string token);

        /// Insert or update a message to connected gossip node. The message will use arrived time as version,
        /// so this operation will not fail because of version check.
        /// \param key The key of message
        /// \param value The value of message
        /// \return Insert result
        Result<bool> insertOrUpdateMessage(const std::string &key, const std::string &value);

        /// Get the value of message by key. This operation will only search local slot.
        /// \param key The key of message
        /// \return Search result
        /// @note Use getMessageFromAllNode if you want to search message from all gossip nodes.
        Result<std::string> getMessage(const std::string &key);

        /// Get the value of message by key. This operation will start search operation in all slots.
        /// If param latest is set true, this operation will get latest version of value.
        /// \param key The key of message
        /// \param latest If latest version is needed
        /// \return Search result
        /// @note If latest is set false, this operation will only search on connected node. This means message
        /// may be not newest.
        Result<pf_gossip::SearchResult> getMessageFromAllNode(const std::string &key, bool latest = false);

        /// Delete a message by key. This operation will only delete message in local slot.
        /// \param key The key of message
        /// \return Delete result
        Result<bool> deleteMessage(const std::string &key);

        /// Send a command to connected node that make it connects to another node with assigned address.
        /// This operation can connect two gossip cluster.
        /// \param address The address of node to connect
        /// \return Connection result
        Result<bool> connectToNode(const std::string &address);

        /// Send a command to connected node that make it quit.
        /// \return Shutdown result
        Result<bool> shutdown();

        /// Send a command to connected node to get its current status, which contains node's name, address,
        /// memory use and peer nodes' information.
        /// \return Node status of Json type.
        Result<nlohmann::json> getNodeStatus();

        /// Get the ip address of connected gossip node.
        /// \return Ip address of gossip node
        [[nodiscard]] std::string peerAddress() const {
            return address_;
        }

    private:

        /// Get grpc timeout time point.
        /// \param second Timeout period
        /// \return Timeout point
        static gpr_timespec delay(int second) {
            return gpr_timespec{second, 0, GPR_TIMESPAN};
        }

    private:

        std::string address_;
        std::string token_;
        std::unique_ptr<pf_gossip_cli::GossipClient::Stub> stub_;
    };


}

#endif //GOSSIP_GOSSIPCLIENT_H

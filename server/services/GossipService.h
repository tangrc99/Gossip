//
// Created by 唐仁初 on 2022/9/14.
//

#ifndef GOSSIP_GOSSIPSERVICE_H
#define GOSSIP_GOSSIPSERVICE_H

#include "Gossip.grpc.pb.h"


namespace gossip::server {

    class GossipNode;

}

namespace gossip::server::service {


    /// @brief GRPC interface to handle other gossip node's request.
    /// @details Class GossipService is derived pf_gossip::Gossip::Service and defines function. This class
    /// must be owned by a Class GossipNode impl.
    class GossipService : public pf_gossip::Gossip::Service {

    public:
        /// Constructor of GossipService.
        /// \param node The owner of this class
        explicit GossipService(GossipNode *node) : node_(node) {

        }

        /// Receive and handle search request from a gossip node.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status search(::grpc::ServerContext *context, const ::pf_gossip::Message *request,
                              ::pf_gossip::SearchResult *response) override;

        /// Receive and handle pull request from a gossip node.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status pull(::grpc::ServerContext *context, const ::pf_gossip::SlotUpdate *request,
                            ::pf_gossip::updateResult *response) override;

        /// Receive and handle echo request from a gossip node.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status echo(::grpc::ServerContext *context, const ::pf_gossip::Message *request,
                            ::pf_gossip::Message *response) override;

        /// Receive and handle connect request from a gossip node.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status EstablishConnection(::grpc::ServerContext *context, const ::pf_gossip::GossipNodeInfo *request,
                                           ::pf_gossip::GossipNodeInfo *response) override;

        /// Receive and handle heartbeat request from a gossip node.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status heartBeat(::grpc::ServerContext *context, const ::pf_gossip::NodeVersions *request,
                                 ::pf_gossip::NodeVersions *response) override;

        /// Receive and handle new node notify request from a gossip node.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status newNodeNotify(::grpc::ServerContext *context, const ::pf_gossip::GossipNodeInfo *request,
                                     ::pf_gossip::updateResult *response) override;

        /// Receive and handle node delete notify request from a gossip node.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status deleteNodeNotify(::grpc::ServerContext *context, const ::pf_gossip::GossipNodeInfo *request,
                                        ::pf_gossip::updateResult *response) override;


    private:
        GossipNode *node_;  // the owner of this class
    };
}


#endif //GOSSIP_GOSSIPSERVICE_H

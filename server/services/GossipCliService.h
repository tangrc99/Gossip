//
// Created by 唐仁初 on 2022/9/16.
//

#ifndef GOSSIP_GOSSIPCLISERVICE_H
#define GOSSIP_GOSSIPCLISERVICE_H

#include "GossipCli.grpc.pb.h"
#include "GossipCli.pb.h"

namespace gossip::server{

    class GossipNode;

}

namespace gossip::server::service {


    /// @brief GRPC interface to handle client's request.
    /// @details Class GossipCliService is derived from pf_gossip_cli::GossipClient::Service and defines its functions.
    /// This class must be owned by a Class GossipNode impl.
    class GossipCliService final : public pf_gossip_cli::GossipClient::Service {
    public:

        /// Constructor of GossipCliService.
        /// \param node The owner of this class
        explicit GossipCliService(GossipNode *node);

        /// Receive and handle echo request from a gossip client.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status echo(::grpc::ServerContext *context, const ::pf_gossip_cli::Echo *request,
                            ::pf_gossip_cli::Echo *response) override;

        // CRUD request Interface From a gossip client.

        /// Receive and handle Create and Update request from a gossip client.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status addMessage(::grpc::ServerContext *context, const ::pf_gossip::Message *request,
                                  ::pf_gossip_cli::Echo *response) override;

        /// Receive and handle Delete request from a gossip client.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status deleteMessage(::grpc::ServerContext *context, const ::pf_gossip_cli::Key *request,
                                     ::pf_gossip_cli::Echo *response) override;

        /// Receive and handle Read request from a gossip client.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status getMessage(::grpc::ServerContext *context, const ::pf_gossip_cli::Key *request,
                                  ::pf_gossip::Message *response) override;

        /// Receive and handle Read request from a gossip client.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status searchMessage(::grpc::ServerContext *context, const ::pf_gossip_cli::SearchInfo *request,
                                     ::pf_gossip::SearchResult *response) override;

        /// Receive and handle node status request from a gossip client.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status getNodeStatus(::grpc::ServerContext *context, const ::google::protobuf::Any *request,
                                     ::pf_gossip_cli::JsonValue *response) override;

        /// Receive and handle node network status from a gossip client.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status getGossipNetwork(::grpc::ServerContext *context, const ::google::protobuf::Any *request,
                                        ::pf_gossip_cli::JsonValue *response) override;

        // Commands Interface From a gossip client.

        /// Receive and handle connection commands from a gossip client.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status connect(::grpc::ServerContext *context, const ::pf_gossip_cli::Url *request,
                               ::pf_gossip_cli::Echo *response) override;

        /// Receive and handle shutdown commands from a gossip client.
        /// \param context The grpc context
        /// \param request The request of rpc
        /// \param response The response of rpc
        /// \return The status of rpc
        ::grpc::Status shutdown(::grpc::ServerContext *context, const ::google::protobuf::Any *request,
                                ::pf_gossip_cli::Echo *response) override;

    private:

        /// Check is client's token matches node' token.
        /// \param metadata Gossip client's grpc metadata
        /// \return Is token matched
        bool IsTokenMatched(const std::multimap<grpc::string_ref, grpc::string_ref> &metadata);

    private:

        GossipNode *node_; // the owner of this class

    };

}


#endif //GOSSIP_GOSSIPCLISERVICE_H

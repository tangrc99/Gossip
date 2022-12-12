//
// Created by 唐仁初 on 2022/9/14.
//

#include "GossipService.h"
#include "../GossipNode.h"

namespace gossip::server::service {


    ::grpc::Status GossipService::search(::grpc::ServerContext *context, const ::pf_gossip::Message *request,
                                         ::pf_gossip::SearchResult *response) try {
        return grpc::Status::OK;

    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }

    ::grpc::Status GossipService::pull(::grpc::ServerContext *context, const ::pf_gossip::SlotUpdate *request,
                                       ::pf_gossip::updateResult *response) try {

        if (request->name().empty()) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "Empty NodeName"};
        }

        SlotValues values;
        values.reserve(request->messages_size());
        for (int i = 0; i < request->messages_size(); i++) {
            auto &message = request->messages(i);
            values.emplace(message.key(), message.value());
        }

        std::vector<std::string> nodes;
        nodes.reserve(request->pass_nodes_size());
        for (int i = 0; i < request->pass_nodes_size(); i++) {
            nodes.emplace_back(request->pass_nodes(i));
        }

        auto version = node_->handlePullRequest(request->name(), values, request->version(), request);

        response->set_version(version);
        response->set_succeed(request->messages_size());

        return grpc::Status::OK;

    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }

    ::grpc::Status GossipService::echo(::grpc::ServerContext *context, const ::pf_gossip::Message *request,
                                       ::pf_gossip::Message *response) try {

        if (request->key() != "echo") {
            return {grpc::StatusCode::INVALID_ARGUMENT, "Invalid Message"};
        }

        response->set_key("echo");
        response->set_value("hello");
        return grpc::Status::OK;

    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }

    ::grpc::Status
    GossipService::EstablishConnection(::grpc::ServerContext *context, const ::pf_gossip::GossipNodeInfo *request,
                                       ::pf_gossip::GossipNodeInfo *response) try {

        if (request->name().empty()) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "Empty NodeName"};
        }

        ::pf_gossip::GossipNodeInfo pass_nodes;
        pass_nodes.add_pass_nodes(node_->NodeName());
        node_->handleNewNodeNotify(request->name(), request->address(), request->version(), &pass_nodes);

        response->set_name(node_->NodeName());
        response->set_version(node_->localSlot()->version());
        response->set_address(request->address());  // FIXME

        return grpc::Status::OK;
    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }


    ::grpc::Status GossipService::heartBeat(::grpc::ServerContext *context, const ::pf_gossip::NodeVersions *request,
                                            ::pf_gossip::NodeVersions *response) try {

        if (request->node_name().empty()) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "Empty NodeName"};
        }

        auto this_version = node_->handleHeartbeatRequest(request->node_name(), request->slot_name(),
                                                          request->slot_version());

        response->set_node_name(node_->NodeName());
        response->set_slot_name(request->slot_name());
        response->set_slot_version(this_version);

        return grpc::Status::OK;
    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }

    ::grpc::Status
    GossipService::newNodeNotify(::grpc::ServerContext *context, const ::pf_gossip::GossipNodeInfo *request,
                                 ::pf_gossip::updateResult *response) try {

        if (request->name().empty()) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "Empty NodeName"};
        }

        node_->handleNewNodeNotify(request->name(), request->address(), request->version(), request);

        response->set_succeed(1);
        response->set_version(11111111111);
        return grpc::Status::OK;
    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }

    ::grpc::Status
    GossipService::deleteNodeNotify(::grpc::ServerContext *context, const ::pf_gossip::GossipNodeInfo *request,
                                    ::pf_gossip::updateResult *response) try {

        if (request->name().empty()) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "Empty NodeName"};
        }

        node_->handleDeleteNodeNotify(request->name(), request->address(), request->version(), request);

        response->set_succeed(1);
        response->set_version(11111111111);

        return grpc::Status::OK;
    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }


}


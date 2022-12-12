//
// Created by 唐仁初 on 2022/9/16.
//

#include "GossipCliService.h"
#include "../GossipNode.h"

namespace gossip::server::service {

    GossipCliService::GossipCliService(GossipNode *node) : node_(node) {

    }

    ::grpc::Status GossipCliService::echo(::grpc::ServerContext *context, const ::pf_gossip_cli::Echo *request,
                                          ::pf_gossip_cli::Echo *response) {

        // 如果没有设置 token 则不需要进行验证
        if(node_->Token().empty()){
            response->set_succeed(true);
            response->set_message(request->message());
            return grpc::Status::OK;
        }

        // 本地的地址不需要进行验证，直接返回一个 token
        if(context->peer().find("127.0.0.1") != std::string::npos){
            context->AddTrailingMetadata("token",node_->Token());
            response->set_succeed(true);
            response->set_message(request->message());
            return grpc::Status::OK;
        }

        auto metadata = context->client_metadata();

        auto it = metadata.find("token");
        if(it == metadata.end()){
            response->set_succeed(false);
            response->set_message("No Token");
            return grpc::Status::OK;
        }else if(it->second != node_->Token()){
            response->set_succeed(false);
            response->set_message("Token Not Matched");
            return grpc::Status::OK;
        }

        response->set_succeed(true);
        response->set_message(request->message());
        return grpc::Status::OK;
    }


    ::grpc::Status GossipCliService::addMessage(::grpc::ServerContext *context, const ::pf_gossip::Message *request,
                                                ::pf_gossip_cli::Echo *response) try {

        if(request->key().empty()){
            return {grpc::StatusCode::INVALID_ARGUMENT,"Empty Key"};
        }

        if(!IsTokenMatched(context->client_metadata())){
            return {grpc::StatusCode::FAILED_PRECONDITION,"Token Not Matched"};
        }


        auto version = node_->insertOrUpdateMessage(request->key(), request->value());

        response->set_succeed(version != -1);

        return grpc::Status::OK;

    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }

    ::grpc::Status GossipCliService::deleteMessage(::grpc::ServerContext *context, const ::pf_gossip_cli::Key *request,
                                                   ::pf_gossip_cli::Echo *response) try {

        if(request->content().empty()){
            return {grpc::StatusCode::INVALID_ARGUMENT,"Empty Key"};
        }

        if(!IsTokenMatched(context->client_metadata())){
            return {grpc::StatusCode::FAILED_PRECONDITION,"Token Not Matched"};
        }

        auto version = node_->deleteMessage(request->content());

        response->set_succeed(version > 0);
        response->set_message("No such key");

        return grpc::Status::OK;

    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }

    ::grpc::Status GossipCliService::getMessage(::grpc::ServerContext *context, const ::pf_gossip_cli::Key *request,
                                                ::pf_gossip::Message *response) try {

        if(request->content().empty()){
            return {grpc::StatusCode::INVALID_ARGUMENT,"Empty Key"};
        }

        if(!IsTokenMatched(context->client_metadata())){
            return {grpc::StatusCode::FAILED_PRECONDITION,"Token Not Matched"};
        }

        auto value = node_->getMessage(request->content());

        response->set_key(request->content());
        response->set_value(value);

        return grpc::Status::OK;

    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }

    ::grpc::Status
    GossipCliService::searchMessage(::grpc::ServerContext *context, const ::pf_gossip_cli::SearchInfo *request,
                                    ::pf_gossip::SearchResult *response) try {

        if(request->key().empty()){
            return {grpc::StatusCode::INVALID_ARGUMENT,"Empty Key"};
        }

        if(!IsTokenMatched(context->client_metadata())){
            return {grpc::StatusCode::FAILED_PRECONDITION,"Token Not Matched"};
        }

        auto values = node_->searchMessage(request->key(), request->latest());

        response->MergeFrom(values);

        return grpc::Status::OK;

    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }


    ::grpc::Status
    GossipCliService::getGossipNetwork(::grpc::ServerContext *context, const ::google::protobuf::Any *request,
                                       ::pf_gossip_cli::JsonValue *response) try {


        return grpc::Status::OK;

    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }

    ::grpc::Status
    GossipCliService::getNodeStatus(::grpc::ServerContext *context, const ::google::protobuf::Any *request,
                                    ::pf_gossip_cli::JsonValue *response) try{

        if(!IsTokenMatched(context->client_metadata())){
            return {grpc::StatusCode::FAILED_PRECONDITION,"Token Not Matched"};
        }

        auto status = node_->getNodeStatus();

        response->set_content(status.dump());

        return grpc::Status::OK;

    }catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }


    ::grpc::Status GossipCliService::connect(::grpc::ServerContext *context, const ::pf_gossip_cli::Url *request,
                                             ::pf_gossip_cli::Echo *response) try {

        if(!IsTokenMatched(context->client_metadata())){
            return {grpc::StatusCode::FAILED_PRECONDITION,"Token Not Matched"};
        }

        // 这里需要对 url 进行检查吗

        auto conn_res = node_->startConnection(request->content());

        response->set_succeed(conn_res.empty());
        response->set_message(conn_res);

        return grpc::Status::OK;

    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }

    ::grpc::Status GossipCliService::shutdown(::grpc::ServerContext *context, const ::google::protobuf::Any *request,
                                              ::pf_gossip_cli::Echo *response) try {

        if(!IsTokenMatched(context->client_metadata())){
            return {grpc::StatusCode::FAILED_PRECONDITION,"Token Not Matched"};
        }

        auto stop_res = node_->stop();

        response->set_succeed(stop_res);

        return grpc::Status::OK;
    } catch (std::exception &exception) {

        std::cerr << exception.what() << std::endl;
        return {grpc::StatusCode::ABORTED, "Exception Threw"};
    }


    bool GossipCliService::IsTokenMatched(const std::multimap<grpc::string_ref, grpc::string_ref>& metadata) {
        // 如果没有设置 token 则不需要进行验证
        if(node_->Token().empty()){
            return true;
        }

        auto it = metadata.find("token");
        if(it == metadata.end() || it->second != node_->Token()){
            return false;
        }

        return true;
    }




}

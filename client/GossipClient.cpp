//
// Created by 唐仁初 on 2022/9/16.
//

#include "GossipClient.h"

#include <utility>

namespace gossip::client {

    GossipClient::GossipClient(std::string address,std::string token) : address_(std::move(address)),token_(std::move(token)) {

        stub_ = pf_gossip_cli::GossipClient::NewStub(
                grpc::CreateChannel(address_, grpc::InsecureChannelCredentials()));

        grpc::ClientContext context;
        context.AddMetadata("token",token_);
        context.set_deadline(delay(2));

        pf_gossip_cli::Echo request,response;
        request.set_message("hello");

        auto status = stub_->echo(&context,request,&response);

        if(status.ok()){
            if(request.message() != response.message())
                throw std::runtime_error("Unpaired Token");

            if(token_.empty()){
                auto str_ref = context.GetServerTrailingMetadata().find("token")->second;
                token_ =  {str_ref.data(),str_ref.size()};
            }
        }
    }

    GossipClient::Result<bool>
    GossipClient::insertOrUpdateMessage(const std::string &key, const std::string &value) try {

        grpc::ClientContext context;
        context.AddMetadata("token",token_);

        pf_gossip::Message request;
        request.set_key(key);
        request.set_value(value);
        pf_gossip_cli::Echo echo;

        auto status = stub_->addMessage(&context, request, &echo);

        if (!status.ok()) {
            return Result<bool>(false, status.error_message());
        }
        return Result<bool>(true, echo.message(), echo.succeed());

    } catch (std::exception &exception) {
        return Result<bool>(false, exception.what());
    }

    GossipClient::Result<std::string> GossipClient::getMessage(const std::string &key) try {

        grpc::ClientContext context;
        context.AddMetadata("token",token_);

        pf_gossip_cli::Key request;
        request.set_content(key);
        pf_gossip::Message response;

        auto status = stub_->getMessage(&context, request, &response);

        if (!status.ok()) {
            return Result<std::string>(false, status.error_message());
        }
        return Result<std::string>(true, "", response.value());

    } catch (std::exception &exception) {
        return Result<std::string>(false, exception.what());
    }

    GossipClient::Result<bool> GossipClient::deleteMessage(const std::string &key) try {

        grpc::ClientContext context;
        context.AddMetadata("token",token_);

        pf_gossip_cli::Key request;
        request.set_content(key);
        pf_gossip_cli::Echo response;

        auto status = stub_->deleteMessage(&context, request, &response);

        if (!status.ok()) {
            return Result<bool>(false, status.error_message());
        }
        return Result<bool>(true, response.message(), response.succeed());

    } catch (std::exception &exception) {
        return Result<bool>(false, exception.what());
    }

    GossipClient::Result<bool> GossipClient::connectToNode(const std::string &address) try {

        grpc::ClientContext context;
        context.AddMetadata("token",token_);

        pf_gossip_cli::Url url;
        url.set_content(address);
        pf_gossip_cli::Echo echo;

        auto status = stub_->connect(&context, url, &echo);

        if (!status.ok()) {
            return Result<bool>(false, status.error_message());
        }

        return Result<bool>(true, echo.message(), echo.succeed());

    } catch (std::exception &exception) {
        return Result<bool>(false, exception.what());
    }

    GossipClient::Result<bool> GossipClient::shutdown() try {

        grpc::ClientContext context;
        context.AddMetadata("token",token_);

        google::protobuf::Any any;
        pf_gossip_cli::Echo response;
        auto status = stub_->shutdown(&context, any, &response);

        if (!status.ok()) {
            return Result<bool>(false, status.error_message());
        }

        return Result<bool>(true, response.message(), response.succeed());

    } catch (std::exception &exception) {
        return Result<bool>(false, exception.what());
    }

    GossipClient::Result<pf_gossip::SearchResult>
    GossipClient::getMessageFromAllNode(const std::string &key, bool latest) try {

        grpc::ClientContext context;
        context.AddMetadata("token",token_);

        pf_gossip_cli::SearchInfo searchInfo;
        searchInfo.set_key(key);
        searchInfo.set_latest(latest);
        pf_gossip::SearchResult response;

        auto status = stub_->searchMessage(&context, searchInfo, &response);

        if (!status.ok()) {
            return Result<pf_gossip::SearchResult>(false, status.error_message());
        }

        return Result<pf_gossip::SearchResult>(true, "", response);

    } catch (std::exception &exception) {
        return Result<pf_gossip::SearchResult>(false, exception.what());
    }

    GossipClient::Result<nlohmann::json> GossipClient::getNodeStatus() try {
        grpc::ClientContext context;
        context.AddMetadata("token",token_);

        google::protobuf::Any request;
        pf_gossip_cli::JsonValue response;

        auto status = stub_->getNodeStatus(&context, request, &response);

        if (!status.ok()) {
            return Result<nlohmann::json>{false, status.error_message()};
        }

        return Result<nlohmann::json>{true, "", nlohmann::json::parse(response.content())};

    } catch (std::exception &exception) {
        return Result<nlohmann::json>(false, exception.what());
    }
}
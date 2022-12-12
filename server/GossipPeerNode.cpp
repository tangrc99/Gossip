//
// Created by 唐仁初 on 2022/9/14.
//

#include "GossipPeerNode.h"
#include "GossipNode.h"
#include "Gossip.pb.h"

#include <grpcpp/grpcpp.h>
#include <utility>


namespace gossip::server {

    GossipPeerNode::GossipPeerNode(std::string name, std::string address, SlotVersion version, GossipNode *node)
            : name_(std::move(name)), address_(std::move(address)), version_(version), node_(node),
              timestamp_(time(nullptr)), alive_(true) {

        stub_ = Gossip::NewStub(grpc::CreateChannel(address_, grpc::InsecureChannelCredentials()));
        async_th_ = std::thread([this] { receiveResult(); });
    }

    GossipPeerNode::~GossipPeerNode() {
        rpc_queue.Shutdown();

        if (async_th_.joinable())
            async_th_.join();
    }


    // 发起用的接口，因为这里默认每个结点只会发送自己的slot
    void GossipPeerNode::pull(const pf_gossip::SlotUpdate *origin) {

        auto rpc_call = new AsyncClientCall<pf_gossip::SlotUpdate, pf_gossip::updateResult>();

        if (origin != nullptr) {
            rpc_call->input_->MergeFrom(*origin);

        } else {
            auto slot = node_->localSlot();

            rpc_call->input_->set_name(slot->name());
            rpc_call->input_->set_version(slot->version());

            for (auto &m: slot->messages()) {
                auto message = rpc_call->input_->add_messages();
                message->set_key(m.first);
                message->set_value(m.second);
            }
        }

        rpc_call->input_->add_pass_nodes(node_->NodeName());

        rpc_call->reader_ = stub_->PrepareAsyncpull(&rpc_call->context_, *rpc_call->input_, &rpc_queue);
        rpc_call->cb_ = []() {
            return;
        };

        rpc_call->retry_ = [rpc_call](GossipPeerNode *cli) {
            rpc_call->reader_ = cli->stub_->PrepareAsyncpull(&rpc_call->context_, *rpc_call->input_, &cli->rpc_queue);
            rpc_call->start();
        };

        rpc_call->start();

    }

    std::string GossipPeerNode::search(const std::string &key) {

    }

    bool GossipPeerNode::echo() {
        grpc::ClientContext context;
        pf_gossip::Message message, response;
        message.set_key("request");
        message.set_value("hello");
        auto status = stub_->echo(&context, message, &response);

        if (status.ok()) {
            updateTimestamp();
        }
        return status.ok();
    }

    void GossipPeerNode::heartBeat() {

        auto rpc_call = new AsyncClientCall<pf_gossip::NodeVersions, pf_gossip::NodeVersions>();

        auto slot = node_->localSlot();

        rpc_call->input_->set_node_name(node_->NodeName());
        rpc_call->input_->set_slot_name(slot->name());
        rpc_call->input_->set_slot_version(slot->version());


        rpc_call->reader_ = stub_->PrepareAsyncheartBeat(&rpc_call->context_, *rpc_call->input_, &rpc_queue);
        rpc_call->cb_ = [rpc_call, this]() {

            node_->handleHeartbeatRequest(rpc_call->reply_->node_name(), rpc_call->reply_->slot_name(),
                                          rpc_call->reply_->slot_version());

        };

        rpc_call->start();
    }

    void GossipPeerNode::newNodeNotify(const std::string &name, const std::string &address, SlotVersion version) {

        auto rpc_call = new AsyncClientCall<pf_gossip::GossipNodeInfo, pf_gossip::updateResult>();

        rpc_call->input_->set_name(name);
        rpc_call->input_->set_address(address);
        rpc_call->input_->set_version(version);


        rpc_call->reader_ = stub_->PrepareAsyncnewNodeNotify(&rpc_call->context_, *rpc_call->input_, &rpc_queue);
        rpc_call->cb_ = []() {
            return;
        };
        rpc_call->start();

    }

    void GossipPeerNode::newNodeNotify(const pf_gossip::GossipNodeInfo *origin) {
        auto rpc_call = new AsyncClientCall<pf_gossip::GossipNodeInfo, pf_gossip::updateResult>();

        if (origin != nullptr) {
            rpc_call->input_->MergeFrom(*origin);
        }

        rpc_call->input_->add_pass_nodes(node_->NodeName());

        rpc_call->reader_ = stub_->PrepareAsyncnewNodeNotify(&rpc_call->context_, *rpc_call->input_, &rpc_queue);
        rpc_call->cb_ = []() {
            return;
        };

        rpc_call->start();


    }


    void
    GossipPeerNode::deleteNodeNotify(const std::string &name, const std::string &address, SlotVersion version) {
        auto rpc_call = new AsyncClientCall<pf_gossip::GossipNodeInfo, pf_gossip::updateResult>();

        rpc_call->input_->set_name(name);
        rpc_call->input_->set_address(address);
        rpc_call->input_->set_version(version);


        rpc_call->reader_ = stub_->PrepareAsyncdeleteNodeNotify(&rpc_call->context_, *rpc_call->input_, &rpc_queue);
        rpc_call->cb_ = []() {
            return;
        };
        rpc_call->start();


    }

    void GossipPeerNode::deleteNodeNotify(const pf_gossip::GossipNodeInfo *origin) {
        auto rpc_call = new AsyncClientCall<pf_gossip::GossipNodeInfo, pf_gossip::updateResult>();


        if (origin != nullptr) {
            rpc_call->input_->MergeFrom(*origin);
        }


        rpc_call->reader_ = stub_->PrepareAsyncdeleteNodeNotify(&rpc_call->context_, *rpc_call->input_, &rpc_queue);
        rpc_call->cb_ = []() {
            return;
        };
        rpc_call->start();
    }

    std::string GossipPeerNode::establishConnection() {

        grpc::ClientContext context;
        pf_gossip::GossipNodeInfo request;
        request.set_name(node_->NodeName());
        request.set_address(node_->internalAddress());
        request.set_version(node_->localSlot()->version());

        pf_gossip::GossipNodeInfo response;

        auto status = stub_->EstablishConnection(&context, request, &response);

        if (status.ok()) {
            name_ = response.name();
            address_ = response.address();
            version_ = response.version();
            return "";
        }

        return response.name();
    }

    void GossipPeerNode::receiveResult() {
        void *got_tag;
        bool ok = false;

        // Block until the next result is available in the completion queue "cq".
        while (rpc_queue.Next(&got_tag, &ok)) {
            // The tag in this example is the memory location of the call object
            auto call = static_cast<AsyncClientCall<google::protobuf::Any, google::protobuf::Any> *>(got_tag);

            if (call->status_.ok()) {

                updateTimestamp();

                call->cb_();

                // Once we're complete, deallocate the call object.
                delete call;

            } else {

                std::cerr << "PeerNode " << name_ << " " << address_ << std::endl;
                std::cerr << "RPC failed, reason: " << call->status_.error_code() << std::endl;

                if (call->status_.error_code() == grpc::UNAVAILABLE) {
                    alive_ = false;
                }

                // 如果没有绑定retry，那么进行删除
                if (!call->retry_) {
                    delete call;
                } else {
                    node_->addRetryTask(call);
                }

            }


        }
    }

    template<class Input, class Output>
    GossipPeerNode::AsyncClientCall<Input, Output>::AsyncClientCall() {
        input_ = new Input;
        reply_ = new Output;
    }

    template<class Input, class Output>
    GossipPeerNode::AsyncClientCall<Input, Output>::~AsyncClientCall() {
        delete input_;
        delete reply_;
    }

    template<class Input, class Output>
    void GossipPeerNode::AsyncClientCall<Input, Output>::start() {
        if (reader_ == nullptr) {
            std::cerr << "Not Bind Async RPC" << std::endl;
            return;
        }
        reader_->StartCall();
        reader_->Finish(reply_, &status_, (void *) this);
    }



}
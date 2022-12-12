//
// Created by 唐仁初 on 2022/9/14.
//

#include "GossipNode.h"
#include "GossipPeerNode.h"
#include "GossipSlot.h"

#include <unordered_map>
#include <string>
#include <utility>

namespace gossip::server {

    GossipNode::GossipNode(std::string name, std::string ex_addr, std::string in_addr, std::string token)
            : name_(std::move(name)), external_address_(std::move(ex_addr)), internal_address_(std::move(in_addr)),
              token_(std::move(token)) {

        if (name_.empty()) {
            throw std::runtime_error("Empty Node Name");
        }

        std::cout << "Gossip Node Name: \"" << name_ << "\"" << std::endl
                  << "Gossip External Address: \"" << external_address_ << "\"" << std::endl
                  << "Gossip Internal Address: \"" << internal_address_ << "\"" << std::endl
                  << "Gossip Entry Token: \"" << token_ << "\"" << std::endl;

        slots_.emplace(name_, GossipSlot(name_));
        local_slot_ = &slots_.at(name_);

        internal_service_ = new service::GossipService(this);
        external_service_ = new service::GossipCliService(this);

        if (!internal_address_.empty()) {
            grpc::ServerBuilder builder;
            builder.AddListeningPort(internal_address_, grpc::InsecureServerCredentials());
            builder.RegisterService(internal_service_);
            internal_server = builder.BuildAndStart();
        }
        if (!external_address_.empty()) {
            grpc::ServerBuilder builder;
            builder.AddListeningPort(external_address_, grpc::InsecureServerCredentials());
            builder.RegisterService(external_service_);
            external_server = builder.BuildAndStart();
        }


    }

    GossipNode::~GossipNode() {
        internal_server->Shutdown();
        external_server->Shutdown();
        if (th1_.joinable())
            th1_.join();
        if (th2_.joinable())
            th2_.join();
        delete internal_service_;
        delete external_service_;
    }

    void GossipNode::run() try {
        std::cout << "Gossip Node starts" << std::endl;
        th1_ = std::thread([this] {
            if (!internal_server)
                throw std::runtime_error("Internal Server Error");
            internal_server->Wait();
        });
        th2_ = std::thread([this] {
            if (!external_server)
                throw std::runtime_error("External Server Error");
            external_server->Wait();
        });

        DaemonTask();

    } catch (std::exception &e) {
        std::cerr << e.what();
        stop();
    }

    bool GossipNode::stop() {
        quit = true;
        startDeleteNodeNotify();
        std::cout << "ready to shutdown" << std::endl;
        return true;
    }

    SlotVersion GossipNode::insertOrUpdateMessage(const std::string &key, const std::string &value) {

        if (key.empty())
            return -1;

        std::cout << "insert message " << key << " " << value << std::endl;

        auto version = local_slot_->insertOrUpdate(key, value);

        std::cout << "inserted version " << version << std::endl;

        pf_gossip::SlotUpdate slot;
        slot.set_name(local_slot_->name());
        slot.set_version(version);
        slot.add_pass_nodes(NodeName());

        // 传播消息
        auto selected = randomSelectGossipNodes(slot.pass_nodes());
        for (auto &peer: selected) {
            peer->pull(&slot);    // 异步，不会进行阻塞
        }

        return version;
    }

    SlotVersion GossipNode::deleteMessage(const std::string &key) {

        if (key.empty())
            return -1;

        std::cout << "delete message " << key << std::endl;

        auto version = local_slot_->remove(key);

        if (version == -1)
            return -1;

        pf_gossip::SlotUpdate slot;
        slot.set_name(local_slot_->name());
        slot.set_version(version);
        slot.add_pass_nodes(NodeName());

        auto selected = randomSelectGossipNodes(slot.pass_nodes());
        for (auto &peer: selected) {
            peer->pull(&slot);    // 异步，不会进行阻塞
        }
        return version;
    }

    pf_gossip::SearchResult GossipNode::searchMessage(const std::string &key, bool latest) {

        pf_gossip::SearchResult res;

        // 如果要求返回最新的结果
        if (latest) {
            // FIXME
        }

        for (auto &[name, slot]: slots_) {

            auto[value, version] = slot.find(key);
            if (version == 0)
                continue;

            auto message = res.add_message();
            message->set_key(key);
            message->set_value(value);
            message->set_owner(name);
            message->set_version(version);
        }

        return std::move(res);
    }

    SlotVersion GossipNode::handlePullRequest(const std::string &slot, const SlotValues &values, SlotVersion version,
                                              const pf_gossip::SlotUpdate *origin) {

        std::cout << "slot: " << slot << " version: " << version << std::endl;

        if (slots_.find(slot) == slots_.end()) {
            slots_.emplace(slot, GossipSlot(slot));
        }

        // 如果是旧消息，则不会进行传播操作
        if (version < slots_.at(slot).version())
            return -1;

        // 传播
        auto selected = randomSelectGossipNodes(origin->pass_nodes());
        for (auto &peer: selected) {
            peer->pull(origin);    // 异步，不会进行阻塞
        }

        return slots_.at(slot).compareAndMergeSlot(values, version);
    }

    SlotVersion
    GossipNode::handleHeartbeatRequest(const std::string &node_name, const std::string &slot_name,
                                       SlotVersion version) {

        std::cout << "received heartbeat" << std::endl;

        auto it = slots_.find(slot_name);
        if (it == slots_.end())
            return -1;


        if (version < it->second.version())
            lower_slots.emplace_back(node_name);

        return it->second.version();
    }

    bool GossipNode::handleNewNodeNotify(const std::string &name, const std::string &address, SlotVersion version,
                                         const ::pf_gossip::GossipNodeInfo *origin) {

        if (address == internal_address_) {
            std::cerr << "Received Self's New Node Notify" << std::endl;
            return false;
        }

        // 如果断线重连，或者是已经收到通知，则什么都不做
        if (peers_.find(name) != peers_.end())
            return true;

        auto node = new GossipPeerNode(name, address, version, this);

        // 对消息进行传播
        auto selected = randomSelectGossipNodes(origin->pass_nodes());
        for (auto &peer: selected) {
            peer->newNodeNotify(origin);    // 异步，不会进行阻塞
        }

        auto ptr = peers_.emplace(name, node).first->second;
        peers_list_.emplace_back(ptr);

        std::cout << "established new connection " << std::endl;

        return true;
    }

    void GossipNode::handleDeleteNodeNotify(const std::string &name, const std::string &address, SlotVersion version,
                                            const ::pf_gossip::GossipNodeInfo *origin) {

        auto it = peers_.find(name);

        if (it == peers_.end()) {
            std::cerr << "Delete Not Exist Node" << std::endl;
            return;
        }

        if (it->second->version() > version) {
            std::cerr << "Delete Version Lower" << std::endl;
            return;
        }

        it->second->setNotAlive();

        // 对消息进行传播
        auto selected = randomSelectGossipNodes(origin->pass_nodes());
        for (auto &peer: selected) {
            peer->deleteNodeNotify(origin);    // 异步，不会进行阻塞
        }
    }

    std::string GossipNode::getMessage(const std::string &key) {

        auto[value, version] = local_slot_->find(key);

        if (version == 0)
            return {};

        std::cout << "get value: " << value << std::endl;

        return value;
    }

    std::string GossipNode::startConnection(const std::string &address) {

        auto peer = new GossipPeerNode("", address, 0, this);
        auto conn_res = peer->establishConnection();

        // 如果是两个 gossip 网络相互连接的话，那么发起节点也需要进行连接
        startNewNodeNotify(peer->name(), peer->address(), peer->version());

        if (conn_res.empty()) { // 如果成功
            auto ptr = peers_.emplace(peer->name(), peer).first->second;
            slots_.emplace(peer->name(), GossipSlot(peer->name()));
            peers_list_.emplace_back(ptr);
        }

        return conn_res;
    }

    void GossipNode::startHeartBeat() {

        auto cli = randomSelectGossipNode();

        if (cli == nullptr) {
            std::cout << "No available client to send heart beat" << std::endl;
            return;
        }

        cli->heartBeat();
    }

    void GossipNode::startDeleteNodeNotify() {
        // 对消息进行传播
        auto selected = randomSelectGossipNodes({});
        for (auto &peer: selected) {
            peer->deleteNodeNotify(name_, internal_address_, local_slot_->version());    // 异步，不会进行阻塞
        }
    }

    void GossipNode::startNewNodeNotify(const std::string &name, const std::string &address, SlotVersion version) {
        // 对消息进行传播
        auto selected = randomSelectGossipNodes({});
        for (auto &peer: selected) {
            peer->newNodeNotify(name, address, version);    // 异步，不会进行阻塞
        }
    }

    void GossipNode::DaemonTask() {
        time_t clock = time(nullptr) + 5;

        while (!quit) {

            auto now = time(nullptr);

            if (now >= clock) {
                checkPeerHealth();
                clock = now + 5;    // 定时一段时间来随机检查一次健康状况
            }

            doRetryTask();
            doGossipTask();
            removeUnHealthyNode();
            pullSlotToLowerNode();

            sleep(1);
        }

    }

    void GossipNode::removeUnHealthyNode() {

        std::list<GossipPeerNode *> peers;

        std::swap(unhealthy_peers_, peers);

        auto now = time(nullptr);

        // 大于 30s 没有通信且无应答的节点视为下线
        for (auto &peer: peers) {
            if (now - peer->timestamp() > 30) {
                if (!peer->echo()) {
                    peers_.erase(peer->name());
                }
            } else {
                unhealthy_peers_.emplace_back(peer);
            }
        }
    }

    void GossipNode::pullSlotToLowerNode() {
        std::lock_guard<std::mutex> lg(ls_mtx);
        if (lower_slots.empty())
            return;

        auto front = lower_slots.front();
        auto it = peers_.find(front);
        lower_slots.pop_front();

        if (it == peers_.end()) {
            std::cerr << "Pull Not Exist Slot." << std::endl;
            return;
        }

        pf_gossip::SlotUpdate slot;
        slot.set_name(local_slot_->name());
        slot.set_version(local_slot_->version());
        slot.add_pass_nodes(NodeName());
        it->second->pull(&slot);
    }

    void GossipNode::checkPeerHealth() {

        startHeartBeat();

        for (auto &[name, peer]: peers_) {
            if (!peer->isAlive())
                unhealthy_peers_.emplace_back(peer);
        }
    }

    void GossipNode::addRetryTask(GossipPeerNode::AsyncClientCall<google::protobuf::Any, google::protobuf::Any> *call) {
        std::lock_guard<std::mutex> lg(mtx);
        retry_list_.emplace_back(call);
    }

    void GossipNode::doRetryTask() {
        std::list<RetryCall *> calls;
        {
            std::lock_guard<std::mutex> lg(mtx);
            std::swap(calls, retry_list_);
        }

        for (auto &call: calls) {
            auto cli = randomSelectGossipNode();
            call->retry(cli);
        }

    }

    void GossipNode::doGossipTask() {

    }

    GossipPeerNode *GossipNode::randomSelectGossipNode() {

        if (peers_list_.empty())
            return {};

        std::random_device rd;
        std::default_random_engine e(rd());
        std::uniform_int_distribution<int> u(0, static_cast<int>(peers_.size()) - 1);

        int selected = u(e);

        if (peers_list_[selected]->isAlive())
            return peers_.at(peers_list_[selected]->name());

        return {};
    }

    std::list<GossipPeerNode *>
    GossipNode::randomSelectGossipNodes(const google::protobuf::RepeatedPtrField<std::string> &passed_nodes,
                                        size_t nums) {

        if (peers_list_.empty())
            return {};

        std::list<GossipPeerNode *> nodes;

        auto select_nums = std::min(nums, peers_list_.size());
        std::set<int> peer_no;

        std::random_device rd;
        std::default_random_engine e(rd());
        std::uniform_int_distribution<int> u(0, static_cast<int>(peers_list_.size()) - 1);

        while (peer_no.size() < select_nums) {

            int rand = u(e);

            auto find_node = [passed_nodes, this, rand] {
                if (passed_nodes.empty())
                    return false;
                for (auto &node: passed_nodes) {
                    if (node == peers_list_[rand]->name()) {
                        return true;
                    }
                }
                return false;
            };

            if (!find_node())
                peer_no.emplace(u(e));
        }

        for (auto rand: peer_no) {
            nodes.emplace_back(peers_.at(peers_list_[rand]->name()));
        }

        return nodes;
    }

    nlohmann::json GossipNode::getNodeStatus() {

        nlohmann::json json;

        json["name"] = name_;
        json["internal_address"] = internal_address_;
        json["external_address"] = external_address_;
        json["mem_use"] = mem_use_;
        for (auto &[name, peer]: peers_) {
            json["peers"].emplace_back(nlohmann::json{{"name",    name},
                                                      {"address", peer->address()},
                                                      {"alive",   peer->isAlive()}});
        }
        return json;
    }


}



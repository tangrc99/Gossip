

#include "protos/Gossip.pb.h"
#include "server/GossipNode.h"
#include "server/GossipPeerNode.h"

#include <grpcpp/grpcpp.h>
#include <iostream>


using namespace gossip;
using namespace gossip::server;


void runNode1() {

    // 创建一个服务实例
    // 名称: name, 唯一名称，用于区分消息的命名空间
    // 客户端服务监听地址: 127.0.0.1:9999, 客户端的访问地址
    // 节点通信接听地址: 127.0.0.1:8888, 节点之间的通信地址
    // 匹配密匙: secret, 非本地客户端需要匹配 secret 才能通信
    GossipNode node("name", "127.0.0.1:9999", "127.0.0.1:8888","secret");

    // 启动服务
    node.run();
}

void runNode2() {
    GossipNode node("2", "127.0.0.1:10000", "127.0.0.1:8889");

    std::cout << node.startConnection("127.0.0.1:8888");

    node.insertOrUpdateMessage("1", "11111111");

    node.run();
}


int main(int argc, char *argv[]) {

    if (argc == 1) {
        runNode1();

    } else {
        if (strcmp(argv[1], "1") == 0)
            runNode1();
        else
            runNode2();
    }
    return 0;

}

//
// Created by 唐仁初 on 2022/9/15.
//


#include "client/GossipClient.h"


int main() {

    // 连接到一个 gossip server, 必须匹配 token 才可以成功连接
    gossip::client::GossipClient client("127.0.0.1:9999", "1");
    // 控制当前 gossip server 连接到另外一个 gossip server
    client.connectToNode("127.0.0.1:8889");

    auto res = client.getNodeStatus();
    if(!res.isSucceed()){
        std::cerr << res.errorText() << std::endl;
    }else{
        auto status = res.value().dump(4);
        std::cout << status << std::endl;
    }



//    {
//        "external_address": "127.0.0.1:9999",
//                "internal_address": "127.0.0.1:8888",
//                "mem_use": 0,
//                "name": "1",
//                "peers": [
//        {
//            "address": "127.0.0.1:8889",
//                    "alive": true,
//                    "name": "2"
//        }
//        ]
//    }

    // 插入键值对
    auto insert_res = client.insertOrUpdateMessage("msg","content");
    if(!insert_res.isSucceed()){
        // handle error
    }

    // 查找键值对
    auto get_res = client.getMessage("msg");
    if(!get_res.isSucceed()){
        // handle error
    }else{
        std::cout << get_res.value() << std::endl;
    }

    // 删除键值对
    auto del_res = client.deleteMessage("msg");
    if(!del_res.isSucceed()){
        // handle error
    }

    // 关闭服务端
    client.shutdown();


    return 0;


}
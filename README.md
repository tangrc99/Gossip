# Gossip
Gossip 是 Gossip 协议的一个简单 C++ 实现，采用了 Gossip 协议的反熵模型，并且使用 Push 方法进行节点之间的通信。 项目基于 C++ 17 标准与 grpc 框架。

# 使用场景
项目的使用场景如下：
- 局域网内多用户数据共享，每个用户是数据生产者，并且数据的生命周期与用户的生命周期基本相同。
- 局域网内的用户数量不固定，可能会动态变化。
- 用户对其他用户的数据只具有读权限，而不具备写权限。
- 可能不具备长时间在线的主机
在这种使用场景下，使用传统的中心式注册服务是不合适的。又因为用户产生的数据与用户主机的存活状态一致，所以考虑使用 gossip 协议进行消息传递。

# 项目设计
## 消息存储结构
- 每一台主机需要使用一个唯一名称，并且任何一台主机都会使用该主机名称作为命名空间。不同命名空间的消息允许重复，即不同主机可以使用相同的键值进行消息传递。
- 内存表使用 命名空间 - 键值对 二级索引。

## gossip 协议
- 采用 push 方法进行节点通信
- 新节点上线、新消息产生时会产生gossip消息进行谣言传播
- 节点之间使用心跳机制，并在心跳中交换版本号
- 节点默认只搜寻本地内存表，如果需要最新消息则使用分布式搜索

## grpc 模型
- 客户端使用异步模型
- 服务端使用同步模式

# 项目使用

## 客户端连接与控制

```c++
    // 连接到一个 gossip server, 必须匹配 token 才可以成功连接
    gossip::client::GossipClient client("127.0.0.1:9999", "1");
    
    // 控制当前 gossip server 连接到另外一个 gossip server
    client.connectToNode("127.0.0.1:8889");
    
    // 状态信息
    auto res = client.getNodeStatus();
    if(!res.isSucceed()){
        std::cerr << res.errorText() << std::endl;
    }else{
        auto status = res.value().dump(4);
        std::cout << status << std::endl;
    }
 
    // 关闭服务端
    client.shutdown();
```

## 客户端 CRUD 操作

```c++
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
```

## 服务端启动

```c++
    // 创建一个服务实例
    // 名称: name, 唯一名称，用于区分消息的命名空间
    // 客户端服务监听地址: 127.0.0.1:9999, 客户端的访问地址
    // 节点通信接听地址: 127.0.0.1:8888, 节点之间的通信地址
    // 匹配密匙: secret, 非本地客户端需要匹配 secret 才能通信
    GossipNode node("name", "127.0.0.1:9999", "127.0.0.1:8888","secret");

    // 启动服务
    node.run();
```

# 备注

本项目原本是作为 TinySwarm 中主节点的同步，但由于后期引入 ETCD 作为注册中心，本项目仅作为备选方案。

本项目基本功能已经完成，但还有部分功能没有完成。
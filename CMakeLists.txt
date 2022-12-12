cmake_minimum_required(VERSION 3.20)
project(Gossip)

set(CMAKE_CXX_STANDARD 17)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

include_directories(${CMAKE_SOURCE_DIR}/protos)

aux_source_directory(protos PROTOS)

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(Gossip server.cpp
        server/GossipNode.cpp server/GossipNode.h
        server/GossipPeerNode.cpp server/GossipPeerNode.h
        server/GossipSlot.cpp server/GossipSlot.h
        server/services/GossipService.cpp server/services/GossipService.h
        server/services/GossipCliService.cpp server/services/GossipCliService.h

        ${PROTOS}
        )


add_executable(client client.cpp
        client/GossipClient.cpp client/GossipClient.h

        ${PROTOS}
        )

target_link_libraries(Gossip gRPC::grpc++ protobuf nlohmann_json)
target_link_libraries(client gRPC::grpc++ protobuf nlohmann_json)
#! /bin/bash

protoc --cpp_out=./ Gossip.proto
protoc --grpc_out=./ --plugin=protoc-gen-grpc="/usr/local/bin/grpc_cpp_plugin" Gossip.proto
protoc --cpp_out=./ GossipCli.proto
protoc --grpc_out=./ --plugin=protoc-gen-grpc="/usr/local/bin/grpc_cpp_plugin" GossipCli.proto

#include <iostream>
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <array>
#include <boost/asio.hpp>
#include <grpcpp/grpcpp.h>
#include "monitor.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using monitor::MonitorService;
using monitor::Empty;
using monitor::BoolResponse;
using monitor::UdpStats;

class MonitorServiceImpl final : public MonitorService::Service {
public:
    MonitorServiceImpl(std::atomic<bool>& ready, std::atomic<uint64_t>& pkts, std::atomic<uint64_t>& aBytes)
        : ready_(ready), pkts_(pkts), aBytes_(aBytes) {}

    Status IsReady(ServerContext*, const Empty*, BoolResponse* response) override {
        response->set_is_ready(ready_.load());
        return Status::OK;
    }

    Status GetUdpStatistics(ServerContext*, const Empty*, UdpStats* response) override {
        response->set_packets(pkts_.load());
        response->set_abytes(aBytes_.load());
        return Status::OK;
    }

private:
    std::atomic<bool>& ready_;
    std::atomic<uint64_t>& pkts_;
    std::atomic<uint64_t>& aBytes_;
};

class UdpListener {
public:
    UdpListener(boost::asio::io_context& io, uint16_t port,
                std::atomic<uint64_t>& pkts, std::atomic<uint64_t>& aBytes)
        : socket_(io, boost::asio::ip::udp::endpoint(
            boost::asio::ip::make_address("127.0.0.1"), port)),
          pkts_(pkts), aBytes_(aBytes) {
        buffer_.fill(0);
        start_receive();
    }

    void close() { socket_.close(); }

private:
    void start_receive() {
        socket_.async_receive_from(
            boost::asio::buffer(buffer_),
            remote_endpoint_,
            [this](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    pkts_++;
                    for (std::size_t i = 0; i < length; ++i) {
                        if (buffer_[i] == 'A') {
                            aBytes_++;
                        }
                    }
                    start_receive();
                }
            });
    }

    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint remote_endpoint_;
    std::atomic<uint64_t>& pkts_;
    std::atomic<uint64_t>& aBytes_;
    std::array<char, 1500> buffer_;
};

void RunServer(uint16_t udp_port, uint16_t grpc_port) {
    std::atomic<bool> ready(false);
    std::atomic<uint64_t> pkts(0);
    std::atomic<uint64_t> aBytes(0);

    boost::asio::io_context io;
    UdpListener listener(io, udp_port, pkts, aBytes);

    std::thread io_thread([&io]() { io.run(); });

    MonitorServiceImpl service(ready, pkts, aBytes);
    ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:" + std::to_string(grpc_port), grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << grpc_port << std::endl;

    ready = true;

    server->Wait();

    io.stop();
    io_thread.join();
    listener.close();
}

int main(int argc, char** argv) {
    uint16_t udp_port = 2032;
    uint16_t grpc_port = 2031;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--udp_port" && i + 1 < argc) {
            udp_port = std::stoi(argv[++i]);
        } else if (arg == "--grpc_port" && i + 1 < argc) {
            grpc_port = std::stoi(argv[++i]);
        }
    }

    RunServer(udp_port, grpc_port);
    return 0;
}

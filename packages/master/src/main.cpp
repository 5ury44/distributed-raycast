#include "master_server.h"
#include <iostream>
#include <signal.h>
#include <csignal>

namespace {
    std::unique_ptr<RaycastMaster::MasterServer> g_server;
    
    void SignalHandler(int signal) {
        std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
        if (g_server) {
            g_server->Stop();
        }
    }
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string address = "0.0.0.0";
    int port = 50052;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--address" && i + 1 < argc) {
            address = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --address <addr>    Server address (default: 0.0.0.0)\n"
                      << "  --port <port>       Server port (default: 50052)\n"
                      << "  --help              Show this help message\n";
            return 0;
        }
    }
    
    // Set up signal handlers
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    
    // Create and start server
    g_server = std::make_unique<RaycastMaster::MasterServer>(address, port);
    
    if (!g_server->Start()) {
        std::cerr << "Failed to start master server" << std::endl;
        return 1;
    }
    
    std::cout << "Master server started successfully on " 
              << g_server->GetAddress() << ":" << g_server->GetPort() << std::endl;
    
    // Wait for server to finish
    g_server->Wait();
    
    std::cout << "Master server shutdown complete" << std::endl;
    return 0;
}

#include "shared_memory_json.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

using json = nlohmann::json;
using namespace shared_memory;

// Service that both reads commands and writes status
class Service {
public:
    Service(const std::string& service_name, 
            const std::string& command_channel,
            const std::string& status_channel)
        : name_(service_name)
        , commands_(command_channel, 1024 * 1024, false)  // Open existing
        , status_(status_channel, 1024 * 1024, true)      // Create new
    {
        std::cout << "Service '" << name_ << "' started" << std::endl;
    }
    
    void run() {
        uint64_t last_cmd_seq = 0;
        int status_counter = 0;
        
        while (running_) {
            // Check for commands (non-blocking with short timeout)
            json command;
            if (commands_.readWithTimeout(command, 100, last_cmd_seq)) {
                last_cmd_seq = commands_.getSequenceNumber();
                processCommand(command);
            }
            
            // Publish status every second
            auto now = std::chrono::steady_clock::now();
            if (now - last_status_update_ >= std::chrono::seconds(1)) {
                publishStatus(status_counter++);
                last_status_update_ = now;
            }
        }
    }
    
    void stop() {
        running_ = false;
    }

private:
    std::string name_;
    SharedMemoryJSON commands_;
    SharedMemoryJSON status_;
    bool running_ = true;
    std::chrono::steady_clock::time_point last_status_update_ = 
        std::chrono::steady_clock::now();
    
    // Simulate some service state
    double temperature_ = 20.0;
    bool active_ = true;
    std::string mode_ = "auto";
    
    void processCommand(const json& cmd) {
        std::cout << "\n[" << name_ << "] Received command:" << std::endl;
        std::cout << cmd.dump(2) << std::endl;
        
        if (cmd.contains("action")) {
            std::string action = cmd["action"];
            
            if (action == "set_temperature" && cmd.contains("value")) {
                temperature_ = cmd["value"];
                std::cout << "→ Temperature set to " << temperature_ << "°C" << std::endl;
            }
            else if (action == "set_mode" && cmd.contains("mode")) {
                mode_ = cmd["mode"];
                std::cout << "→ Mode set to " << mode_ << std::endl;
            }
            else if (action == "toggle_active") {
                active_ = !active_;
                std::cout << "→ Active state: " << (active_ ? "ON" : "OFF") << std::endl;
            }
            else if (action == "shutdown") {
                std::cout << "→ Shutdown requested" << std::endl;
                running_ = false;
            }
        }
    }
    
    void publishStatus(int counter) {
        // Add some random variation to temperature
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(-0.5, 0.5);
        
        double current_temp = temperature_ + dis(gen);
        
        json status = {
            {"service", name_},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
            {"counter", counter},
            {"active", active_},
            {"mode", mode_},
            {"metrics", {
                {"temperature", current_temp},
                {"cpu_usage", 15.5 + dis(gen) * 10},
                {"memory_mb", 256 + static_cast<int>(dis(gen) * 50)}
            }},
            {"health", active_ ? "healthy" : "inactive"}
        };
        
        if (status_.write(status)) {
            std::cout << "[" << name_ << "] Status published (counter=" 
                      << counter << ")" << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <service_name>" << std::endl;
        std::cout << "Example: " << argv[0] << " Service1" << std::endl;
        return 1;
    }
    
    try {
        std::string service_name = argv[1];
        std::string command_channel = "commands";
        std::string status_channel = "status_" + service_name;
        
        Service service(service_name, command_channel, status_channel);
        
        std::cout << "\nListening for commands on: " << command_channel << std::endl;
        std::cout << "Publishing status to: " << status_channel << std::endl;
        std::cout << "\nPress Ctrl+C to stop.\n" << std::endl;
        
        service.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

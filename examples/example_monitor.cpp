#include "shared_memory_json.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <map>

using json = nlohmann::json;
using namespace shared_memory;

class Monitor {
public:
    void addService(const std::string& service_name) {
        try {
            std::string channel = "status_" + service_name;
            auto shm = std::make_shared<SharedMemoryJSON>(channel, 1024 * 1024, false);
            services_[service_name] = {shm, 0};
            std::cout << "✓ Monitoring service: " << service_name << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "✗ Failed to add service " << service_name 
                      << ": " << e.what() << std::endl;
        }
    }
    
    void monitor() {
        std::cout << "\n=== Service Monitor ===" << std::endl;
        std::cout << "Press Ctrl+C to stop.\n" << std::endl;
        
        while (true) {
            bool any_update = false;
            
            for (auto& [name, info] : services_) {
                json status;
                
                if (info.shm->readWithTimeout(status, 100, info.last_seq)) {
                    info.last_seq = info.shm->getSequenceNumber();
                    displayStatus(name, status);
                    any_update = true;
                }
            }
            
            if (!any_update) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
    
    void snapshot() {
        std::cout << "\n=== Current Status Snapshot ===" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        for (auto& [name, info] : services_) {
            json status;
            
            if (info.shm->read(status)) {
                displayStatus(name, status);
            } else {
                std::cout << "\n[" << name << "] No data available" << std::endl;
            }
        }
        
        std::cout << std::string(80, '=') << std::endl;
    }

private:
    struct ServiceInfo {
        std::shared_ptr<SharedMemoryJSON> shm;
        uint64_t last_seq;
    };
    
    std::map<std::string, ServiceInfo> services_;
    
    void displayStatus(const std::string& name, const json& status) {
        std::cout << "\n┌─ " << name << " " 
                  << std::string(76 - name.length(), '─') << "┐" << std::endl;
        
        if (status.contains("counter")) {
            std::cout << "│ Update #" << status["counter"] << std::endl;
        }
        
        if (status.contains("active")) {
            std::cout << "│ Status: " 
                      << (status["active"].get<bool>() ? "🟢 ACTIVE" : "🔴 INACTIVE") 
                      << std::endl;
        }
        
        if (status.contains("mode")) {
            std::cout << "│ Mode: " << status["mode"] << std::endl;
        }
        
        if (status.contains("health")) {
            std::cout << "│ Health: " << status["health"] << std::endl;
        }
        
        if (status.contains("metrics")) {
            const auto& metrics = status["metrics"];
            std::cout << "│ Metrics:" << std::endl;
            
            if (metrics.contains("temperature")) {
                std::cout << "│   Temperature: " 
                          << std::fixed << std::setprecision(2)
                          << metrics["temperature"].get<double>() << "°C" << std::endl;
            }
            
            if (metrics.contains("cpu_usage")) {
                std::cout << "│   CPU Usage: " 
                          << std::fixed << std::setprecision(1)
                          << metrics["cpu_usage"].get<double>() << "%" << std::endl;
            }
            
            if (metrics.contains("memory_mb")) {
                std::cout << "│   Memory: " 
                          << metrics["memory_mb"].get<int>() << " MB" << std::endl;
            }
        }
        
        std::cout << "└" << std::string(78, '─') << "┘" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <service1> [service2] [service3] ..." << std::endl;
        std::cout << "       " << argv[0] << " --snapshot <service1> [service2] ..." << std::endl;
        std::cout << "\nExample: " << argv[0] << " Service1 Service2" << std::endl;
        return 1;
    }
    
    try {
        Monitor monitor;
        
        bool snapshot_mode = false;
        int start_idx = 1;
        
        if (std::string(argv[1]) == "--snapshot") {
            snapshot_mode = true;
            start_idx = 2;
            
            if (argc < 3) {
                std::cerr << "Error: No services specified for snapshot mode" << std::endl;
                return 1;
            }
        }
        
        // Add all specified services
        for (int i = start_idx; i < argc; ++i) {
            monitor.addService(argv[i]);
        }
        
        if (snapshot_mode) {
            monitor.snapshot();
        } else {
            monitor.monitor();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

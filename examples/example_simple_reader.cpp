#include "shared_memory_json.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using json = nlohmann::json;
using namespace shared_memory;

int main() {
    try {
        // Open existing shared memory
        SharedMemoryJSON shm("my_shared_data", 1024 * 1024, false);
        
        std::cout << "Simple reader started. Polling every second..." << std::endl;
        std::cout << "Press Ctrl+C to stop." << std::endl << std::endl;
        
        while (true) {
            json data;
            
            // Simple read without timeout
            if (shm.read(data)) {
                std::cout << "✓ Current data:" << std::endl;
                std::cout << "  Counter: " << data.value("counter", -1) << std::endl;
                std::cout << "  Message: " << data.value("message", "N/A") << std::endl;
                
                // Safe access with default values
                if (data.contains("data") && data["data"].is_object()) {
                    std::cout << "  Temperature: " 
                              << data["data"].value("temperature", 0.0) << "°C" << std::endl;
                }
            } else {
                std::cout << "✗ Failed to read: " << shm.getLastError() << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

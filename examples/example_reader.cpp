#include "shared_memory_json.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using json = nlohmann::json;
using namespace shared_memory;

int main() {
    try {
        // Open existing shared memory (don't create)
        SharedMemoryJSON shm("my_shared_data", 1024 * 1024, false);
        
        std::cout << "Reader started. Reading JSON data..." << std::endl;
        std::cout << "Press Ctrl+C to stop." << std::endl << std::endl;
        
        uint64_t last_seq = 0;
        
        while (true) {
            json data;
            
            // Wait for new data with 5 second timeout
            if (shm.readWithTimeout(data, 5000, last_seq)) {
                last_seq = shm.getSequenceNumber();
                
                std::cout << "✓ Read new data (seq=" << last_seq << ")" << std::endl;
                std::cout << "  Counter: " << data["counter"] << std::endl;
                std::cout << "  Message: " << data["message"] << std::endl;
                std::cout << "  Temperature: " << data["data"]["temperature"] << "°C" << std::endl;
                std::cout << "  Humidity: " << data["data"]["humidity"] << "%" << std::endl;
                std::cout << "  Full JSON: " << data.dump(2) << std::endl << std::endl;
            } else {
                std::cout << "⏱ Timeout or error: " << shm.getLastError() << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

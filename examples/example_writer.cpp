#include "shared_memory_json.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using json = nlohmann::json;
using namespace shared_memory;

int main() {
    try {
        // Create shared memory with 1MB capacity
        SharedMemoryJSON shm("my_shared_data", 1024 * 1024, true);
        
        std::cout << "Writer started. Writing JSON data every 2 seconds..." << std::endl;
        std::cout << "Press Ctrl+C to stop." << std::endl << std::endl;
        
        int counter = 0;
        
        while (true) {
            // Create JSON data
            json data = {
                {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
                {"counter", counter},
                {"message", "Hello from writer"},
                {"data", {
                    {"temperature", 23.5 + (counter % 10)},
                    {"humidity", 45.0 + (counter % 20)},
                    {"pressure", 1013.25}
                }},
                {"array", {1, 2, 3, 4, 5}},
                {"nested", {
                    {"level1", {
                        {"level2", {
                            {"value", "deep value"}
                        }}
                    }}
                }}
            };
            
            // Write to shared memory
            if (shm.write(data)) {
                std::cout << "✓ Written data (counter=" << counter << ")" << std::endl;
                std::cout << "  JSON: " << data.dump(2) << std::endl << std::endl;
            } else {
                std::cerr << "✗ Failed to write: " << shm.getLastError() << std::endl;
            }
            
            counter++;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

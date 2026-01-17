#include "shared_memory_json.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

using json = nlohmann::json;
using namespace shared_memory;

class Controller {
public:
    Controller() : commands_("commands", 1024 * 1024, true) {
        std::cout << "Controller started. Publishing commands..." << std::endl;
    }
    
    void sendCommand(const json& cmd) {
        if (commands_.write(cmd)) {
            std::cout << "\n✓ Sent command:" << std::endl;
            std::cout << cmd.dump(2) << std::endl;
        } else {
            std::cerr << "✗ Failed to send command: " 
                      << commands_.getLastError() << std::endl;
        }
    }
    
    void interactiveMode() {
        std::cout << "\n=== Interactive Command Mode ===" << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  1 - Set temperature to 25°C" << std::endl;
        std::cout << "  2 - Set mode to 'manual'" << std::endl;
        std::cout << "  3 - Set mode to 'auto'" << std::endl;
        std::cout << "  4 - Toggle active state" << std::endl;
        std::cout << "  5 - Shutdown service" << std::endl;
        std::cout << "  6 - Custom JSON command" << std::endl;
        std::cout << "  q - Quit" << std::endl;
        std::cout << "\nEnter command: ";
        
        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "q" || input == "quit") {
                break;
            }
            
            json cmd;
            
            if (input == "1") {
                cmd = {
                    {"action", "set_temperature"},
                    {"value", 25.0},
                    {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
                };
            }
            else if (input == "2") {
                cmd = {
                    {"action", "set_mode"},
                    {"mode", "manual"},
                    {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
                };
            }
            else if (input == "3") {
                cmd = {
                    {"action", "set_mode"},
                    {"mode", "auto"},
                    {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
                };
            }
            else if (input == "4") {
                cmd = {
                    {"action", "toggle_active"},
                    {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
                };
            }
            else if (input == "5") {
                cmd = {
                    {"action", "shutdown"},
                    {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
                };
            }
            else if (input == "6") {
                std::cout << "Enter JSON command: ";
                std::string json_str;
                std::getline(std::cin, json_str);
                try {
                    cmd = json::parse(json_str);
                } catch (const std::exception& e) {
                    std::cerr << "Invalid JSON: " << e.what() << std::endl;
                    std::cout << "\nEnter command: ";
                    continue;
                }
            }
            else {
                std::cout << "Unknown command. Try again." << std::endl;
                std::cout << "\nEnter command: ";
                continue;
            }
            
            sendCommand(cmd);
            std::cout << "\nEnter command: ";
        }
    }
    
    void automatedDemo() {
        std::cout << "\n=== Automated Demo Mode ===" << std::endl;
        std::cout << "Sending commands every 3 seconds...\n" << std::endl;
        
        int counter = 0;
        
        while (true) {
            json cmd;
            
            switch (counter % 5) {
                case 0:
                    cmd = {
                        {"action", "set_temperature"},
                        {"value", 20.0 + counter},
                        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
                    };
                    break;
                case 1:
                    cmd = {
                        {"action", "set_mode"},
                        {"mode", "auto"},
                        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
                    };
                    break;
                case 2:
                    cmd = {
                        {"action", "toggle_active"},
                        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
                    };
                    break;
                case 3:
                    cmd = {
                        {"action", "set_mode"},
                        {"mode", "manual"},
                        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
                    };
                    break;
                case 4:
                    cmd = {
                        {"action", "toggle_active"},
                        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
                    };
                    break;
            }
            
            sendCommand(cmd);
            counter++;
            
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

private:
    SharedMemoryJSON commands_;
};

int main(int argc, char* argv[]) {
    try {
        Controller controller;
        
        if (argc > 1 && std::string(argv[1]) == "--demo") {
            controller.automatedDemo();
        } else {
            controller.interactiveMode();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

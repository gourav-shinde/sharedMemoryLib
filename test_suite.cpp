#include "shared_memory_json.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <vector>

using json = nlohmann::json;
using namespace shared_memory;

// ANSI color codes for output
#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

class TestRunner {
public:
    void run_all_tests() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Shared Memory JSON - Test Suite" << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        test_basic_write_read();
        test_sequence_numbers();
        test_timeout();
        test_large_json();
        test_nested_json();
        test_empty_data();
        test_overwrite();
        test_multiple_readers();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Test Summary" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << GREEN << "✓ Passed: " << passed_ << RESET << std::endl;
        std::cout << RED << "✗ Failed: " << failed_ << RESET << std::endl;
        std::cout << "  Total:  " << (passed_ + failed_) << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        if (failed_ > 0) {
            exit(1);
        }
    }

private:
    int passed_ = 0;
    int failed_ = 0;
    
    void assert_true(bool condition, const std::string& test_name) {
        if (condition) {
            std::cout << GREEN << "✓ " << test_name << RESET << std::endl;
            passed_++;
        } else {
            std::cout << RED << "✗ " << test_name << RESET << std::endl;
            failed_++;
        }
    }
    
    void test_basic_write_read() {
        std::cout << "\n[Test] Basic Write/Read" << std::endl;
        
        try {
            SharedMemoryJSON writer("test_basic", 1024 * 1024, true);
            SharedMemoryJSON reader("test_basic", 1024 * 1024, false);
            
            json write_data = {
                {"string", "hello"},
                {"number", 42},
                {"bool", true},
                {"null", nullptr}
            };
            
            assert_true(writer.write(write_data), "Write operation");
            
            json read_data;
            assert_true(reader.read(read_data), "Read operation");
            
            assert_true(read_data["string"] == "hello", "String value match");
            assert_true(read_data["number"] == 42, "Number value match");
            assert_true(read_data["bool"] == true, "Bool value match");
            assert_true(read_data["null"].is_null(), "Null value match");
            
        } catch (const std::exception& e) {
            assert_true(false, std::string("Exception: ") + e.what());
        }
    }
    
    void test_sequence_numbers() {
        std::cout << "\n[Test] Sequence Numbers" << std::endl;
        
        try {
            SharedMemoryJSON writer("test_seq", 1024 * 1024, true);
            SharedMemoryJSON reader("test_seq", 1024 * 1024, false);
            
            uint64_t seq1 = writer.getSequenceNumber();
            
            writer.write({{"counter", 1}});
            uint64_t seq2 = reader.getSequenceNumber();
            
            writer.write({{"counter", 2}});
            uint64_t seq3 = reader.getSequenceNumber();
            
            assert_true(seq2 > seq1, "Sequence increments on first write");
            assert_true(seq3 > seq2, "Sequence increments on second write");
            assert_true(seq3 == seq2 + 1, "Sequence increments by one");
            
        } catch (const std::exception& e) {
            assert_true(false, std::string("Exception: ") + e.what());
        }
    }
    
    void test_timeout() {
        std::cout << "\n[Test] Timeout Behavior" << std::endl;
        
        try {
            SharedMemoryJSON writer("test_timeout", 1024 * 1024, true);
            SharedMemoryJSON reader("test_timeout", 1024 * 1024, false);
            
            // Write initial data
            writer.write({{"value", 1}});
            uint64_t seq = reader.getSequenceNumber();
            
            // Try to read with same sequence (should timeout)
            json data;
            auto start = std::chrono::steady_clock::now();
            bool result = reader.readWithTimeout(data, 500, seq);
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            ).count();
            
            assert_true(!result, "Timeout occurs when no new data");
            assert_true(elapsed >= 450 && elapsed <= 600, "Timeout duration is approximately correct");
            
            // Write new data and verify it's received
            std::thread writer_thread([&writer]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                writer.write({{"value", 2}});
            });
            
            start = std::chrono::steady_clock::now();
            result = reader.readWithTimeout(data, 1000, seq);
            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            ).count();
            
            writer_thread.join();
            
            assert_true(result, "Read succeeds when new data arrives");
            assert_true(elapsed < 500, "Read returns quickly when data available");
            assert_true(data["value"] == 2, "Correct data received");
            
        } catch (const std::exception& e) {
            assert_true(false, std::string("Exception: ") + e.what());
        }
    }
    
    void test_large_json() {
        std::cout << "\n[Test] Large JSON Data" << std::endl;
        
        try {
            SharedMemoryJSON writer("test_large", 10 * 1024 * 1024, true);
            SharedMemoryJSON reader("test_large", 10 * 1024 * 1024, false);
            
            // Create large JSON with array of objects
            json large_data = {{"items", json::array()}};
            for (int i = 0; i < 1000; ++i) {
                large_data["items"].push_back({
                    {"id", i},
                    {"name", "Item " + std::to_string(i)},
                    {"value", i * 3.14},
                    {"tags", {"tag1", "tag2", "tag3"}}
                });
            }
            
            assert_true(writer.write(large_data), "Write large JSON");
            
            json read_data;
            assert_true(reader.read(read_data), "Read large JSON");
            
            assert_true(read_data["items"].size() == 1000, "Array size preserved");
            assert_true(read_data["items"][500]["id"] == 500, "Middle element correct");
            assert_true(read_data["items"][999]["name"] == "Item 999", "Last element correct");
            
        } catch (const std::exception& e) {
            assert_true(false, std::string("Exception: ") + e.what());
        }
    }
    
    void test_nested_json() {
        std::cout << "\n[Test] Nested JSON Structures" << std::endl;
        
        try {
            SharedMemoryJSON writer("test_nested", 1024 * 1024, true);
            SharedMemoryJSON reader("test_nested", 1024 * 1024, false);
            
            json nested = {
                {"level1", {
                    {"level2", {
                        {"level3", {
                            {"level4", {
                                {"deep_value", "found me!"}
                            }}
                        }}
                    }}
                }}
            };
            
            assert_true(writer.write(nested), "Write nested JSON");
            
            json read_data;
            assert_true(reader.read(read_data), "Read nested JSON");
            
            std::string deep = read_data["level1"]["level2"]["level3"]["level4"]["deep_value"];
            assert_true(deep == "found me!", "Deeply nested value preserved");
            
        } catch (const std::exception& e) {
            assert_true(false, std::string("Exception: ") + e.what());
        }
    }
    
    void test_empty_data() {
        std::cout << "\n[Test] Empty Data Handling" << std::endl;
        
        try {
            SharedMemoryJSON writer("test_empty", 1024 * 1024, true);
            SharedMemoryJSON reader("test_empty", 1024 * 1024, false);
            
            // Try to read before any write
            json data;
            bool result = reader.read(data);
            assert_true(!result, "Read fails when no data written");
            
            // Write empty object
            writer.write(json::object());
            result = reader.read(data);
            assert_true(result, "Read succeeds with empty object");
            assert_true(data.is_object() && data.empty(), "Empty object preserved");
            
            // Write empty array
            writer.write(json::array());
            result = reader.read(data);
            assert_true(result, "Read succeeds with empty array");
            assert_true(data.is_array() && data.empty(), "Empty array preserved");
            
        } catch (const std::exception& e) {
            assert_true(false, std::string("Exception: ") + e.what());
        }
    }
    
    void test_overwrite() {
        std::cout << "\n[Test] Data Overwrite" << std::endl;
        
        try {
            SharedMemoryJSON writer("test_overwrite", 1024 * 1024, true);
            SharedMemoryJSON reader("test_overwrite", 1024 * 1024, false);
            
            writer.write({{"version", 1}});
            writer.write({{"version", 2}});
            writer.write({{"version", 3}});
            
            json data;
            reader.read(data);
            
            assert_true(data["version"] == 3, "Latest write is preserved");
            
        } catch (const std::exception& e) {
            assert_true(false, std::string("Exception: ") + e.what());
        }
    }
    
    void test_multiple_readers() {
        std::cout << "\n[Test] Multiple Readers" << std::endl;
        
        try {
            SharedMemoryJSON writer("test_multi", 1024 * 1024, true);
            
            json test_data = {{"message", "hello from writer"}};
            writer.write(test_data);
            
            // Create multiple reader threads
            std::vector<std::thread> readers;
            std::vector<bool> results(5, false);
            
            for (int i = 0; i < 5; ++i) {
                readers.emplace_back([&results, i]() {
                    try {
                        SharedMemoryJSON reader("test_multi", 1024 * 1024, false);
                        json data;
                        if (reader.read(data)) {
                            results[i] = (data["message"] == "hello from writer");
                        }
                    } catch (...) {
                        results[i] = false;
                    }
                });
            }
            
            for (auto& t : readers) {
                t.join();
            }
            
            bool all_success = true;
            for (bool r : results) {
                all_success = all_success && r;
            }
            
            assert_true(all_success, "All readers can access data concurrently");
            
        } catch (const std::exception& e) {
            assert_true(false, std::string("Exception: ") + e.what());
        }
    }
};

int main() {
    TestRunner runner;
    runner.run_all_tests();
    return 0;
}

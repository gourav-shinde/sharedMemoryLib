#pragma once

#include <string>
#include <cstring>
#include <stdexcept>
#include <mutex>
#include <chrono>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#endif

namespace shared_memory {

using json = nlohmann::json;

// Shared memory region structure
struct SharedMemoryHeader {
    uint32_t magic_number;      // Validation magic number
    uint32_t version;           // Protocol version
    uint64_t data_size;         // Size of JSON data
    uint64_t sequence_number;   // Incremented on each write (wraps after ~584 years at 1B writes/sec)
    uint64_t timestamp;         // Last write timestamp (microseconds since epoch)
    char padding[32];           // Reserved for future use
};

constexpr uint32_t MAGIC_NUMBER = 0x534D4A53; // "SMJS" - Shared Memory JSON
constexpr uint32_t PROTOCOL_VERSION = 1;
constexpr size_t HEADER_SIZE = sizeof(SharedMemoryHeader);

class SharedMemoryJSON {
public:
    /**
     * Constructor
     * @param name Unique name for the shared memory region
     * @param max_size Maximum size for JSON data (excluding header)
     * @param create If true, create new shared memory; if false, open existing
     */
    SharedMemoryJSON(const std::string& name, size_t max_size, bool create = true)
        : name_(name)
        , max_data_size_(max_size)
        , total_size_(HEADER_SIZE + max_size)
        , is_creator_(create)
    {
#ifdef _WIN32
        initWindows(create);
#else
        initPosix(create);
#endif
    }

    ~SharedMemoryJSON() {
        cleanup();
    }

    // Prevent copying
    SharedMemoryJSON(const SharedMemoryJSON&) = delete;
    SharedMemoryJSON& operator=(const SharedMemoryJSON&) = delete;

    /**
     * Write JSON data to shared memory
     * @param data JSON object to write
     * @return true if successful, false otherwise
     */
    bool write(const json& data) {
        try {
            std::string serialized = data.dump();
            
            if (serialized.size() > max_data_size_) {
                throw std::runtime_error("JSON data too large for shared memory region");
            }

            lock();
            
            // Get header pointer
            SharedMemoryHeader* header = reinterpret_cast<SharedMemoryHeader*>(mapped_ptr_);
            
            // Update header
            header->magic_number = MAGIC_NUMBER;
            header->version = PROTOCOL_VERSION;
            header->data_size = serialized.size();
            header->sequence_number++;
            header->timestamp = getCurrentTimestamp();
            
            // Write JSON data after header
            char* data_ptr = reinterpret_cast<char*>(mapped_ptr_) + HEADER_SIZE;
            std::memcpy(data_ptr, serialized.c_str(), serialized.size());
            
            unlock();
            return true;
            
        } catch (const std::exception& e) {
            unlock();
            last_error_ = e.what();
            return false;
        }
    }

    /**
     * Read JSON data from shared memory
     * @param data Output parameter for JSON object
     * @return true if successful, false otherwise
     */
    bool read(json& data) {
        try {
            lock();
            
            // Get header pointer
            SharedMemoryHeader* header = reinterpret_cast<SharedMemoryHeader*>(mapped_ptr_);
            
            // Validate header
            if (header->magic_number != MAGIC_NUMBER) {
                unlock();
                last_error_ = "Invalid magic number - shared memory not initialized";
                return false;
            }
            
            if (header->version != PROTOCOL_VERSION) {
                unlock();
                last_error_ = "Protocol version mismatch";
                return false;
            }
            
            if (header->data_size == 0) {
                unlock();
                last_error_ = "No data in shared memory";
                return false;
            }
            
            // Read JSON data
            char* data_ptr = reinterpret_cast<char*>(mapped_ptr_) + HEADER_SIZE;
            std::string serialized(data_ptr, header->data_size);
            
            data = json::parse(serialized);
            
            unlock();
            return true;
            
        } catch (const std::exception& e) {
            unlock();
            last_error_ = e.what();
            return false;
        }
    }

    /**
     * Read with timeout - waits for new data (based on sequence number)
     * @param data Output parameter for JSON object
     * @param timeout_ms Timeout in milliseconds
     * @param last_seq Last sequence number seen (0 to read regardless)
     * @return true if new data read, false on timeout or error
     *
     * @note Uses sequence_number > last_seq comparison. On uint64_t overflow
     *       (after ~584 years at 1B writes/sec), one update may be missed.
     */
    bool readWithTimeout(json& data, uint64_t timeout_ms, uint64_t last_seq = 0) {
        auto start = std::chrono::steady_clock::now();
        
        while (true) {
            lock();
            SharedMemoryHeader* header = reinterpret_cast<SharedMemoryHeader*>(mapped_ptr_);
            
            if (header->magic_number == MAGIC_NUMBER && 
                header->data_size > 0 && 
                header->sequence_number > last_seq) {
                unlock();
                return read(data);
            }
            
            unlock();
            
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            ).count();
            
            if (elapsed >= timeout_ms) {
                last_error_ = "Timeout waiting for new data";
                return false;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    /**
     * Get the current sequence number without reading data
     */
    uint64_t getSequenceNumber() {
        lock();
        SharedMemoryHeader* header = reinterpret_cast<SharedMemoryHeader*>(mapped_ptr_);
        uint64_t seq = header->sequence_number;
        unlock();
        return seq;
    }

    /**
     * Get last error message
     */
    std::string getLastError() const {
        return last_error_;
    }

    /**
     * Get maximum data size
     */
    size_t getMaxDataSize() const {
        return max_data_size_;
    }

private:
    std::string name_;
    size_t max_data_size_;
    size_t total_size_;
    bool is_creator_;
    std::string last_error_;

#ifdef _WIN32
    HANDLE file_mapping_;
    HANDLE mutex_;
    void* mapped_ptr_;

    void initWindows(bool create) {
        std::string mutex_name = "Global\\mutex_" + name_;
        std::string shm_name = "Global\\" + name_;

        // Create or open mutex
        mutex_ = CreateMutexA(nullptr, FALSE, mutex_name.c_str());
        if (!mutex_) {
            throw std::runtime_error("Failed to create mutex");
        }

        if (create) {
            file_mapping_ = CreateFileMappingA(
                INVALID_HANDLE_VALUE,
                nullptr,
                PAGE_READWRITE,
                0,
                static_cast<DWORD>(total_size_),
                shm_name.c_str()
            );
        } else {
            file_mapping_ = OpenFileMappingA(
                FILE_MAP_ALL_ACCESS,
                FALSE,
                shm_name.c_str()
            );
        }

        if (!file_mapping_) {
            throw std::runtime_error("Failed to create/open file mapping");
        }

        mapped_ptr_ = MapViewOfFile(
            file_mapping_,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            total_size_
        );

        if (!mapped_ptr_) {
            CloseHandle(file_mapping_);
            throw std::runtime_error("Failed to map view of file");
        }

        if (create) {
            std::memset(mapped_ptr_, 0, total_size_);
        }
    }

    void lock() {
        WaitForSingleObject(mutex_, INFINITE);
    }

    void unlock() {
        ReleaseMutex(mutex_);
    }

    void cleanup() {
        if (mapped_ptr_) {
            UnmapViewOfFile(mapped_ptr_);
        }
        if (file_mapping_) {
            CloseHandle(file_mapping_);
        }
        if (mutex_) {
            CloseHandle(mutex_);
        }
    }

#else
    int shm_fd_;
    sem_t* sem_;
    void* mapped_ptr_;

    void initPosix(bool create) {
        std::string sem_name = "/sem_" + name_;
        std::string shm_name = "/" + name_;

        // Create or open semaphore
        if (create) {
            sem_unlink(sem_name.c_str()); // Clean up any existing semaphore
            sem_ = sem_open(sem_name.c_str(), O_CREAT | O_EXCL, 0666, 1);
        } else {
            sem_ = sem_open(sem_name.c_str(), 0);
        }

        if (sem_ == SEM_FAILED) {
            throw std::runtime_error("Failed to create/open semaphore: " + std::string(strerror(errno)));
        }

        // Create or open shared memory
        if (create) {
            shm_unlink(shm_name.c_str()); // Clean up any existing shared memory
            shm_fd_ = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
            if (shm_fd_ == -1) {
                sem_close(sem_);
                throw std::runtime_error("Failed to create shared memory: " + std::string(strerror(errno)));
            }

            if (ftruncate(shm_fd_, total_size_) == -1) {
                close(shm_fd_);
                sem_close(sem_);
                throw std::runtime_error("Failed to set shared memory size: " + std::string(strerror(errno)));
            }
        } else {
            shm_fd_ = shm_open(shm_name.c_str(), O_RDWR, 0666);
            if (shm_fd_ == -1) {
                sem_close(sem_);
                throw std::runtime_error("Failed to open shared memory: " + std::string(strerror(errno)));
            }
        }

        // Map shared memory
        mapped_ptr_ = mmap(nullptr, total_size_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
        if (mapped_ptr_ == MAP_FAILED) {
            close(shm_fd_);
            sem_close(sem_);
            throw std::runtime_error("Failed to map shared memory: " + std::string(strerror(errno)));
        }

        if (create) {
            std::memset(mapped_ptr_, 0, total_size_);
        }
    }

    void lock() {
        sem_wait(sem_);
    }

    void unlock() {
        sem_post(sem_);
    }

    void cleanup() {
        if (mapped_ptr_ != MAP_FAILED) {
            munmap(mapped_ptr_, total_size_);
        }
        if (shm_fd_ != -1) {
            close(shm_fd_);
            if (is_creator_) {
                std::string shm_name = "/" + name_;
                shm_unlink(shm_name.c_str());
            }
        }
        if (sem_ != SEM_FAILED) {
            sem_close(sem_);
            if (is_creator_) {
                std::string sem_name = "/sem_" + name_;
                sem_unlink(sem_name.c_str());
            }
        }
    }
#endif

    static uint64_t getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }
};

} // namespace shared_memory

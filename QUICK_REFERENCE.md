# Shared Memory JSON - Quick Reference

## Include & Namespace
```cpp
#include "shared_memory_json.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace shared_memory;
```

## Constructor
```cpp
// Create new shared memory
SharedMemoryJSON shm_writer("name", 1024*1024, true);

// Open existing shared memory
SharedMemoryJSON shm_reader("name", 1024*1024, false);
```

## Write Data
```cpp
json data = {
    {"key", "value"},
    {"number", 42},
    {"array", {1, 2, 3}},
    {"nested", {{"inner", "value"}}}
};

if (shm.write(data)) {
    // Success
} else {
    std::cerr << shm.getLastError() << std::endl;
}
```

## Read Data
```cpp
json data;
if (shm.read(data)) {
    std::cout << data["key"] << std::endl;
    int num = data["number"];
} else {
    std::cerr << shm.getLastError() << std::endl;
}
```

## Read with Timeout (Wait for New Data)
```cpp
uint64_t last_seq = 0;

while (true) {
    json data;
    if (shm.readWithTimeout(data, 5000, last_seq)) {
        last_seq = shm.getSequenceNumber();
        // Process new data
    } else {
        // Timeout or error
    }
}
```

## Get Sequence Number
```cpp
uint64_t seq = shm.getSequenceNumber();
```

## Check Max Size
```cpp
size_t max = shm.getMaxDataSize();
```

## Common Patterns

### Simple Publisher
```cpp
SharedMemoryJSON pub("channel", 1024*1024, true);
while (true) {
    json msg = {{"data", get_data()}};
    pub.write(msg);
    sleep(1);
}
```

### Simple Subscriber
```cpp
SharedMemoryJSON sub("channel", 1024*1024, false);
uint64_t last = 0;
while (true) {
    json msg;
    if (sub.readWithTimeout(msg, 1000, last)) {
        last = sub.getSequenceNumber();
        process(msg);
    }
}
```

## Build Commands

### CMake
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Make
```bash
make all
```

### Manual (Linux)
```bash
g++ -std=c++17 myapp.cpp -o myapp -lpthread -lrt
```

## Clean Up (Linux)
```bash
# Remove shared memory
rm /dev/shm/channel_name
rm /dev/shm/sem_channel_name

# Or use Make
make clean_shm
```

## Error Handling
```cpp
try {
    SharedMemoryJSON shm("name", size, create);
    // Use shm
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

## Memory Layout
```
[Header: 128 bytes]
  - Magic number (validation)
  - Version
  - Data size
  - Sequence number
  - Timestamp
  - Reserved
[JSON Data: up to max_size bytes]
```

## Platform Notes

**Linux:**
- Shared memory: `/dev/shm/`
- Semaphore: `/dev/shm/sem_*`
- Link: `-lpthread -lrt`

**macOS:**
- Link: `-lpthread`
- Auto cleanup on process exit

**Windows:**
- Uses Windows API
- Named mutexes
- Global namespace

## Limits
- Max JSON size: Specified in constructor
- Single value per shared memory region
- Last-write-wins (no queuing)

## Tips
✓ Writer creates (`create=true`)
✓ Readers open (`create=false`)
✓ Use sequence numbers to detect new data
✓ Check return values
✓ Handle exceptions
✓ Clean up shared memory between runs (development)

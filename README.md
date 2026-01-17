# Shared Memory JSON Library

A cross-platform C++17 library for inter-process communication using shared memory with nlohmann/json serialization.

## Features

- ✅ **Cross-platform**: Works on Linux, macOS, and Windows
- ✅ **JSON serialization**: Uses nlohmann/json for flexible data structures
- ✅ **Thread-safe**: Built-in mutex/semaphore synchronization
- ✅ **Header-only**: Easy to integrate into your projects
- ✅ **Versioning**: Sequence numbers and timestamps for data tracking
- ✅ **Timeout support**: Wait for new data with configurable timeouts
- ✅ **Validation**: Magic number and version checking

## Project Structure

```
sharedMemoryLib/
├── include/                      # Header-only library
│   └── shared_memory_json.hpp    # Main library header (copy this to use)
├── examples/                     # Example applications
│   ├── example_writer.cpp
│   ├── example_reader.cpp
│   ├── example_simple_reader.cpp
│   ├── example_service.cpp
│   ├── example_controller.cpp
│   └── example_monitor.cpp
├── tests/                        # Test suite
│   └── test_suite.cpp
├── docs/                         # Documentation
│   ├── FILE_OVERVIEW.md
│   ├── QUICK_REFERENCE.md
│   └── USAGE_GUIDE.md
├── third_party/                  # Third-party dependencies (auto-downloaded, gitignored)
│   └── nlohmann/json.hpp
├── CMakeLists.txt
├── Makefile
└── README.md
```

## Requirements

- C++17 compatible compiler
- CMake 3.10 or higher (optional)
- nlohmann/json (auto-downloaded by Makefile, or fetched by CMake)

### Platform-specific requirements:

**Linux/macOS:**
- POSIX shared memory support (`shm_open`, `mmap`)
- POSIX semaphores

**Windows:**
- Windows API (included by default)

## Building

### Using Makefile (recommended):

```bash
# Build all examples and tests (auto-downloads nlohmann/json to third_party/)
make all

# Run tests
make test

# Clean build artifacts
make clean

# Clean shared memory objects (Linux)
make clean_shm
```

### Using CMake:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Manual compilation (Linux):

```bash
# Download nlohmann/json header
mkdir -p third_party/nlohmann
curl -L https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp -o third_party/nlohmann/json.hpp

# Compile example
g++ -std=c++17 -Iinclude -Ithird_party examples/example_writer.cpp -o writer -lpthread -lrt
```

## Integration

To use this library in your project:

**Option 1: Copy header**
```bash
# Just copy the header file
cp include/shared_memory_json.hpp /your/project/include/
```

**Option 2: Git submodule**
```bash
git submodule add https://github.com/gourav-shinde/sharedMemoryLib.git libs/sharedMemoryLib
# Add -Ilibs/sharedMemoryLib/include to your compiler flags
```

**Option 3: CMake FetchContent**
```cmake
FetchContent_Declare(
    sharedMemoryLib
    GIT_REPOSITORY https://github.com/gourav-shinde/sharedMemoryLib.git
    GIT_TAG main
)
FetchContent_MakeAvailable(sharedMemoryLib)
target_link_libraries(your_target PRIVATE shared_memory_json)
```

**Note:** You need nlohmann/json available in your include path.

## Quick Start

### Writer Process:

```cpp
#include <shared_memory_json.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace shared_memory;

int main() {
    // Create shared memory (1MB capacity)
    SharedMemoryJSON shm("my_data", 1024 * 1024, true);
    
    // Create JSON data
    json data = {
        {"temperature", 23.5},
        {"humidity", 45.0},
        {"status", "active"},
        {"readings", {1, 2, 3, 4, 5}}
    };
    
    // Write to shared memory
    if (shm.write(data)) {
        std::cout << "Data written successfully!" << std::endl;
    } else {
        std::cerr << "Error: " << shm.getLastError() << std::endl;
    }
    
    return 0;
}
```

### Reader Process:

```cpp
#include <shared_memory_json.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace shared_memory;

int main() {
    // Open existing shared memory (don't create)
    SharedMemoryJSON shm("my_data", 1024 * 1024, false);
    
    json data;
    
    // Read from shared memory
    if (shm.read(data)) {
        std::cout << "Temperature: " << data["temperature"] << std::endl;
        std::cout << "Humidity: " << data["humidity"] << std::endl;
        std::cout << "Status: " << data["status"] << std::endl;
    } else {
        std::cerr << "Error: " << shm.getLastError() << std::endl;
    }
    
    return 0;
}
```

## API Reference

### Constructor

```cpp
SharedMemoryJSON(const std::string& name, size_t max_size, bool create = true)
```

- `name`: Unique identifier for the shared memory region
- `max_size`: Maximum size for JSON data (in bytes)
- `create`: If `true`, creates new shared memory; if `false`, opens existing

### Methods

#### `write(const json& data) -> bool`
Writes JSON data to shared memory.

```cpp
json data = {{"key", "value"}};
if (!shm.write(data)) {
    std::cerr << "Write failed: " << shm.getLastError() << std::endl;
}
```

#### `read(json& data) -> bool`
Reads JSON data from shared memory.

```cpp
json data;
if (shm.read(data)) {
    std::cout << data.dump(2) << std::endl;
}
```

#### `readWithTimeout(json& data, uint64_t timeout_ms, uint64_t last_seq = 0) -> bool`
Waits for new data based on sequence number.

```cpp
json data;
uint64_t last_seq = 0;

while (true) {
    if (shm.readWithTimeout(data, 5000, last_seq)) {
        last_seq = shm.getSequenceNumber();
        // Process new data
    } else {
        std::cout << "Timeout waiting for data" << std::endl;
    }
}
```

#### `getSequenceNumber() -> uint64_t`
Returns current sequence number without reading data.

```cpp
uint64_t seq = shm.getSequenceNumber();
```

#### `getLastError() -> std::string`
Returns the last error message.

```cpp
if (!shm.write(data)) {
    std::cout << "Error: " << shm.getLastError() << std::endl;
}
```

#### `getMaxDataSize() -> size_t`
Returns maximum data size in bytes.

```cpp
size_t max_size = shm.getMaxDataSize();
```

## Running Examples

### Terminal 1 (Writer):
```bash
./writer
```

Output:
```
Writer started. Writing JSON data every 2 seconds...
Press Ctrl+C to stop.

✓ Written data (counter=0)
  JSON: {
    "array": [1, 2, 3, 4, 5],
    "counter": 0,
    "data": {
      "humidity": 45.0,
      "pressure": 1013.25,
      "temperature": 23.5
    },
    ...
  }
```

### Terminal 2 (Reader):
```bash
./reader
```

Output:
```
Reader started. Reading JSON data...
Press Ctrl+C to stop.

✓ Read new data (seq=1)
  Counter: 0
  Message: Hello from writer
  Temperature: 23.5°C
  Humidity: 45.0%
  ...
```

## Architecture

### Memory Layout

```
┌─────────────────────────────────────┐
│      SharedMemoryHeader             │
│  - magic_number (validation)        │
│  - version (protocol version)       │
│  - data_size (JSON data size)       │
│  - sequence_number (increments)     │
│  - timestamp (microseconds)         │
│  - padding (reserved)               │
├─────────────────────────────────────┤
│                                     │
│      JSON Data (serialized)         │
│                                     │
│      (up to max_size bytes)         │
│                                     │
└─────────────────────────────────────┘
```

### Synchronization

**Linux/macOS:**
- Uses POSIX named semaphores (`sem_open`)
- Semaphore name: `/sem_{name}`
- Shared memory name: `/{name}`

**Windows:**
- Uses Windows named mutexes
- Mutex name: `Global\mutex_{name}`
- File mapping name: `Global\{name}`

## Best Practices

1. **Size Planning**: Choose `max_size` based on your largest expected JSON payload
2. **Error Handling**: Always check return values and handle errors
3. **Sequence Numbers**: Use `readWithTimeout()` to efficiently wait for new data
4. **Cleanup**: Objects automatically clean up on destruction
5. **Creator Pattern**: Have one process create (`create=true`) and others open (`create=false`)

## Thread Safety

The library is thread-safe for multi-process access. However, if multiple threads in the same process need to access the shared memory, you should add your own synchronization.

## Limitations

- Maximum JSON size must be specified at creation time
- Data is serialized to string format (not binary)
- Last-write-wins semantics (no conflict resolution)
- No built-in compression (add if needed for large payloads)

## Error Handling

Common errors and solutions:

| Error | Cause | Solution |
|-------|-------|----------|
| "Failed to create/open semaphore" | Permission issues or name collision | Use unique names, check permissions |
| "JSON data too large" | Data exceeds `max_size` | Increase `max_size` or reduce data |
| "Invalid magic number" | Shared memory not initialized | Ensure writer runs first |
| "Protocol version mismatch" | Different library versions | Use same library version |

## Performance Considerations

- **Serialization overhead**: JSON parsing adds CPU cost
- **Memory copying**: Data is copied during read/write
- **Lock contention**: Multiple processes compete for locks

For high-frequency updates (>1000 Hz), consider:
- Binary serialization instead of JSON
- Lock-free ring buffers
- Direct memory access patterns

## License

This library is provided as-is for educational and commercial use.

## Contributing

Contributions welcome! Areas for improvement:
- Binary serialization option
- Compression support
- Ring buffer for multiple messages
- Reader/writer versioning
- Performance benchmarks

## Troubleshooting

### Linux: "Permission denied"
```bash
# Check shared memory
ls -la /dev/shm/

# Clean up old shared memory
rm /dev/shm/my_data
rm /dev/shm/sem_my_data
```

### Windows: "Access denied"
Run applications with appropriate permissions or adjust security descriptors.

### macOS: Semaphore issues
macOS has restrictions on semaphore names. Keep names short and simple.

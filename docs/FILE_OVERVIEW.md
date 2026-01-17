# Shared Memory JSON Library - File Overview

## Core Library Files

### `shared_memory_json.hpp`
**The main library header (header-only library)**
- Cross-platform shared memory implementation
- JSON serialization using nlohmann/json
- Thread-safe with mutex/semaphore synchronization
- Support for Linux, macOS, and Windows
- Key features:
  - Write/read JSON data
  - Sequence number tracking
  - Timeout-based waiting for new data
  - Automatic cleanup on destruction

## Build Files

### `CMakeLists.txt`
**CMake build configuration**
- Automatically downloads nlohmann/json dependency
- Builds all examples and test suite
- Platform-independent build system
- Usage: `mkdir build && cd build && cmake .. && cmake --build .`

### `Makefile`
**GNU Make build configuration**
- Alternative to CMake for simpler builds
- Downloads nlohmann/json if needed
- Platform-specific flags (Linux/macOS)
- Usage: `make all` or `make test`

## Basic Examples

### `example_writer.cpp`
**Simple writer demonstration**
- Creates shared memory
- Writes JSON data every 2 seconds
- Shows counter, temperature, humidity data
- Good starting point for understanding the library

### `example_reader.cpp`
**Reader with timeout**
- Opens existing shared memory
- Uses `readWithTimeout()` to wait for new data
- Displays received data with formatting
- Demonstrates sequence number tracking

### `example_simple_reader.cpp`
**Polling reader**
- Simple polling every second
- Uses basic `read()` without timeout
- Good for applications that don't need real-time updates
- Shows safe JSON access with default values

## Advanced Examples

### `example_service.cpp`
**Bidirectional service**
- Reads commands from one shared memory channel
- Publishes status to another channel
- Simulates a service with state (temperature, mode, active status)
- Processes commands: set_temperature, set_mode, toggle_active, shutdown
- Usage: `./service ServiceName`

### `example_controller.cpp`
**Command controller**
- Sends commands to services
- Interactive mode: user selects commands from menu
- Demo mode: automated command sequence
- Publishes to "commands" channel
- Usage: `./controller` (interactive) or `./controller --demo` (automated)

### `example_monitor.cpp`
**Multi-service monitor**
- Monitors status from multiple services
- Real-time display of service metrics
- Snapshot mode for one-time status check
- Nice formatted output with boxes
- Usage: `./monitor Service1 Service2` or `./monitor --snapshot Service1`

## Testing

### `test_suite.cpp`
**Comprehensive test suite**
- Tests all major functionality
- Validates:
  - Basic read/write operations
  - Sequence number behavior
  - Timeout functionality
  - Large JSON handling
  - Nested JSON structures
  - Empty data handling
  - Data overwrite behavior
  - Multiple concurrent readers
- Color-coded output (green for pass, red for fail)
- Usage: `./test_suite` or `make test`

## Documentation

### `README.md`
**Main documentation**
- Overview of library features
- Requirements and building instructions
- Quick start guide
- Complete API reference
- Architecture explanation
- Best practices
- Troubleshooting guide
- Performance considerations

### `USAGE_GUIDE.md`
**Comprehensive usage tutorial**
- Detailed examples for all use cases
- Common design patterns:
  - Request-Response
  - Publisher-Subscriber
  - State Synchronization
  - Heartbeat Monitoring
- Performance optimization tips
- Troubleshooting with solutions
- Best practices checklist

### `QUICK_REFERENCE.md`
**Compact API reference card**
- Constructor syntax
- All methods with examples
- Common patterns
- Build commands
- Platform-specific notes
- Quick tips and limits

## Directory Structure

```
shared-memory-json/
├── shared_memory_json.hpp    # Main library header
├── CMakeLists.txt             # CMake build config
├── Makefile                   # Make build config
│
├── example_writer.cpp         # Basic writer
├── example_reader.cpp         # Reader with timeout
├── example_simple_reader.cpp  # Polling reader
│
├── example_service.cpp        # Advanced service
├── example_controller.cpp     # Command controller
├── example_monitor.cpp        # Service monitor
│
├── test_suite.cpp             # Test suite
│
├── README.md                  # Main documentation
├── USAGE_GUIDE.md             # Tutorial and patterns
├── QUICK_REFERENCE.md         # API quick reference
└── FILE_OVERVIEW.md           # This file
```

## Quick Start

### 1. Build Everything
```bash
# Using CMake
mkdir build && cd build
cmake ..
cmake --build .

# OR using Make
make all
```

### 2. Run Basic Example
```bash
# Terminal 1
./writer

# Terminal 2
./reader
```

### 3. Run Advanced Example
```bash
# Terminal 1: Start first service
./service Service1

# Terminal 2: Start second service
./service Service2

# Terminal 3: Monitor services
./monitor Service1 Service2

# Terminal 4: Control services
./controller
```

### 4. Run Tests
```bash
./test_suite
# OR
make test
```

## Integration into Your Project

### Header-Only Library
Simply copy `shared_memory_json.hpp` to your project:

```cpp
#include "shared_memory_json.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace shared_memory;

int main() {
    SharedMemoryJSON shm("my_data", 1024*1024, true);
    json data = {{"key", "value"}};
    shm.write(data);
    return 0;
}
```

### With CMake
Add to your `CMakeLists.txt`:

```cmake
# Fetch nlohmann_json
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Add shared_memory_json
add_library(shared_memory_json INTERFACE)
target_include_directories(shared_memory_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(shared_memory_json INTERFACE nlohmann_json::nlohmann_json)

if(UNIX AND NOT APPLE)
    target_link_libraries(shared_memory_json INTERFACE rt pthread)
endif()

# Your application
add_executable(myapp myapp.cpp)
target_link_libraries(myapp PRIVATE shared_memory_json)
```

## Platform-Specific Notes

### Linux
- Shared memory files: `/dev/shm/`
- Semaphores: `/dev/shm/sem_*`
- Clean up: `rm /dev/shm/your_channel_name*`
- Link flags: `-lpthread -lrt`

### macOS
- Automatic cleanup on process exit
- Link flags: `-lpthread`
- May have semaphore name restrictions (keep names short)

### Windows
- Uses Windows API automatically
- Named mutexes in Global namespace
- No special cleanup needed

## Common Use Cases

1. **IPC between microservices**: Use publisher-subscriber pattern
2. **Configuration sharing**: One writer, multiple readers
3. **Status monitoring**: Services publish status, monitor reads
4. **Command and control**: Controller sends commands, services respond
5. **Data pipeline**: Chain of processes passing data
6. **Heartbeat checking**: Services publish heartbeat, monitor checks

## Next Steps

1. Read `README.md` for overview
2. Try basic examples (`writer` + `reader`)
3. Run test suite to verify installation
4. Review `USAGE_GUIDE.md` for patterns
5. Study advanced examples for complex scenarios
6. Integrate into your project

## Support

For issues or questions:
1. Check `USAGE_GUIDE.md` troubleshooting section
2. Review example code for similar use cases
3. Run test suite to verify functionality
4. Check platform-specific notes above

## License

This library is provided as-is for educational and commercial use.

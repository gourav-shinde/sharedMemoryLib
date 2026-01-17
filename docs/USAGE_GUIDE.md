# Usage Guide - Shared Memory JSON Library

## Table of Contents

1. [Basic Usage](#basic-usage)
2. [Advanced Examples](#advanced-examples)
3. [Common Patterns](#common-patterns)
4. [Performance Tips](#performance-tips)
5. [Troubleshooting](#troubleshooting)

## Basic Usage

### Example 1: Simple Writer/Reader

**Step 1: Start the writer**
```bash
./writer
```

**Step 2: In another terminal, start the reader**
```bash
./reader
```

The reader will automatically receive updates as the writer publishes new data.

### Example 2: Polling Reader

For applications that don't need real-time updates:

```bash
./simple_reader
```

This polls the shared memory every second for the latest data.

## Advanced Examples

### Multi-Service Architecture

This demonstrates a complete microservices-style setup with:
- Multiple services publishing their status
- A controller sending commands
- A monitor observing all services

#### Setup

**Terminal 1: Start Service 1**
```bash
./service Service1
```

**Terminal 2: Start Service 2**
```bash
./service Service2
```

**Terminal 3: Start the Monitor**
```bash
./monitor Service1 Service2
```

**Terminal 4: Start the Controller**
```bash
# Interactive mode
./controller

# OR automated demo mode
./controller --demo
```

#### Interactive Controller Commands

When running `./controller`, you can send commands:

```
1 - Set temperature to 25°C
2 - Set mode to 'manual'
3 - Set mode to 'auto'
4 - Toggle active state
5 - Shutdown service
6 - Custom JSON command
q - Quit
```

#### Example Custom JSON Commands

When you select option 6, you can send custom JSON:

```json
{"action": "set_temperature", "value": 30.5}
```

```json
{"action": "set_mode", "mode": "eco", "level": 3}
```

```json
{"action": "custom_command", "params": {"key": "value"}}
```

#### Monitor Snapshot Mode

To get a one-time snapshot of all services:

```bash
./monitor --snapshot Service1 Service2
```

## Common Patterns

### Pattern 1: Request-Response

**Requester:**
```cpp
#include "shared_memory_json.hpp"

SharedMemoryJSON request_channel("requests", 1024 * 1024, true);
SharedMemoryJSON response_channel("responses", 1024 * 1024, false);

// Send request
json request = {{"id", 123}, {"query", "get_status"}};
request_channel.write(request);

// Wait for response
json response;
if (response_channel.readWithTimeout(response, 5000)) {
    std::cout << "Response: " << response.dump() << std::endl;
}
```

**Responder:**
```cpp
SharedMemoryJSON request_channel("requests", 1024 * 1024, false);
SharedMemoryJSON response_channel("responses", 1024 * 1024, true);

uint64_t last_seq = 0;
while (true) {
    json request;
    if (request_channel.readWithTimeout(request, 1000, last_seq)) {
        last_seq = request_channel.getSequenceNumber();
        
        // Process request
        json response = {{"id", request["id"]}, {"result", "OK"}};
        response_channel.write(response);
    }
}
```

### Pattern 2: Publisher-Subscriber

**Publisher:**
```cpp
SharedMemoryJSON events("events", 1024 * 1024, true);

while (true) {
    json event = {
        {"type", "sensor_reading"},
        {"value", read_sensor()},
        {"timestamp", get_timestamp()}
    };
    events.write(event);
    sleep(1);
}
```

**Subscriber 1:**
```cpp
SharedMemoryJSON events("events", 1024 * 1024, false);

uint64_t last_seq = 0;
while (true) {
    json event;
    if (events.readWithTimeout(event, 5000, last_seq)) {
        last_seq = events.getSequenceNumber();
        process_event(event);
    }
}
```

**Subscriber 2:**
```cpp
// Same code as Subscriber 1
// Multiple subscribers can independently read the same data
```

### Pattern 3: State Synchronization

**Primary Service:**
```cpp
SharedMemoryJSON state("app_state", 1024 * 1024, true);

while (true) {
    // Update application state
    app_state.counter++;
    app_state.last_update = time(nullptr);
    
    // Serialize to JSON
    json state_json = {
        {"counter", app_state.counter},
        {"last_update", app_state.last_update},
        {"config", app_state.config}
    };
    
    state.write(state_json);
    sleep(1);
}
```

**Secondary Services:**
```cpp
SharedMemoryJSON state("app_state", 1024 * 1024, false);

json current_state;
if (state.read(current_state)) {
    // Sync local state
    local_counter = current_state["counter"];
    local_config = current_state["config"];
}
```

### Pattern 4: Heartbeat Monitoring

**Service:**
```cpp
SharedMemoryJSON heartbeat("service_heartbeat", 1024 * 1024, true);

while (running) {
    json hb = {
        {"service", "MyService"},
        {"timestamp", get_timestamp()},
        {"status", "alive"},
        {"uptime", get_uptime()}
    };
    heartbeat.write(hb);
    sleep(5); // Heartbeat every 5 seconds
}
```

**Monitor:**
```cpp
SharedMemoryJSON heartbeat("service_heartbeat", 1024 * 1024, false);

while (true) {
    json hb;
    if (heartbeat.read(hb)) {
        uint64_t last_seen = hb["timestamp"];
        uint64_t now = get_timestamp();
        
        if (now - last_seen > 15'000'000) { // 15 seconds in microseconds
            alert("Service not responding!");
        }
    }
    sleep(5);
}
```

## Performance Tips

### 1. Choose Appropriate Buffer Sizes

```cpp
// For small messages (<1KB)
SharedMemoryJSON shm("small", 10 * 1024, true); // 10KB

// For medium messages (1-100KB)
SharedMemoryJSON shm("medium", 1024 * 1024, true); // 1MB

// For large messages (>100KB)
SharedMemoryJSON shm("large", 10 * 1024 * 1024, true); // 10MB
```

### 2. Minimize JSON Serialization Overhead

**Bad:**
```cpp
// Re-serializing on every loop iteration
while (true) {
    json data = build_large_json(); // Expensive
    shm.write(data);
    sleep(1);
}
```

**Good:**
```cpp
// Only update what changes
json data = build_initial_json();
while (true) {
    data["timestamp"] = get_timestamp();
    data["counter"]++;
    shm.write(data);
    sleep(1);
}
```

### 3. Use Sequence Numbers Efficiently

**Bad:**
```cpp
// Polling without sequence tracking
while (true) {
    json data;
    shm.read(data);
    process(data); // Might process same data multiple times
    sleep(1);
}
```

**Good:**
```cpp
// Track sequence to process only new data
uint64_t last_seq = 0;
while (true) {
    json data;
    if (shm.readWithTimeout(data, 1000, last_seq)) {
        last_seq = shm.getSequenceNumber();
        process(data); // Only new data
    }
}
```

### 4. Batch Updates When Possible

**Bad:**
```cpp
// Multiple small writes
shm.write({{"temp", 23.5}});
shm.write({{"humidity", 45.0}});
shm.write({{"pressure", 1013.25}});
```

**Good:**
```cpp
// Single batched write
json data = {
    {"temp", 23.5},
    {"humidity", 45.0},
    {"pressure", 1013.25}
};
shm.write(data);
```

## Troubleshooting

### Issue: "Failed to create/open shared memory"

**Cause:** Permission issues or leftover shared memory from crashed process.

**Solution (Linux):**
```bash
# List shared memory objects
ls -la /dev/shm/

# Remove specific shared memory
rm /dev/shm/my_shared_data
rm /dev/shm/sem_my_shared_data

# Or remove all
rm /dev/shm/*
```

**Solution (macOS):**
```bash
# Shared memory objects are automatically cleaned up on macOS
# If issues persist, reboot
```

**Solution (Windows):**
- Restart the application
- If persistent, reboot the system

### Issue: "JSON data too large for shared memory region"

**Cause:** JSON payload exceeds the `max_size` specified in constructor.

**Solution:**
```cpp
// Increase max_size
SharedMemoryJSON shm("data", 10 * 1024 * 1024, true); // 10MB instead of 1MB
```

Or compress/reduce your JSON:
```cpp
// Instead of storing large arrays
json bad = {{"data", vector_of_10000_items}};

// Paginate or summarize
json good = {
    {"summary", compute_summary(data)},
    {"count", data.size()}
};
```

### Issue: Reader doesn't see writer's data

**Checklist:**
1. ✓ Writer must run first (with `create=true`)
2. ✓ Both use the same name string
3. ✓ Both use compatible buffer sizes
4. ✓ Writer actually calls `write()`

**Debug:**
```cpp
// In reader, check if shared memory is initialized
json data;
if (!shm.read(data)) {
    std::cout << "Error: " << shm.getLastError() << std::endl;
    // Will show "Invalid magic number" if writer hasn't written yet
}
```

### Issue: High CPU usage

**Cause:** Tight polling loop without sleep.

**Bad:**
```cpp
while (true) {
    json data;
    shm.read(data); // No delay - 100% CPU
}
```

**Good:**
```cpp
while (true) {
    json data;
    if (shm.readWithTimeout(data, 1000, last_seq)) {
        // Process new data
    }
    // readWithTimeout includes built-in sleep
}
```

### Issue: Data corruption or crashes

**Cause:** Buffer overflow or concurrent access issues.

**Solutions:**

1. Check JSON size:
```cpp
std::string serialized = data.dump();
std::cout << "JSON size: " << serialized.size() << " bytes" << std::endl;
std::cout << "Buffer size: " << shm.getMaxDataSize() << " bytes" << std::endl;
```

2. Validate data before writing:
```cpp
try {
    std::string test = data.dump();
    if (test.size() > shm.getMaxDataSize()) {
        std::cerr << "Data too large!" << std::endl;
        return;
    }
    shm.write(data);
} catch (const std::exception& e) {
    std::cerr << "Serialization error: " << e.what() << std::endl;
}
```

### Issue: Deadlock

**Cause:** Process crashed while holding lock.

**Solution (Linux):**
```bash
# Force remove semaphore
rm /dev/shm/sem_my_shared_data
```

**Prevention:**
```cpp
// Use try-catch to ensure cleanup
try {
    SharedMemoryJSON shm("data", 1024 * 1024, true);
    // ... your code ...
} catch (...) {
    // Destructor automatically called, releases lock
    throw;
}
```

## Best Practices Checklist

- [ ] Choose appropriate buffer size (not too small, not wastefully large)
- [ ] Use sequence numbers to track new data
- [ ] Handle errors and check return values
- [ ] Use try-catch for exception safety
- [ ] Clean up shared memory on development machine between runs
- [ ] Document the JSON schema your services use
- [ ] Version your JSON protocol if it may change
- [ ] Add timestamps to track data freshness
- [ ] Use meaningful names for shared memory regions
- [ ] Test with multiple readers/writers
- [ ] Monitor memory usage in production
- [ ] Implement timeout handling
- [ ] Add logging for debugging
- [ ] Consider data validation before writing

## Next Steps

1. Review the example code in `example_*.cpp`
2. Build and run the basic examples
3. Experiment with the advanced multi-service examples
4. Design your own JSON schemas for your use case
5. Implement error handling for production use
6. Add monitoring and logging as needed

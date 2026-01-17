# Makefile for Shared Memory JSON Library

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude -Ithird_party

# Platform detection
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
    LDFLAGS = -lpthread -lrt
endif
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -lpthread
endif

# Targets
TARGETS = writer reader simple_reader service controller monitor test_suite

all: $(TARGETS)

# Check for nlohmann/json (third-party dependency)
check_json:
	@if [ ! -f third_party/nlohmann/json.hpp ]; then \
		echo "Downloading nlohmann/json header..."; \
		mkdir -p third_party/nlohmann; \
		curl -L https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp -o third_party/nlohmann/json.hpp; \
	fi

# Build targets
writer: check_json examples/example_writer.cpp include/shared_memory_json.hpp
	$(CXX) $(CXXFLAGS) examples/example_writer.cpp -o writer $(LDFLAGS)

reader: check_json examples/example_reader.cpp include/shared_memory_json.hpp
	$(CXX) $(CXXFLAGS) examples/example_reader.cpp -o reader $(LDFLAGS)

simple_reader: check_json examples/example_simple_reader.cpp include/shared_memory_json.hpp
	$(CXX) $(CXXFLAGS) examples/example_simple_reader.cpp -o simple_reader $(LDFLAGS)

service: check_json examples/example_service.cpp include/shared_memory_json.hpp
	$(CXX) $(CXXFLAGS) examples/example_service.cpp -o service $(LDFLAGS)

controller: check_json examples/example_controller.cpp include/shared_memory_json.hpp
	$(CXX) $(CXXFLAGS) examples/example_controller.cpp -o controller $(LDFLAGS)

monitor: check_json examples/example_monitor.cpp include/shared_memory_json.hpp
	$(CXX) $(CXXFLAGS) examples/example_monitor.cpp -o monitor $(LDFLAGS)

test_suite: check_json tests/test_suite.cpp include/shared_memory_json.hpp
	$(CXX) $(CXXFLAGS) tests/test_suite.cpp -o test_suite $(LDFLAGS)

# Clean
clean:
	rm -f $(TARGETS)
	rm -f *.o

# Clean shared memory (Linux only)
clean_shm:
	@echo "Cleaning shared memory objects..."
	@rm -f /dev/shm/my_shared_data /dev/shm/sem_my_shared_data
	@rm -f /dev/shm/commands /dev/shm/sem_commands
	@rm -f /dev/shm/status_* /dev/shm/sem_status_*
	@echo "Done."

# Help
help:
	@echo "Available targets:"
	@echo "  all          - Build all examples"
	@echo "  writer       - Build basic writer example"
	@echo "  reader       - Build basic reader example"
	@echo "  simple_reader - Build simple polling reader"
	@echo "  service      - Build service example"
	@echo "  controller   - Build controller example"
	@echo "  monitor      - Build monitor example"
	@echo "  test_suite   - Build test suite"
	@echo "  test         - Build and run tests"
	@echo "  clean        - Remove built binaries"
	@echo "  clean_shm    - Clean shared memory objects (Linux only)"
	@echo "  help         - Show this help message"

# Run tests
test: test_suite
	@echo "Running test suite..."
	@./test_suite

.PHONY: all clean clean_shm help check_json

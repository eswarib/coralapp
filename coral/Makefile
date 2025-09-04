# Unit test sources
TEST_DIR := test
TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp, $(BIN_DIR)/%.test.o, $(TEST_SRCS))
TEST_BINS := $(patsubst $(TEST_DIR)/%.cpp, $(BIN_DIR)/%, $(TEST_SRCS))

# Build all tests
.PHONY: tests

alltests: $(TEST_BINS)

$(BIN_DIR)/%: $(TEST_DIR)/%.cpp $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

# Run all tests
.PHONY: runtests
runtests: alltests
	@for test in $(TEST_BINS); do echo "Running $$test"; $$test; done

# Source files and objects
SRC_DIR := src
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BIN_DIR)/%.o, $(SRCS))

# Add TextInjector.cpp and Recorder.cpp explicitly if not already included
SRCS += $(SRC_DIR)/TextInjector.cpp
OBJS += $(BIN_DIR)/TextInjector.o
SRCS += $(SRC_DIR)/Recorder.cpp
OBJS += $(BIN_DIR)/Recorder.o 

.PHONY: all backend frontend clean

all: backend frontend

# Invoke the backend Makefile in src/
backend:
	$(MAKE) -C src

# Build Electron frontend
frontend:
	cd ../coral-electron && npm ci
	# Uncomment if you have a build step:
	# cd ../coral-electron && npm run build

clean:
	$(MAKE) -C src clean
	cd ../coral-electron && rm -rf node_modules

# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -lX11

# xdo static library and include
XDO_LIB := ../../libxdo/libxdo.a
XDO_INC := -I../../libxdo

# Include directories
INCLUDES := -I/usr/local/include \
            -I/usr/include \
            -I../../whispercpp/include \
            -I../../whispercpp/ggml/include \
            $(XDO_INC)

# Library directories
LIBDIRS := -L/usr/local/lib -L../lib

# Libraries to link
LIBS:= -lwhisper -lpthread -lm -lportaudio -lsndfile

# Paths
SRC_DIR := .
BIN_DIR := ../bin
TARGET := $(BIN_DIR)/coral

# Source and object files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BIN_DIR)/%.o, $(SRCS))

RPATH := /usr/local/lib:../lib

# Default target
all: $(TARGET)

# Link object files into final binary
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
		$(CXX) $(CXXFLAGS) $(LIBDIRS) -Wl,-rpath,$(RPATH) -o $@ $^ $(XDO_LIB) -lXinerama -lXtst -lX11 -lxkbcommon $(LIBS)

# Compile source files to object files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean
clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean


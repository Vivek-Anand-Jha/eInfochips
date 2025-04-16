# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -fPIC -Iinclude -I/usr/local/include/azureiot -I/home/vivekjha/Project
LDFLAGS = -L/usr/local/lib -L/home/vivekjha/Project
LIBS = -liothub_client -liothub_client_mqtt_transport -lumqtt -laziotsharedutil \
       -lssl -lcrypto -lpthread -lparson -lcurl -lazureDemo

# Directories
SRC_DIR = src
INCLUDE_DIR = include
MAIN_DIR = main
BIN_DIR = bin
EXEC_DIR = executable

# Source files
SRC_FILES = $(SRC_DIR)/azureDemo.cpp
MAIN_FILE = $(MAIN_DIR)/callLibrary.cpp

# Object files (mapped to bin/)
OBJ = $(BIN_DIR)/azureDemo.o $(BIN_DIR)/callLibrary.o

# Targets
STATIC_LIB = libazureDemo.a
SHARED_LIB = libazureDemo.so
EXEC = callLibrary

# Default target
all: $(EXEC_DIR)/$(EXEC)

# Create necessary directories
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(EXEC_DIR):
	mkdir -p $(EXEC_DIR)

# Compile source files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN_DIR)/%.o: $(MAIN_DIR)/%.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build executable
$(EXEC_DIR)/$(EXEC): $(OBJ) | $(EXEC_DIR)
	$(CXX) -o $@ $(OBJ) $(LDFLAGS) $(LIBS)
	@echo "Built executable: $@"

# Build libraries
lib: $(BIN_DIR)/azureDemo.o
	$(CXX) -shared -o $(SHARED_LIB) $< $(LDFLAGS) $(LIBS)
	ar rcs $(STATIC_LIB) $<
	@echo "Built shared library: $(SHARED_LIB)"
	@echo "Built static library: $(STATIC_LIB)"

# Clean
clean:
	rm -rf $(BIN_DIR) $(EXEC_DIR) $(STATIC_LIB) $(SHARED_LIB)
	@echo "Cleaned all build"


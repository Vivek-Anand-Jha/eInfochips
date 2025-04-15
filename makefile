# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -fPIC -I. -I/usr/local/include/azureiot
LDFLAGS = -L/usr/local/lib
LIBS = -liothub_client -liothub_client_mqtt_transport -liothub_client_mqtt_ws_transport \
       -lprov_mqtt_transport -lprov_mqtt_ws_transport -lparson -luamqp -lprov_auth_client \
       -lhsm_security_client -lserializer -lumqtt -laziotsharedutil -lcurl -lssl -lcrypto -pthread

# Source and object
SRC = azureDemo.cpp
OBJ = azureDemo.o

# Targets
STATIC_LIB = libazureDemo.a
SHARED_LIB = libazureDemo.so

# Default: build shared library
all: shared

# Build object file
$(OBJ): $(SRC) azureDemo.hpp
	$(CXX) $(CXXFLAGS) -c $(SRC) -o $(OBJ)

# Build shared library
shared: $(OBJ)
	$(CXX) -shared -o $(SHARED_LIB) $(OBJ) $(LDFLAGS) $(LIBS)
	@echo "Built shared library: $(SHARED_LIB)"

# Build static library
static: $(OBJ)
	ar rcs $(STATIC_LIB) $(OBJ)
	@echo "Built static library: $(STATIC_LIB)"

# Clean
clean:
	rm -f $(OBJ) #$(STATIC_LIB) $(SHARED_LIB)
	@echo "Cleaned up build files"


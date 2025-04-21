/*
 * Author:Vivekanand Jha
 * Created:18/03/2025
 * Modified:26/03/2025
 * Azure Task By Mentor
 */

#include "azureDemo.hpp"

std::atomic<bool> running(true);
std::queue<int> taskQueue;
std::mutex queueMutex;
std::condition_variable cv;

/**
 * initAzureClient()
 * Initialize the Azure IoT Hub client.
 */
IOTHUB_DEVICE_CLIENT_LL_HANDLE initAzureClient(const char* connectionString) {
    IoTHub_Init();  // Ensure SDK initialization

    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
    if (device_handle == nullptr) {
        std::cerr << "Failed to create IoT Hub client handle." << std::endl;
        IoTHub_Deinit();
        return nullptr;
    }

    const char* cert_path = "/home/vivekjha/Project/BaltimoreCyberTrustRoot.crt.pem";
    IoTHubDeviceClient_LL_SetOption(device_handle, "TrustedCerts", cert_path);

    return device_handle;
}
/**
 * messageReceivedCallback()
 * Triggered when a message from the cloud arrives.
 */
IOTHUBMESSAGE_DISPOSITION_RESULT messageReceivedCallback(IOTHUB_MESSAGE_HANDLE message, void* userContext) {
    if (message == nullptr) {
        std::cerr << "Message handle is NULL." << std::endl;
        return IOTHUBMESSAGE_REJECTED;
    }

    const char* receivedMessage = IoTHubMessage_GetString(message);
    if (receivedMessage != nullptr) {
        std::cout << "Received message from Azure IoT Hub: " << receivedMessage << std::endl;
    } else {
        std::cerr << "Failed to retrieve message string." << std::endl;
    }

    IoTHubMessage_Destroy(message);
    return IOTHUBMESSAGE_ACCEPTED;
}
/**
 * deviceToCloud()
 * Sends a message to the cloud via Azure IoT Hub.
 */
void deviceToCloud(IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle) {
    const char* message = "{Hello from Vivek to Azure}";
    IOTHUB_MESSAGE_HANDLE message_handle = IoTHubMessage_CreateFromString(message);

    if (message_handle == nullptr) {
        std::cerr << "Failed to create message." << std::endl;
    } else {
        if (IoTHubDeviceClient_LL_SendEventAsync(device_handle, message_handle, nullptr, nullptr) != IOTHUB_CLIENT_OK) {
            std::cerr << "Failed to send message to Azure IoT Hub." << std::endl;
        } else {
            std::cout << "Message sent " << std::endl;
        }
        IoTHubMessage_Destroy(message_handle);
    }
}
/**
 * processQueue()
 * Processes the task queue and sends messages to the cloud.
 */
void processQueue(IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle) {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [] { return !taskQueue.empty() || !running; });

        while (!taskQueue.empty()) {
            //int task = taskQueue.front();
            taskQueue.pop();
            lock.unlock();

            //std::cout << "Processing task " << task << std::endl;
            deviceToCloud(device_handle);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            lock.lock();
        }
    }
}
/**
 * tcpSender()
 * Sends tasks to a client via a TCP socket.
 */
void tcpSender(int port) {
    int server_fd, client_fd;
    struct sockaddr_in address {};
    int opt = 1;
    int addrlen = sizeof(address);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed!" << std::endl;
        return;
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options!" << std::endl;
        close(server_fd);
        return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port); 

    // Bind the socket to an IP/port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed!" << std::endl;
        close(server_fd);
        return;
    }
    // Start listening for incoming connections
    if (listen(server_fd, 1) < 0) {
        std::cerr << "Listen failed!" << std::endl;
        close(server_fd);
        return;
    }
    std::cout << "Waiting for client on port " << port << "..." << std::endl;
    // Accept a connection
    client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (client_fd < 0) {
        std::cerr << "Accept failed!" << std::endl;
        close(server_fd);
        return;
    }
    std::cout << "Client connected!" << std::endl;
    // Sending data to the client
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [] { return !taskQueue.empty() || !running; });

        if (!running && taskQueue.empty()) break;

        int task = taskQueue.front();
        taskQueue.pop();
        lock.unlock();

        std::string message = "Task from device: " + std::to_string(task) + "\n";
        if (send(client_fd, message.c_str(), message.length(), 0) < 0) {
            std::cerr << "Failed to send data!" << std::endl;
        } else {
            std::cout << "Sent: " << message;
        }
    }
    close(client_fd);
    close(server_fd);
    std::cout << "Server shut down." << std::endl;
}
/**
 * listenForMessages()
 * Continuously listens for messages from the cloud.
 */
void listenForMessages(IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle) {
    while (running) {
        IoTHubDeviceClient_LL_DoWork(device_handle);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
/**
 * shutdown()
 * Handles cleanup upon program termination.
 */
void shutdown(IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle) {
    running = false;  // Signal all threads to stop
    cv.notify_all();  // Notify waiting threads
    // Clean up Azure IoT SDK
    if (device_handle != nullptr) {
        IoTHubDeviceClient_LL_Destroy(device_handle);
    }
    IoTHub_Deinit();  // Deinitialize Azure IoT SDK
}

/*
 * Author:Vivekanand Jha
 * Created:18/03/2025
 * Modified:26/03/2025
 * Azure Task By Mentor
 */

#include <azureDemo.hpp>

std::atomic<bool> running(true);
std::queue<int> taskQueue;
std::mutex queueMutex;
std::condition_variable cv;

/**
 * messageReceivedCallback()
 * This function is triggered when a message from the cloud arrives.
 */
IOTHUBMESSAGE_DISPOSITION_RESULT messageReceivedCallback(IOTHUB_MESSAGE_HANDLE message, void* userContext) {
    const char* receivedMessage = IoTHubMessage_GetString(message);
    if (receivedMessage != NULL) {
        std::cout << "Received message from Azure IoT Hub: " << receivedMessage << std::endl;
    } else {
        std::cerr << "Failed to retrieve message string." << std::endl;
    }
    IoTHubMessage_Destroy(message);
    return IOTHUBMESSAGE_ACCEPTED; 
}

/**
 * deviceToCloud()
 * This function sends a message to the cloud via Azure IoT Hub.
 */
void deviceToCloud(IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle) {
    const char* message = "{Hello from Raspberry Pi to Azure}";
    IOTHUB_MESSAGE_HANDLE message_handle = IoTHubMessage_CreateFromString(message);

    if (message_handle == NULL) {
        std::cerr << "Failed to create message" << std::endl;
    } else {
        if (IoTHubDeviceClient_LL_SendEventAsync(device_handle, message_handle, NULL, NULL) != IOTHUB_CLIENT_OK) {
            std::cerr << "Failed to send message to Azure IoT Hub" << std::endl;
        } else {
            std::cout << "Message sent: " << message << std::endl;
        }
        IoTHubMessage_Destroy(message_handle);
    }
}

/**
 * processQueue()
 * This function processes the queue when a task arrives.
 */
void processQueue(IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle) {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [] { return !taskQueue.empty() || !running; });

        while (!taskQueue.empty()) {
            int task = taskQueue.front();
            taskQueue.pop();
            lock.unlock();

            std::cout << "⚙️  Processing task " << task << std::endl;
            deviceToCloud(device_handle); 
            std::this_thread::sleep_for(std::chrono::seconds(1)); 

            lock.lock();
        }
    }
}

/**
 * listenForMessages()
 * This function continuously listens for messages from the cloud.
 */
void listenForMessages(IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle) {
    while (running) {
        IoTHubDeviceClient_LL_DoWork(device_handle);
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    }
}


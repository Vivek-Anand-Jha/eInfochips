/*
 * Author:Vivekanand Jha
 * Created:18/03/2025
 * Modified:26/03/2025
 * Azure Task By Mentor
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <azureiot/iothub.h>
#include <azureiot/iothub_device_client_ll.h>
#include <azureiot/iothubtransportmqtt.h>
#include <azureiot/iothub_message.h>

#define CONNECTION_STRING "HostName=vivek-iot-hub.azure-devices.net;DeviceId=raspberrypi;SharedAccessKey=u3uc5v3kwVXGjoIgD9OwC0htrpB9gXy2boo8FC6of3w="

std::atomic<bool> running(true);
std::queue<int> taskQueue;
std::mutex queueMutex;
std::condition_variable cv;

/**
 * messageReceivedCallback()
 * This function is triggered when a message from the cloud arrives.
 */
static IOTHUBMESSAGE_DISPOSITION_RESULT messageReceivedCallback(IOTHUB_MESSAGE_HANDLE message, void* userContext) {
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

int main() {
    IoTHub_Init();

    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(CONNECTION_STRING, MQTT_Protocol);
    if (device_handle == NULL) {
        std::cerr << "Failed to create IoT Hub device handle" << std::endl;
        return 1;
    }

    // Set up message callback for receiving messages from the cloud
    if (IoTHubDeviceClient_LL_SetMessageCallback(device_handle, messageReceivedCallback, NULL) != IOTHUB_CLIENT_OK) {
        std::cerr << "Failed to set message callback" << std::endl;
        IoTHubDeviceClient_LL_Destroy(device_handle);
        IoTHub_Deinit();
        return 1;
    } else {
        std::cout << "Listening for cloud-to-device messages..." << std::endl;
    }

    std::thread worker(processQueue, device_handle);
    std::thread messageListener(listenForMessages, device_handle);

    int taskCounter = 1;
    std::string input;

    while (running) {
        std::cout << "Press Enter to add a task, or type 'q' to quit: ";
        std::getline(std::cin, input);

        if (input == "q") {
            running = false;
            cv.notify_all();
            break;
        }

        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(taskCounter++);
        cv.notify_one();
    }

    worker.join(); // Wait for worker thread to finish
    messageListener.join(); // Wait for message listener thread to finish

    IoTHubDeviceClient_LL_Destroy(device_handle);
    IoTHub_Deinit();
    std::cout << "Process stopped.\n";

    return 0;
}


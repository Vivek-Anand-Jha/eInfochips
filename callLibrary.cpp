#include <iostream>
#include <azureDemo.hpp>

#define CONNECTION_STRING "HostName=vivek-iot-hub.azure-devices.net;DeviceId=raspberrypi;SharedAccessKey=u3uc5v3kwVXGjoIgD9OwC0htrpB9gXy2boo8FC6of3w="

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

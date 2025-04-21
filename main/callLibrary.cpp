#include <iostream>
#include <azureDemo.hpp>

#define CONNECTION_STRING "HostName=Cloud-project-task.azure-devices.net;DeviceId=eInfochipsproject;SharedAccessKey=eke4woVreeuzbNze5BSX2HMMaiOHc6CG8akUOvl5Nx8="

int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);

    IoTHub_Init();

    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(CONNECTION_STRING, MQTT_Protocol);
    if (device_handle == NULL) {
        std::cerr << "Failed to create IoT Hub device handle." << std::endl;
        IoTHub_Deinit();
        return 1;
    }

    if (IoTHubDeviceClient_LL_SetMessageCallback(device_handle, messageReceivedCallback, NULL) != IOTHUB_CLIENT_OK) {
        std::cerr << "Failed to set message callback." << std::endl;
        IoTHubDeviceClient_LL_Destroy(device_handle);
        IoTHub_Deinit();
        return 1;
    } else {
        std::cout << "Listening for cloud-to-device messages..." << std::endl;
    }

    IoTHubDeviceClient_LL_SetMessageCallback(device_handle, messageReceivedCallback, nullptr);
    std::thread workerThread([&]() { processQueue(device_handle); }); // Process queued tasks
    std::thread messageListenerThread([&]() { listenForMessages(device_handle); }); // Listen for Azure IoT Hub messages
    std::thread tcpThread([&]() { tcpSender(port); }); // Start TCP server on specified port

    int taskCounter = 1;
    std::string input;

    while (running) {
        std::cout << "Press enter or 'q':\n";
        std::getline(std::cin, input);
        if (input == "q") {
            running = false; // Signal threads to stop
            cv.notify_all(); // Notify waiting threads
            exit(1);

            break;
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            taskQueue.push(taskCounter++); // Add a new task

            cv.notify_all(); // Notify the processing thread
        }
    }

    workerThread.join();
    messageListenerThread.join();
    tcpThread.join();

    IoTHubDeviceClient_LL_Destroy(device_handle);
    IoTHub_Deinit();
    std::cout << "Process stopped.\n";

    return 0;
}


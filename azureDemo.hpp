/*
 * Author:Vivekanand Jha
 * Created:07-04-2025
 * This is header file for my Demo code 
 */

#ifndef AZUREDEMO_HPP
#define AZUREDEMO_HPP

/* Standard Headers */
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

/* userDefined Headers */
#include <azureiot/iothub.h>
#include <azureiot/iothub_device_client_ll.h>
#include <azureiot/iothubtransportmqtt.h>
#include <azureiot/iothub_message.h>

/* Function Prototypes */
extern std::atomic<bool> running;
extern std::queue<int> taskQueue;
extern std::mutex queueMutex;
extern std::condition_variable cv;

void processQueue(IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle);
void listenForMessages(IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle);
void sendHelloWorld(IOTHUB_DEVICE_CLIENT_LL_HANDLE device_handle);
IOTHUBMESSAGE_DISPOSITION_RESULT messageReceivedCallback(IOTHUB_MESSAGE_HANDLE message, void* userContext);

#endif

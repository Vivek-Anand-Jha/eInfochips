#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <iostream>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> server;
typedef websocketpp::connection_hdl connection_hdl;

std::string shared_data;
std::mutex data_mutex;

std::set<connection_hdl, std::owner_less<connection_hdl>> clients;
std::mutex clients_mutex;

bool running = true;

void websocket_server(int ws_port) {
    server ws_server;

    try {
        ws_server.init_asio();

        // Register new connection
        ws_server.set_open_handler([](connection_hdl hdl) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.insert(hdl);
            std::cout << "[WS] Client connected.\n";
        });

        // Remove connection when it closes
        ws_server.set_close_handler([](connection_hdl hdl) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(hdl);
            std::cout << "[WS] Client disconnected.\n";
        });

        // Handle incoming messages
        ws_server.set_message_handler([](connection_hdl hdl, server::message_ptr msg) {
            std::cout << "[WS] Received from client: " << msg->get_payload() << std::endl;
        });

        // Start listening on the given port
        ws_server.listen(ws_port);
        ws_server.start_accept();

        // Start the broadcaster thread
        std::thread broadcaster([&]() {
            while (running) {
                std::string data_copy;
                {
                    std::lock_guard<std::mutex> lock(data_mutex);
                    data_copy = shared_data;
                }

                std::lock_guard<std::mutex> lock(clients_mutex);
                for (auto it = clients.begin(); it != clients.end(); ) {
                    auto con = ws_server.get_con_from_hdl(*it);
                    if (con->get_state() != websocketpp::session::state::open) {
                        it = clients.erase(it); // remove closed client
                        continue;
                    }

                    websocketpp::lib::error_code ec;
                    ws_server.send(*it, data_copy, websocketpp::frame::opcode::text, ec);

                    if (ec) {
                        std::cerr << "[WS SEND ERROR] " << ec.message() << std::endl;
                        it = clients.erase(it); // remove client on error
                    } else {
                        ++it;
                    }
                }

                // Sleep to control broadcast rate
                std::this_thread::sleep_for(std::chrono::seconds(1)); // Adjust rate as needed
            }
        });

        // Start a thread to update the shared_data with simulated device readings
        std::thread data_updater([&]() {
            while (running) {
                std::this_thread::sleep_for(std::chrono::seconds(5));

                // Simulate new device data (replace with actual data collection code)
                {
                    std::lock_guard<std::mutex> lock(data_mutex);
                    shared_data = "Device Data: Temperature = " + std::to_string(rand() % 30 + 10) + "°C";  // Example data
                    std::cout << "[DATA] Updated shared data: " << shared_data << std::endl;
                }
            }
        });

        // Run the WebSocket server
        ws_server.run();

    } catch (const std::exception& e) {
        std::cerr << "[WS] Exception: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Example: parse port from command line
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    int port = std::stoi(argv[1]);

    // Start the WebSocket server
    websocket_server(port);

    return 0;
}


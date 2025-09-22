#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

std::vector<SOCKET> clients;
std::map<SOCKET, std::string> usernames; 
std::mutex clients_mutex;

void broadcast(const std::string& msg, SOCKET exclude = INVALID_SOCKET) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto client : clients) {
        if (client != exclude) {
            send(client, msg.c_str(), msg.size(), 0);
        }
    }
}

void handle_client(SOCKET client) {
    char buffer[1024];
    while (true) {
        int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break; // client disconnected
        buffer[bytes] = '\0';
        std::string msg = buffer;
        broadcast(msg, client);
    }

    {   // cleanup
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
    }
    closesocket(client);
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5555);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    listen(server, SOMAXCONN);
    std::cout << "[server] Listening on port 5555...\n";

    while (true) {
        SOCKET client = accept(server, nullptr, nullptr);
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(client);
        }
        std::thread(handle_client, client).detach();
    }

    closesocket(server);
    WSACleanup();
    return 0;
}

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include "AirplaneFleet.h"
#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define SERVER_PORT 27000
#define FILENAME "katl-kefd-B737-700.txt"  // Your input file

void sendTelemetry(const string& serverIp) {
    WSADATA wsaData;
    SOCKET clientSocket;
    sockaddr_in serverAddr;

    // Init Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed\n";
        return;
    }

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed\n";
        WSACleanup();
        return;
    }

    // Setup server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp.c_str());

    // Connect to server
    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Connection to server failed\n";
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    cout << "[CONNECTED] to server at " << serverIp << ":" << SERVER_PORT << "\n";

    // Open file
    ifstream file(FILENAME);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << FILENAME << "\n";
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    // Prepare AirplaneFleet object
    AirplaneFleet fleetPkt;
    srand(time(NULL));  // Random ID seed
    fleetPkt.SetId(rand() % 10000);

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;

        // Parse line: expected format "timestamp,fuel"
        size_t commaPos = line.find(',');
        if (commaPos == string::npos) continue;

        string timestamp = line.substr(0, commaPos);
        double fuel = stod(line.substr(commaPos + 1));

        fleetPkt.SetDateTime(timestamp);
        fleetPkt.SetFuelAmount(fuel);

        // Send the raw bytes of the struct
        int result = send(clientSocket, (char*)&fleetPkt, sizeof(fleetPkt), 0);
        if (result == SOCKET_ERROR) {
            cerr << "[ERROR] Failed to send data.\n";
            break;
        }

        cout << "[SENT] ID: " << fleetPkt.GetId() << ", " << timestamp << ", Fuel: " << fuel << "\n";
        this_thread::sleep_for(chrono::seconds(1));
    }

    file.close();
    closesocket(clientSocket);
    WSACleanup();
    cout << "[DONE] Telemetry data sent successfully.\n";
}

int main() {
    string serverIp;

    cout << "Enter the destination server IP address (e.g., 127.0.0.1): ";
    cin >> serverIp;

    sendTelemetry(serverIp);

    return 0;
}

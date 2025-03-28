#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define SERVER_PORT 27000
#define FILENAME "katl-kefd-B737-700.txt"  // Path to the telemetry data file

void sendTelemetry(const string& serverIp) {
    WSADATA wsaData;
    SOCKET clientSocket;
    sockaddr_in serverAddr;

    // Initialize Winsock
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

    // Open file with telemetry data
    ifstream file(FILENAME);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << FILENAME << "\n";
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    // Generate unique ID for the client (for SYS-050 and SYS-030)
    unsigned int clientId = rand() % 10000;
    string line;

    while (getline(file, line)) {
        if (line.empty()) continue;

        // Parse line: expected format "timestamp,fuel"
        size_t commaPos = line.find(',');
        if (commaPos == string::npos) continue;

        string timestamp = line.substr(0, commaPos);
        double fuel = stod(line.substr(commaPos + 1));

        // Create packet with client ID, timestamp, and fuel amount
        string data = to_string(clientId) + "," + timestamp + "," + to_string(fuel);

        // Send the data to the server
        int bytesSent = send(clientSocket, data.c_str(), data.length(), 0);
        if (bytesSent == SOCKET_ERROR) {
            cerr << "[ERROR] Failed to send data.\n";
            break;
        }

        cout << "[SENT] " << data << "\n";

        // Wait for a short time before sending next data (100ms delay)
        this_thread::sleep_for(chrono::milliseconds(100));
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

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS


#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define SERVER_PORT 27000
#define FILENAME "Telem_2023_3_12 14_56_40.txt"  // Your input file

mutex fileMutex;


struct TelemetryPkt
{
    char timestamp[20];
    double fuel;
};

void SendTelemetry(const string& serverIp) {

    WSADATA wsaData;
    SOCKET clientSocket;
    sockaddr_in serverAddr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "[ERROR] WSAStartup failed\n";
        return;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "[ERROR] Socket creation failed\n";
        WSACleanup();
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp.c_str());

    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "[ERROR] Connection to server failed\n";
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    std::cout << "[CONNECTED] to " << serverIp << "\n";

    ifstream file(FILENAME);
    if (!file.is_open()) {
        cerr << "[ERROR] File not found: " << FILENAME << "\n";
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;

        size_t commaPos = line.find(',');
        if (commaPos == string::npos) {
            cerr << "[WARN] Skipping malformed line: " << line << "\n";
            continue;
        }

        string timestamp = line.substr(0, commaPos);
        double fuel = stod(line.substr(commaPos + 1));

        string payload = timestamp + "," + to_string(fuel);

        int result = send(clientSocket, payload.c_str(), payload.length(), 0);
        if (result == SOCKET_ERROR) {
            cerr << "[ERROR] Send failed: " << WSAGetLastError() << "\n";
            break;
        }

        std::cout << "[SENT] " << payload << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    file.close();
    closesocket(clientSocket);
    WSACleanup();
    std::cout << "[DONE] Client completed.\n";
}


int main(int argc, char* argv[]) {
    //string serverIp;

    //cout << "Enter the destination server IP address (e.g., 127.0.0.1): ";
    //cin >> serverIp;

    SendTelemetry(argv[1]);

    return 0;
}
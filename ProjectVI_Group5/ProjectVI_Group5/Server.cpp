#define _CRT_SECURE_NO_WARNINGS
#define MIN_FUEL_AMOUNT 10
#define PORT 27000

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <thread>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#include <mutex>
#include <map>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

mutex fileMutex;
mutex fuelMutex;

double globalTotalFuel = 0.0;

// Structure to track flight data per airplane
struct FlightSession {
    string airplaneId;
    vector<double> fuelReadings;
    vector<string> timestamps;
};

// Global map to store sessions per airplane
map<string, FlightSession> airplaneSessions;

// Helper to parse data: ID,time,fuel
bool ParseTelemetryPacket(const string& packet, string& id, string& timestamp, double& fuelAmount) {
    size_t firstComma = packet.find(',');
    size_t secondComma = packet.find(',', firstComma + 1);

    if (firstComma == string::npos || secondComma == string::npos) return false;

    id = packet.substr(0, firstComma);
    timestamp = packet.substr(firstComma + 1, secondComma - firstComma - 1);

    try {
        fuelAmount = stod(packet.substr(secondComma + 1));
    }
    catch (...) {
        return false;
    }
    return true;
}

void HandleClient(SOCKET clientSocket) {
    char buffer[1024];
    string clientId;
    FlightSession session;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            cout << "Client disconnected. Processing final data...\n";
            break;
        }

        string packet(buffer);
        string timestamp;
        double fuelAmount;

        if (ParseTelemetryPacket(packet, clientId, timestamp, fuelAmount)) {
            session.airplaneId = clientId;
            session.timestamps.push_back(timestamp);
            session.fuelReadings.push_back(fuelAmount);

            cout << "[RECEIVED] ID: " << clientId << ", Time: " << timestamp << ", Fuel: " << fuelAmount << endl;

            // Save individual reading
            {
                lock_guard<mutex> lock(fileMutex);
                ofstream file("airplane_fleet_data.txt", ios::app);
                file << "ID: " << clientId << ", Time: " << timestamp << ", Fuel: " << fuelAmount << endl;
                file.close();
            }

            send(clientSocket, "OK", sizeof("OK"), 0);
        }
        else {
            cerr << "Invalid packet received: " << packet << endl;
            send(clientSocket, "ERR", sizeof("ERR"), 0);
        }
    }

    // After disconnect, store average fuel consumption
    if (!session.fuelReadings.empty()) {
        double total = 0.0;
        for (double f : session.fuelReadings) total += f;
        double avgFuel = total / session.fuelReadings.size();

        {
            lock_guard<mutex> lock(fileMutex);
            ofstream file("airplane_fleet_data.txt", ios::app);
            file << ">>> Flight for Airplane ID: " << clientId << " ended.\n";
            file << ">>> Average Fuel Consumption: " << avgFuel << " <<<\n\n";
            file.close();
        }

        {
            lock_guard<mutex> lock(fuelMutex);
            globalTotalFuel += total;
        }

        cout << "[INFO] Flight ended for ID: " << clientId << ", Avg Fuel: " << avgFuel << endl;
    }

    closesocket(clientSocket);
    cout << "Client thread exiting.\n";
}

int main() {
    WSADATA wsaData;
    SOCKET ServerSocket;
    sockaddr_in serverAddr;
    vector<thread> clientThreads;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed.\n";
        return -1;
    }

    ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ServerSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed.\n";
        WSACleanup();
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(ServerSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Bind failed.\n";
        closesocket(ServerSocket);
        WSACleanup();
        return -1;
    }

    if (listen(ServerSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed.\n";
        closesocket(ServerSocket);
        WSACleanup();
        return -1;
    }

    cout << "[SERVER STARTED] Listening on port " << PORT << "...\n";

    while (true) {
        SOCKET clientSocket = accept(ServerSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Failed to accept client.\n";
            continue;
        }

        clientThreads.emplace_back(thread(HandleClient, clientSocket));
    }

    for (auto& th : clientThreads)
        if (th.joinable()) th.join();

    closesocket(ServerSocket);
    WSACleanup();
    return 0;
}
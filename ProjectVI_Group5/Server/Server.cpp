#define _CRT_SECURE_NO_WARNINGS
#define MIN_FUEL_AMOUNT 10
#define PORT 27000

#include "AirplaneFleet.h"
#include <winsock2.h>
#include <mutex>
#include <windows.h>
#include <iostream>
#include <thread>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")


struct TelemetryPkt
{
    char timestamp[20];
    double fuel;
};

void ReadTransmittedTelemetryData(string& buffer, AirplaneFleet& temp)
{
    // check to see if packet is empty
    if (!buffer.empty()) { return; }

    // initialize delimiter
    string delimiter = ",";

    // do not think it will be senting the ID in the packet
    string dateTime = buffer.substr(0, buffer.find(delimiter));
    string fuelAmountString = buffer.substr(dateTime.size() + 1, buffer.find(delimiter));

    // convert to double
    double fuelAmount = stod(fuelAmountString);

    temp.SetDateTime(dateTime);
    temp.SetFuelAmount(fuelAmount);
}

void CalculateFuelConsumption(AirplaneFleet& temp, SOCKET& ConnectionSocket)
{
    double currFuelAmount = temp.GetFuelAmount();
    if (currFuelAmount < MIN_FUEL_AMOUNT)
    {
        cerr << "LOW FUEL ON AIRPLANE ID: " + temp.GetId() << endl;
        const char warningFuelMessage[] = "LOW FUEL ON AIRPLANE PLEASE LAND TO THE NEAREST POSSIBLE LOCATION";
        // SEND PACKET TO PLANE TO OF WARNING
        send(ConnectionSocket, warningFuelMessage, sizeof(warningFuelMessage), 0);
    }
}

mutex fileMutex;  // Global mutex for file access


void HandleClient(SOCKET clientSocket) {
    AirplaneFleet FleetPkt;

    // Assign random ID to the plane
    srand((unsigned)time(NULL));
    int id = rand() % (100000);
    FleetPkt.SetId(id);

    std::cout << "Airplane ID in flight: " << FleetPkt.GetId() << std::endl;

    while (true) {
        char buffer[sizeof(TelemetryPkt)];
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            std::cout << "Client disconnected. Processing final data...\n";
            break;
        }

        // Cast the received buffer back to the TelemetryPkt struct
        TelemetryPkt* receivedPkt = (TelemetryPkt*)buffer;

        // Set values to airfleet object
        FleetPkt.SetDateTime(receivedPkt->timestamp);
        FleetPkt.SetFuelAmount(receivedPkt->fuel);

        {
            // Protect file writing with a scoped lock
            scoped_lock lock(fileMutex);
            std::ofstream file("airplane_fleet_data.txt", std::ios::app); // Open file in append mode
            if (!file) {
                cerr << "Error opening file for writing." << endl;
                return;
            }
            file << FleetPkt.GetId() << "," << FleetPkt.GetDate() << "," << FleetPkt.GetFuelAmount() << std::endl;
        } // File automatically closes when going out of scope

        send(clientSocket, "OK", sizeof("OK"), 0);
    }

    closesocket(clientSocket);
    std::cout << "Client thread exiting.\n";
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

    std::cout << "[SERVER STARTED] Listening on port " << PORT << "...\n";

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
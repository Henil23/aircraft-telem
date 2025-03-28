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

void HandleClient(SOCKET clientSocket) {

    ofstream file("airplane_fleet_data.txt", ios::app); // Open file in append mode
    if (!file) {
        cerr << "Error opening file for writing." << endl;
        return;
    }

    AirplaneFleet FleetPkt;

    // assign random ID to the plane
    srand((unsigned)time(NULL));
    int id = rand() % (100000);
    FleetPkt.SetId(id);

    while (true) {
        char buffer[sizeof(TelemetryPkt)];
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            cout << "Client disconnected. Processing final data...\n";
            break;
        }

        // Cast the received buffer back to the TelemetryPkt struct
        TelemetryPkt* receivedPkt = (TelemetryPkt*)buffer;

        // set values to airfleet object
        FleetPkt.SetDateTime(receivedPkt->timestamp);
        FleetPkt.SetFuelAmount(receivedPkt->fuel);

        if (ParseTelemetryPacket(packet, clientId, timestamp, fuelAmount)) {
            session.airplaneId = clientId;
            session.timestamps.push_back(timestamp);
            session.fuelReadings.push_back(fuelAmount);

            cout << "[RECEIVED] ID: " << clientId << ", Time: " << timestamp << ", Fuel: " << fuelAmount << endl;

        // Log data
        file << to_string(FleetPkt.GetId()) + "," + FleetPkt.GetDate() + "," + to_string(FleetPkt.GetFuelAmount()) << endl;

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
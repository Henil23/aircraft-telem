// PROJECT-VI-GROUP-5
// AIRPLANE FLEET -> ID, date (Date & Time), fuel consumption

#define _CRT_SECURE_NO_WARNINGS
#define MIN_FUEL_AMOUNT		10
#define PORT			 27000

#include "AirplaneFleet.h"
#include <windows.networking.sockets.h>
#include <iostream>
#include <thread>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#pragma comment(lib, "Ws2_32.lib")

using namespace std;


void ReadTransmittedTelemetryData(string& buffer, AirplaneFleet& temp)
{
	// check to see if packet is empty
	if (!buffer.empty()) { return; }

	// initialize delimiter
	string delimiter = ",";

	// do not think it will be senting the ID in the packet
	string dateTime = buffer.substr(0, buffer.find(delimiter));
	string fuelAmountString = buffer.substr(dateTime.size() + 1); // skips delimiter

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
		const char warningFuelMessage [] = "LOW FUEL ON AIRPLANE PLEASE LAND TO THE NEAREST POSSIBLE LOCATION";
		// SEND PACKET TO PLANE TO OF WARNING
		send(ConnectionSocket, warningFuelMessage, sizeof(warningFuelMessage), 0);
	}
}

// thread Function to Handle a Single Client**
void HandleClient(SOCKET clientSocket) {

    ofstream file("airplane_fleet_data.txt", ios::app); // Open file in append mode
    if (!file) {
        cerr << "Error opening file for writing." << endl;
        return;
    }

    AirplaneFleet FleetPkt;

    // assign random ID to the plane
    srand(time(NULL));
    FleetPkt.SetId(rand() % 10000);

    while (true) {
        char buffer[50];
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            cout << "Client disconnected. Closing thread.\n";
            break;
        }

        memcpy((char*)&FleetPkt, buffer, sizeof(AirplaneFleet));

        // Process fuel level
        CalculateFuelConsumption(FleetPkt, clientSocket);

        // Log data
        file << FleetPkt.GetId() + "," + FleetPkt.GetDate() + "," + to_string(FleetPkt.GetFuelAmount()) << endl;

        send(clientSocket, "OK", sizeof("OK"), 0);
    }

    file.close();

    closesocket(clientSocket);
    cout << "Client thread exiting." << endl;
}

int main() {
    WSADATA wsaData;
    SOCKET ServerSocket;
    sockaddr_in SvrAddr;
    vector<thread> clientThreads;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed.\n";
        return -1;
    }

    ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ServerSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed.\n";
        return -1;
    }

    SvrAddr.sin_family = AF_INET;
    SvrAddr.sin_addr.s_addr = INADDR_ANY;
    SvrAddr.sin_port = htons(PORT);

    if (bind(ServerSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR) {
        cerr << "Binding failed.\n";
        return -1;
    }

    if (listen(ServerSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listening failed.\n";
        return -1;
    }

    cout << "Server is running. Waiting for connections...\n";

    // need a better termination condition
    while (true) {
        SOCKET clientSocket = accept(ServerSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Client accept failed.\n";
            continue;
        }

        // executes thread and stores in the vector
        clientThreads.emplace_back(thread(HandleClient, clientSocket));
    }

    // finishes execution of threads
    for (auto& th : clientThreads) {
        if (th.joinable()) {
            th.join();
        }
    }

    closesocket(ServerSocket);
    WSACleanup();
    return 0;
}
#define  _CRT_SECURE_NO_WARNINGS

#include "AirplaneFleet.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
using namespace std;

// Default Constructor
AirplaneFleet::AirplaneFleet()
    : id(0), dateTime("N/A"), fuelAmount(0.0) {
}

// Constructor
AirplaneFleet::AirplaneFleet(unsigned int id, const string& dateTime, double fuelAmount)
    : id(id), dateTime(dateTime), fuelAmount(fuelAmount) {
}

// Function to get formatted date-time string
string AirplaneFleet::GetFormattedDateTime() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    stringstream ss;

    ss << ltm->tm_mday << "_" << (ltm->tm_mon + 1) << "_" << (ltm->tm_year + 1900)
        << " " << setw(2) << setfill('0') << ltm->tm_hour << ":"
        << setw(2) << setfill('0') << ltm->tm_min << ":"
        << setw(2) << setfill('0') << ltm->tm_sec;

    return ss.str();
}

// Getters
string AirplaneFleet::GetDate() const { return dateTime; }
double AirplaneFleet::GetFuelAmount() const { return fuelAmount; }
unsigned int AirplaneFleet::GetId() const { return id; }

// Setters
void AirplaneFleet::SetId(unsigned int id) { this->id = id; }
void AirplaneFleet::SetDateTime(const string& date) { this->dateTime = date; }
void AirplaneFleet::SetFuelAmount(double fuel) { this->fuelAmount = fuel; }

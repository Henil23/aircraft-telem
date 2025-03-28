#pragma once
#include <string>  // Include necessary libraries

class AirplaneFleet
{
public:
    // [Default] Constructor
    AirplaneFleet();
    // Constructor
    AirplaneFleet(int id, const std::string& dateTime, double fuelAmount);

    // Function to get formatted date-time string
    std::string GetFormattedDateTime();

    // Getters/Setters
    std::string GetDate() const;
    double GetFuelAmount() const;
    unsigned int GetId() const;

    void SetId(int id);
    void SetDateTime(const std::string& date);
    void SetFuelAmount(double fuel);

private:
    unsigned int id;
    std::string dateTime;
    double fuelAmount;
};
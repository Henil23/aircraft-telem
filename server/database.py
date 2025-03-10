# LEFT TO DO: Implement the MongoDB class to insert and retrieve telemetry data.


from pymongo import MongoClient

class MongoDB:
    def __init__(self, db_name="AircraftTelemetry"):
        """Initialize the MongoDB connection."""
        self.client = MongoClient("mongodb://localhost:27017/")
        self.db = self.client[db_name]
        self.flights = self.db["flights"]  # Collection for telemetry data

    def insert_telemetry(self, airplane_id, timestamp, fuel_remaining, fuel_consumption_rate):
        """Insert telemetry data into MongoDB."""
        telemetry_data = {
            "airplane_id": airplane_id,
            "timestamp": timestamp,
            "fuel_remaining": fuel_remaining,
            "fuel_consumption_rate": fuel_consumption_rate
        }
        self.flights.insert_one(telemetry_data)
        print(f"[INFO] Data inserted for {airplane_id} at {timestamp}")

    def get_all_telemetry(self):
        """Retrieve all telemetry data."""
        return list(self.flights.find({}, {"_id": 0}))  # Exclude MongoDB's _id field

if __name__ == "__main__":
    # Test MongoDB connection
    db = MongoDB()
    db.insert_telemetry("A123", "2025-03-10T12:34:56", 5000, 12.5)  # Sample Data
    print(db.get_all_telemetry())  # Fetch all stored data

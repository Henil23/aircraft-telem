import socket
import threading
from datetime import datetime
from flask import Flask, jsonify
import json

# Server configuration
HOST = "0.0.0.0"
PORT = 5003

# Dictionary to store telemetry data (temporary storage instead of DB)
telemetry_data = {}

def parse_telemetry_data(data):
    """Parses telemetry data received from the client."""
    try:
        timestamp_str, fuel_remaining = data.split(",")
        timestamp = datetime.strptime(timestamp_str, "%Y-%m-%dT%H:%M:%S")
        fuel_remaining = float(fuel_remaining)
        return timestamp, fuel_remaining
    except ValueError:
        print("[ERROR] Invalid data format")
        return None, None


def calculate_fuel_consumption(airplane_id, timestamp, fuel_remaining):
    """Calculates fuel consumption rate and stores data in memory."""
    global telemetry_data  # Ensure we are modifying the global variable

    fuel_consumption_rate = None

    if airplane_id in telemetry_data:
        prev_timestamp, prev_fuel = telemetry_data[airplane_id]["last_data"]
        time_diff = (timestamp - prev_timestamp).total_seconds() / 60.0
        fuel_used = prev_fuel - fuel_remaining

        if time_diff > 0:
            fuel_consumption_rate = fuel_used / time_diff
            print(f"[FUEL USAGE] {airplane_id} - {fuel_consumption_rate:.2f} gallons/min")

    # Store data in memory
    telemetry_data[airplane_id] = {
        "timestamp": timestamp.strftime("%Y-%m-%dT%H:%M:%S"),
        "fuel_remaining": fuel_remaining,
        "fuel_consumption_rate": fuel_consumption_rate,
        "last_data": (timestamp, fuel_remaining)
    }

    # Print telemetry data for debugging
    print("[DEBUG] Current Telemetry Data:")
    # Convert datetime objects to string before printing
    telemetry_data_serializable = {
        k: {**v, "last_data": (v["last_data"][0].strftime("%Y-%m-%dT%H:%M:%S"), v["last_data"][1])}
        for k, v in telemetry_data.items()
    }
    print(json.dumps(telemetry_data_serializable, indent=2))  # Pretty-print telemetry data



def handle_client(client_socket, address):
    """Handles an individual client connection."""
    print(f"[NEW CONNECTION] Connected to {address}")

    airplane_id = f"AIRCRAFT_{address[1]}"  # Generate a unique ID for the aircraft

    while True:
        try:
            data = client_socket.recv(4096).decode("utf-8")
            if not data:
                break  # Client disconnected

            timestamp, fuel_remaining = parse_telemetry_data(data)
            if timestamp and fuel_remaining is not None:
                calculate_fuel_consumption(airplane_id, timestamp, fuel_remaining)

        except ConnectionResetError:
            print(f"[DISCONNECTED] Client {address} disconnected abruptly")
            break

    client_socket.close()
    print(f"[CONNECTION CLOSED] {address}")

def start_server():
    """Starts the multi-client TCP server."""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((HOST, PORT))
    server.listen(5)

    print(f"[SERVER STARTED] Listening on {HOST}:{PORT}")

    while True:
        client_socket, addr = server.accept()
        thread = threading.Thread(target=handle_client, args=(client_socket, addr))
        thread.start()

        print(f"[ACTIVE CONNECTIONS] {threading.active_count() - 1}")

if __name__ == "__main__":
    start_server()

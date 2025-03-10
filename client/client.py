import socket
import time
import csv

# Server configuration (use the correct IP if running on different machines)
SERVER_IP = "127.0.0.1"  # Change to the actual server IP if needed
SERVER_PORT = 5003

TELEMETRY_FILE = "telemetry_data.csv"  # Sample telemetry data file

def send_telemetry_data():
    """Reads telemetry data from a CSV file and sends it to the server."""
    try:
        # Connect to the server
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((SERVER_IP, SERVER_PORT))
        print(f"[CONNECTED] Sending data to {SERVER_IP}:{SERVER_PORT}")

        with open(TELEMETRY_FILE, "r") as file:
            csv_reader = csv.reader(file)
            next(csv_reader)  # Skip the header row

            for row in csv_reader:
                timestamp, fuel_remaining = row  # Read time & fuel data
                message = f"{timestamp},{fuel_remaining}"
                
                client.send(message.encode("utf-8"))  # Send data to server
                print(f"[SENT] {message}")

                time.sleep(1)  # Simulate real-time data transmission

        print("[COMPLETED] All data sent")
        client.close()  # Close connection

    except ConnectionRefusedError:
        print("[ERROR] Cannot connect to the server. Make sure it's running.")

if __name__ == "__main__":
    send_telemetry_data()

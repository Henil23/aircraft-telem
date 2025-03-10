import socket
import threading

# Server configuration
HOST = "0.0.0.0"  # Listen on all network interfaces
PORT = 5003        # Server port

def handle_client(client_socket, address):
    """Handles an individual client connection."""
    print(f"[NEW CONNECTION] Connected to {address}")

    while True:
        try:
            data = client_socket.recv(1024).decode("utf-8")  # Receive data
            if not data:
                break  # If no data, client disconnected

            print(f"[DATA RECEIVED] {address}: {data}")

        except ConnectionResetError:
            print(f"[DISCONNECTED] Client {address} disconnected abruptly")
            break

    client_socket.close()  # Close client socket
    print(f"[CONNECTION CLOSED] {address}")

def start_server():
    """Starts the multi-client TCP server."""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Create socket
    server.bind((HOST, PORT))
    server.listen(5)  # Allow 5 clients in backlog

    print(f"[SERVER STARTED] Listening on {HOST}:{PORT}")

    while True:
        client_socket, addr = server.accept()  # Accept client connection
        thread = threading.Thread(target=handle_client, args=(client_socket, addr))
        thread.start()  # Start a new thread for each client

        print(f"[ACTIVE CONNECTIONS] {threading.active_count() - 1}")

if __name__ == "__main__":
    start_server()

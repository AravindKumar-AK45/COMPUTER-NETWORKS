import socket
import json

HOST = '127.0.0.1'
PORT = 54321

# In-memory mock database of student information records
STUDENT_DATABASE = {
    "REG001": {"name": "Arun Kumar", "dept": "Computer Science", "semester": 6, "cgpa": 8.92},
    "REG002": {"name": "Priya Sharma", "dept": "Electronics", "semester": 4, "cgpa": 9.15},
    "REG003": {"name": "John Doe", "dept": "Mechanical", "semester": 8, "cgpa": 7.45},
    "REG004": {"name": "Sneha Reddy", "dept": "Information Technology", "semester": 2, "cgpa": 8.80}
}

def start_udp_server():
    # Initialize a UDP socket using SOCK_DGRAM
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_socket.bind((HOST, PORT))
    
    print(f"[UDP SERVER STARTED] Listening on {HOST}:{PORT}...")
    print("Awaiting lookup requests from clients...\n")
    
    while True:
        try:
            # Receive datagram payload and client routing metadata
            raw_data, client_address = server_socket.recvfrom(1024)
            reg_number = raw_data.decode('utf-8').strip().upper()
            
            print(f"[REQUEST] Received lookup query for '{reg_number}' from {client_address}")
            
            # Search the student entity database mapping
            if reg_number in STUDENT_DATABASE:
                response = {
                    "status": "FOUND",
                    "data": STUDENT_DATABASE[reg_number]
                }
            else:
                response = {
                    "status": "NOT_FOUND",
                    "message": f"Error: Student with Registration Number '{reg_number}' does not exist."
                }
            
            # Serialize payload to JSON and reply directly back to the sender's address
            server_socket.sendto(json.dumps(response).encode('utf-8'), client_address)
            
        except Exception as e:
            print(f"Server encountered an execution error: {e}")

if __name__ == "__main__":
    start_udp_server()


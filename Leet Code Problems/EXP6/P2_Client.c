import socket
import json

SERVER_HOST = '127.0.0.1'
SERVER_PORT = 54321
TIMEOUT_DURATION = 3.0 # Maximum seconds to wait for a response

def run_udp_client():
    # Initialize a connectionless UDP socket (SOCK_DGRAM)
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    client_socket.settimeout(TIMEOUT_DURATION)
    
    print("⚡ Student Information Lookup Client Initialised.")
    print("Type 'exit' or 'quit' at any time to terminate the session.\n")
    
    while True:
        # User input collection and validation
        reg_number = input("Enter Student Registration Number to lookup: ").strip()
        
        if not reg_number:
            print("⚠️ Input cannot be empty. Please try again.")
            continue
            
        if reg_number.lower() in ['exit', 'quit']:
            print("Terminating lookup client. Goodbye!")
            break
            
        try:
            # Transmit datagram packet directly to the server host target
            client_socket.sendto(reg_number.encode('utf-8'), (SERVER_HOST, SERVER_PORT))
            
            # Await incoming datagram response packet
            raw_response, _ = client_socket.recvfrom(2048)
            response = json.loads(raw_response.decode('utf-8'))
            
            # Parse and render structural text results 
            if response["status"] == "FOUND":
                student = response["data"]
                print(f"\n{'-'*35}")
                print(f"✅ STUDENT RECORD FOUND")
                print(f"📌 Reg No   : {reg_number.upper()}")
                print(f"👤 Name     : {student['name']}")
                print(f"🏢 Dept     : {student['dept']}")
                print(f"📅 Semester : {student['semester']}")
                print(f"🏅 CGPA     : {student['cgpa']}")
                print(f"{'-'*35}\n")
            else:
                print(f"\n❌ {response['message']}\n")
                
        except socket.timeout:
            print("\n⏳ [TIMEOUT ERROR] No response from the server. Verify if it's currently running.\n")
        except Exception as e:
            print(f"\n⚠️ Client error encountered: {e}\n")
            
    client_socket.close()

if __name__ == "__main__":
    run_udp_client()


import socket
import json
import time
import random
import hashlib

HOST = '127.0.0.1'
PORT = 9999
TIMEOUT_INTERVAL = 2.0 # Wait 2 seconds for ACK before retransmitting
MAX_RETRANSMISSIONS = 3

class ReliableWeatherClient:
    def __init__(self, station_id):
        self.station_id = station_id
        self.seq_num = 0
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(TIMEOUT_INTERVAL) # Set application timeout loop
        
    def generate_weather_packet(self):
        """Simulates varying real-time sensor metrics."""
        metrics = {
            "temp": round(random.uniform(20.0, 42.0), 2),
            "humidity": round(random.uniform(30.0, 90.0), 2),
            "pressure": round(random.uniform(980.0, 1025.0), 2),
            "wind_speed": round(random.uniform(0.0, 25.0), 2)
        }
        
        payload = {
            "station_id": self.station_id,
            "seq_num": self.seq_num,
            "metrics": metrics
        }
        
        # Serialize payload deterministically for checksum comparison
        payload_str = json.dumps(payload, sort_keys=True)
        checksum = hashlib.md5(payload_str.encode('utf-8')).hexdigest()
        
        return {
            "type": "TELEMETRY",
            "checksum": checksum,
            "payload": payload
        }

    def send_telemetry_reliably(self):
        """Sends data packets and implements Stop-and-Wait ARQ retransmission logic."""
        packet = self.generate_weather_packet()
        attempts = 0
        
        while attempts < MAX_RETRANSMISSIONS:
            try:
                print(f"🚀 Sending Seq {self.seq_num} (Attempt {attempts + 1}/{MAX_RETRANSMISSIONS})...")
                self.sock.sendto(json.dumps(packet).encode('utf-8'), (HOST, PORT))
                
                # Block waiting for server ACK window matching sequence space
                raw_ack, _ = self.sock.recvfrom(1024)
                ack = json.loads(raw_ack.decode('utf-8'))
                
                if ack.get("type") == "ACK" and ack.get("seq_num") == self.seq_num:
                    print(f"✅ ACK verified by Server for Seq {self.seq_num}.")
                    self.seq_num += 1 # Advance sliding window sequence boundary
                    return True
                    
            except socket.timeout:
                print(f"⚠️ [TIMEOUT] No ACK received for Seq {self.seq_num}. Retransmitting...")
                attempts += 1
                time.sleep(0.5) # Linear backoff delay
                
        print(f"❌ [CRITICAL] Packet Seq {self.seq_num} dropped out entirely after maximum retransmissions.")
        self.seq_num += 1 # Advance sequence anyway to simulate a dropped link frame
        return False

    def query_station_stats(self, target_station_id):
        """Queries the server for real-time telemetry metrics and calculated averages."""
        query_payload = {"station_id": target_station_id}
        payload_str = json.dumps(query_payload, sort_keys=True)
        checksum = hashlib.md5(payload_str.encode('utf-8')).hexdigest()
        
        query_packet = {
            "type": "QUERY",
            "checksum": checksum,
            "payload": query_payload
        }
        
        try:
            print(f"\n🔍 Querying global averages for station: '{target_station_id}'...")
            self.sock.sendto(json.dumps(query_packet).encode('utf-8'), (HOST, PORT))
            
            raw_res, _ = self.sock.recvfrom(4096)
            response = json.loads(raw_res.decode('utf-8'))
            print(json.dumps(response, indent=4))
        except socket.timeout:
            print("❌ Query failed: Server is unreachable or timed out.")

if __name__ == "__main__":
    # Prompt user to spin up a specific client name workspace
    station_name = input("Enter a Unique Weather Station ID (e.g., Station_Alpha): ").strip()
    client = ReliableWeatherClient(station_name)
    
    print("\n--- Starting Real-Time Weather Streaming Loop ---")
    for _ in range(5):
        client.send_telemetry_reliably()
        time.sleep(3) # Send telemetry every 3 seconds
        
    # Request latest rolling metrics calculation summary update
    client.query_station_stats(station_name)


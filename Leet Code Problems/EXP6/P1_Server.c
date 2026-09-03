import socket
import json
import hashlib

HOST = '127.0.0.1'
PORT = 9999

# In-memory storage for weather tracking
# Format: { station_id: { "expected_seq": int, "history": [...], "latest": {} } }
weather_db = {}

def calculate_checksum(data_str):
    """Generates an MD5 checksum to verify data integrity over raw UDP packets."""
    return hashlib.md5(data_str.encode('utf-8')).hexdigest()

def process_telemetry(payload):
    """Validates metrics, drops duplicate sequences, and computes running averages."""
    station_id = payload["station_id"]
    seq_num = payload["seq_num"]
    metrics = payload["metrics"]
    
    # Register new station profiles dynamically
    if station_id not in weather_db:
        weather_db[station_id] = {
            "expected_seq": 0,
            "history": [],
            "latest": {}
        }
    
    station = weather_db[station_id]
    
    # Duplicate Packet Detection
    if seq_num < station["expected_seq"]:
        return f"DUPLICATE packet ignored (Received Seq: {seq_num}, Expected Seq: {station['expected_seq']})"
    
    # Missing Packet / Gap Detection
    gap_msg = ""
    if seq_num > station["expected_seq"]:
        missing_count = seq_num - station["expected_seq"]
        gap_msg = f"⚠️ [GAP DETECTED] Missing {missing_count} packet(s) from {station_id}! "
        station["expected_seq"] = seq_num # Resync window to current sequence number
        
    # Commit valid transaction to historical engine
    station["history"].append(metrics)
    station["latest"] = metrics
    station["expected_seq"] += 1
    
    # Compute system-wide rolling statistical averages
    history = station["history"]
    count = len(history)
    
    avg_metrics = {
        "avg_temp": round(sum(d["temp"] for d in history) / count, 2),
        "avg_humidity": round(sum(d["humidity"] for d in history) / count, 2),
        "avg_pressure": round(sum(d["pressure"] for d in history) / count, 2),
        "avg_wind_speed": round(sum(d["wind_speed"] for d in history) / count, 2),
        "total_packets_recorded": count
    }
    
    station["stats"] = avg_metrics
    return f"{gap_msg}Telemetry committed successfully. Total metrics logged: {count}."

def start_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_socket.bind((HOST, PORT))
    print(f"[WEATHER SERVER RUNNING] Listening on {HOST}:{PORT} via UDP...")
    
    while True:
        try:
            raw_data, client_addr = server_socket.recvfrom(4096)
            packet = json.loads(raw_data.decode('utf-8'))
            
            p_type = packet.get("type")
            received_checksum = packet.get("checksum")
            payload = packet.get("payload", {})
            payload_str = json.dumps(payload, sort_keys=True)
            
            # Application Layer Error Detection (Checksum Verification)
            if calculate_checksum(payload_str) != received_checksum:
                print(f"❌ [CORRUPTED PACKET] Checksum mismatch from {client_addr}. Dropping packet.")
                continue
                
            if p_type == "TELEMETRY":
                # Process the data
                result_log = process_telemetry(payload)
                print(f"[{payload['station_id']}] Seq {payload['seq_num']} | {result_log}")
                
                # Send Reliability Acknowledgement (ACK)
                ack_packet = {
                    "type": "ACK",
                    "seq_num": payload["seq_num"],
                    "status": "RECEIVED"
                }
                server_socket.sendto(json.dumps(ack_packet).encode('utf-8'), client_addr)
                
            elif p_type == "QUERY":
                target_station = payload.get("station_id")
                print(f"[QUERY] Client {client_addr} requested stats for: '{target_station}'")
                
                if target_station in weather_db:
                    response_payload = {
                        "status": "SUCCESS",
                        "station_id": target_station,
                        "latest": weather_db[target_station]["latest"],
                        "averages": weather_db[target_station]["stats"]
                    }
                else:
                    response_payload = {"status": "ERROR", "message": "Station ID profile not found."}
                    
                server_socket.sendto(json.dumps(response_payload).encode('utf-8'), client_addr)
                
        except Exception as e:
            print(f"Server processing fault encountered: {e}")

if __name__ == "__main__":
    start_server()


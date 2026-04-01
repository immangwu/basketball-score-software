"""
Serial Bridge — PC to Arduino UNO
===================================
Reads state.json and sends game data to Arduino via USB serial.

Install: pip install pyserial
Usage  : python serial_bridge.py
         python serial_bridge.py --port COM3   (override port)
"""

import json, time, os, argparse
import serial, serial.tools.list_ports

STATE_FILE = os.path.join(os.path.dirname(__file__), "state.json")
BAUD_RATE  = 9600
INTERVAL   = 0.5   # send every 0.5 seconds

def find_arduino():
    for p in serial.tools.list_ports.comports():
        desc = (p.description or "").lower()
        if any(x in desc for x in ["arduino", "ch340", "cp210", "usb serial", "uno"]):
            return p.device
    ports = serial.tools.list_ports.comports()
    return ports[0].device if ports else None

def load_state():
    try:
        with open(STATE_FILE) as f:
            return json.load(f)
    except:
        return None

def build_packet(state):
    """
    Packet format:
      S,<teamA_name>,<scoreA>,<teamB_name>,<scoreB>,<clock>,<quarter>\n
    Example:
      S,SRIT,16,SREC,3,11:47,3\n
    """
    na = state.get("team_a", {}).get("name", "A")[:6]
    nb = state.get("team_b", {}).get("name", "B")[:6]
    sa = state.get("team_a", {}).get("score", 0)
    sb = state.get("team_b", {}).get("score", 0)
    ck = state.get("game_clock", "00:00")
    q  = state.get("quarter", 1)
    return f"S,{na},{sa},{nb},{sb},{ck},{q}\n"

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default=None)
    args = parser.parse_args()

    port = args.port or find_arduino()
    if not port:
        print("ERROR: No Arduino found. Plug in USB or use --port COM3")
        return

    print(f"\n{'='*45}")
    print(f"  Basketball Scoreboard — Serial Bridge")
    print(f"  Arduino Port : {port}")
    print(f"  Baud Rate    : {BAUD_RATE}")
    print(f"  State File   : {STATE_FILE}")
    print(f"{'='*45}\n")

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)   # wait for Arduino reset
        print("Connected! Sending data...\n")
    except Exception as e:
        print(f"ERROR: {e}")
        return

    last_packet = ""
    try:
        while True:
            state = load_state()
            if state:
                packet = build_packet(state)
                if packet != last_packet:
                    ser.write(packet.encode("ascii"))
                    ser.flush()
                    print(f"→ {packet.strip()}")
                    last_packet = packet
            time.sleep(INTERVAL)
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ser.close()

if __name__ == "__main__":
    main()

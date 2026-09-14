# -*- coding: utf-8 -*-
"""
Magnetic Motion Logger
Receives CSV data from Wio Terminal via Serial and saves it to organized folders.

Usage:
    1. Run: python logger.py
    2. On Wio Terminal, press B3 to select class
    3. Press B1 to start recording
    4. Move the magnet/car during the recording window
    5. The file is saved automatically

Works with any class name sent by the device (e.g., FAST, SLOW, NO_CAR,
STRAIGHT, OSCILLATE, PAUSE, etc.)
"""

import serial
import os
import glob
import time

# ============================================
# Configuration
# ============================================
SERIAL_PORT = "COM5"    # Change to match your device
BAUDRATE = 9600


# ============================================
# Main Logic
# ============================================
def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
        print(f"[OK] Connected to {SERIAL_PORT} at {BAUDRATE} baud")
        print("[WAIT] Ready to receive data...")
        print("[INFO] Press B3 to change class, then B1 to start recording")
        print("-" * 50)
    except Exception as e:
        print(f"[ERROR] Connection failed: {e}")
        return

    current_file = None
    current_class = "UNKNOWN"
    file_counter = 0
    sample_counter = 0
    start_time = None

    while True:
        try:
            raw_line = ser.readline()
            if not raw_line:
                continue

            line = raw_line.decode('utf-8', errors='ignore').strip()
            if not line:
                continue

            print(f"[RECV] {line}")

            # ----------------------------------------------
            # 1. Start of a new recording
            # ----------------------------------------------
            if line.startswith("START,"):
                current_class = line[6:]
                if not current_class:
                    current_class = "UNKNOWN"

                folder_path = f"dataset/{current_class}"
                os.makedirs(folder_path, exist_ok=True)

                existing_files = glob.glob(f"{folder_path}/{current_class}_*.csv")
                file_counter = len(existing_files) + 1

                filename = f"{folder_path}/{current_class}_{file_counter:02d}.csv"
                current_file = open(filename, 'w')
                current_file.write("timestamp,value\n")

                sample_counter = 0
                start_time = time.time()

                print(f"[FILE] Recording started: {filename}")
                continue

            # ----------------------------------------------
            # 2. End of recording
            # ----------------------------------------------
            if line == "END":
                if current_file:
                    current_file.close()

                    duration = time.time() - start_time if start_time else 0

                    print(f"[SAVE] Saved: {current_file.name}")
                    print(f"[DATA] Samples: {sample_counter}")
                    print(f"[TIME] Duration: {duration:.1f} seconds")
                    print("-" * 50)

                    current_file = None
                continue

            # ----------------------------------------------
            # 3. Write data and count samples
            # ----------------------------------------------
            if current_file is not None and "," in line:
                current_file.write(line + "\n")
                sample_counter += 1

        except KeyboardInterrupt:
            print("\n[STOP] Program stopped by user.")
            if current_file:
                current_file.close()
                print("[SAVE] Last file saved before exit.")
            break
        except Exception as e:
            print(f"[WARN] Error: {e}")
            continue


if __name__ == "__main__":
    main()

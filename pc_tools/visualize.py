import pandas as pd
import matplotlib.pyplot as plt
import os
import glob

FOLDER = "dataset"
CLASSES = ["SLOW", "FAST", "NO_CAR"]

def find_first_csv(class_name):
    folder_path = os.path.join(FOLDER, class_name)
    if not os.path.exists(folder_path):
        return None
    csv_files = glob.glob(os.path.join(folder_path, "*.csv"))
    if not csv_files:
        return None
    csv_files.sort()
    return csv_files[0]

def plot_files():
    plt.figure(figsize=(12, 6))
    plotted_count = 0
    
    for class_name in CLASSES:
        file_path = find_first_csv(class_name)
        
        if file_path is None:
            print(f"[WARNING] No CSV file found in folder: {class_name} - Skipping")
            continue
        
        try:
            df = pd.read_csv(file_path)
            
            if 'timestamp' not in df.columns or 'value' not in df.columns:
                print(f"[WARNING] {file_path} missing required columns (timestamp, value)")
                continue
            
            label = os.path.basename(os.path.dirname(file_path)) + " - " + os.path.basename(file_path)
            plt.plot(df['timestamp'], df['value'], label=label, linewidth=2)
            plotted_count += 1
            print(f"[OK] Plotted: {file_path}")
            
        except Exception as e:
            print(f"[ERROR] Could not read {file_path}: {e}")
            continue

    if plotted_count == 0:
        print("[ERROR] No files plotted. Please check your dataset folder.")
        return

    plt.xlabel("Time (ms)", fontsize=12)
    plt.ylabel("Hall Sensor Value (ADC)", fontsize=12)
    plt.title("Magnetic Signal Comparison (SLOW vs FAST vs NO_CAR)", fontsize=14)
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    plot_files()

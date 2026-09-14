# Magnetic Vision: Teaching AI to See Motion Without a Camera

A TinyML project using a Hall-effect sensor and Edge Impulse to classify
motion speed and patterns — no camera required. Runs entirely on-device
on a Seeed Wio Terminal.

---

## Experiment 1: Speed Classification

### Classes
- **FAST**: Car moves rapidly past the sensor.
- **SLOW**: Car moves slowly past the sensor.
- **NO_CAR**: No car present near the sensor.

### Processing
- **Window size**: 6000 ms (6 seconds at 100 Hz)
- **Processing block**: Flatten (extracts 7 statistical features)
- **Learning block**: Classification (Keras)

### Features Extracted by Flatten
- Mean
- Standard Deviation
- Minimum
- Maximum
- RMS
- Skewness
- Kurtosis

### Neural Network
- Input: 7 features
- Dense (20 neurons, ReLU) → Dense (10 neurons, ReLU) → Dropout (0.2)
- Output: 3 classes, Softmax

### Results
- Validation accuracy: **100%**
- Test accuracy: **100%**
- Dataset: 10 recordings per class (30 total) → 90 windows

---

## Experiment 2: Motion Pattern Classification

### Classes
- **STRAIGHT**: Steady movement in one direction.
- **PAUSE**: Movement, brief pause, then continuation.
- **OSCILLATE**: Back-and-forth movement within the window.

### Processing
- **Window size**: 3000 ms (3 seconds at 100 Hz)
- **Processing block**: Raw Data (full 300-sample signal)
- **Learning block**: Classification (Keras with 1D CNN)

### Neural Network
- Input: 300 raw samples → Reshape (300 × 1)
- 1D Conv/Pool (4 filters, kernel 3) → 1D Conv/Pool (16 filters, kernel 3)
- Flatten → Dense (20, ReLU) → Dropout (0.2)
- Output: 3 classes, Softmax

### Results
- Validation accuracy: **76.3%**
- Test accuracy: **66.67%**
- Dataset: 28–29 recordings per class (86 total)

**Finding**: Flatten was tested first and reached only 61.5%, confirming that
amplitude-based statistics are insufficient for temporal pattern recognition.

---

## How to Deploy

### Prerequisites

1. **Arduino IDE** (v1.8.19 recommended for stability)
2. **Seeed SAMD board package** installed
3. **TFT_eSPI** library
4. **Edge Impulse Arduino library** (specific to your impulse)

### Step 1: Install the Board

1. Open Arduino IDE.
2. Go to `File → Preferences`.
3. Add this URL to "Additional Boards Manager URLs":
```
   https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
```
4. Go to `Tools → Board → Board Manager`.
5. Search for "Seeed SAMD" and install it.
6. Select `Tools → Board → Seeeduino Wio Terminal`.

### Step 2: Install Libraries

1. Install `TFT_eSPI` via `Sketch → Include Library → Manage Libraries`.
2. Download the Edge Impulse Arduino library from your project's
**Deployment** page on Edge Impulse Studio.
3. Install it via `Sketch → Include Library → Add .ZIP Library...`

### Step 3: Upload the Firmware

1. Open the desired `.ino` file from `firmware/`.
2. Connect the Wio Terminal via USB.
3. Select the correct port in `Tools → Port`.
4. Press **Upload**.

### Step 4: Run the Project

1. Open **Serial Monitor** at **9600 baud**.
2. Press **B1** on the Wio Terminal to start recording.
3. Move the magnet/car during the recording window.
4. The result will appear on the Wio Terminal's screen.

### Step 5: PC Logger (Optional)

If you want to collect your own dataset:

1. Install Python 3 and `pyserial`:
```bash
pip install pyserial
````
2. Edit SERIAL_PORT in pc_tools/logger.py to match your COM port.
3. Run:
python pc_tools/logger.py
4. Press B1 on the Wio Terminal; the data will be saved automatically
to dataset/<CLASS>/.

## Repository Structure

firmware/        Arduino sketches (data collection + inference)
pc_tools/        Python logger for dataset collection

## Full Write-up

```bash
https://www.hackster.io/aula-jazmati/magnetic-vision-teaching-ai-to-see-motion-without-a-camera-e76650
````

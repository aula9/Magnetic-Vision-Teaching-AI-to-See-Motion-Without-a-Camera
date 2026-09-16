# Magnetic Vision: Teaching AI to See Motion Without a Camera

A TinyML project using a Hall-effect sensor and Edge Impulse to classify
motion speed and patterns — no camera required. Runs entirely on-device
on a Seeed Wio Terminal.

![Wio Terminal + Edge Impulse](images/wio_edgeimpulse.png)

![Two components. Zero cameras.](images/no_camera_drama.png)
---

## Table of Contents

1. [Overview](#overview)
2. [Hardware Setup](#hardware-setup)
3. [Experiment 1: Speed Classification](#experiment-1-speed-classification)
4. [Experiment 2: Motion Pattern Classification](#experiment-2-motion-pattern-classification)
5. [Video Demos](#video-demos)
6. [How to Deploy](#how-to-deploy)
7. [Repository Structure](#repository-structure)
8. [Results Summary](#results-summary)
9. [Future Work](#future-work)
10. [Credits](#credits)
11. [Links](#links)

---

## Overview

This project explores whether a simple **Hall-effect sensor** can capture
enough information about a moving object for a machine-learning model to
recognize its motion — **without any camera or computer vision**.

![TinyML System Architecture](images/tinyml_architecture.jpg)

It contains two experiments:
- **Experiment 1**: Classify motion **speed** (FAST / SLOW / NO_CAR) —
  achieved 100% accuracy using the Flatten processing block.
- **Experiment 2**: Classify motion **patterns** (STRAIGHT / OSCILLATE / PAUSE) —
  achieved 76.3% validation accuracy using a 1D CNN on raw signals.


**Key finding**: Statistical features (Flatten) work well for amplitude-based
problems, but temporal patterns require more sophisticated modeling (1D CNN).

---
## Hardware Setup

### Components

| Component | Quantity | Notes |
|---|---|---|
| Seeed Studio Wio Terminal | 1 | ARM Cortex-M4F, built-in LCD |
| A1302 Linear Hall-Effect Sensor | 1 | Analog output |
| Magnetic toy car | 1 | Any magnet works |
| Jumper wires | 3 | For sensor connection |

![Wio Terminal ports](images/wio_ports.jpg)
![Wio Terminal buttons](images/wio_buttons.jpg)
![Magnetic Power Kit](images/magnetic_kit.png)

![A1302 Sensor](images/a1302_sensor.jpg)

![A1302 Sensor Pins](images/sensor_pins.png)
![Wiring setup](images/wiring_diagram.jpg)

### Wiring Diagram

| A1302 Pin | Wio Terminal Pin | Notes |
|---|---|---|
| VCC | 3.3V | Wider ADC dynamic range |
| GND | GND | Common ground |
| VOUT | A0 (Pin 13) | Analog magnetic field strength |

---

## Experiment 1: Speed Classification

### Classes
- **FAST**: Car moves rapidly past the sensor.
- **SLOW**: Car moves slowly past the sensor.
- **NO_CAR**: No car present near the sensor.

### Data Collection

![Python Logger Console](images/pc_logger.png)

### Impulse Design

- **Window size**: 6000 ms (6 seconds at 100 Hz)
- **Window increase**: 3000 ms
- **Processing block**: Flatten (extracts 7 statistical features)
- **Learning block**: Classification (Keras)

![Impulse Design](images/exp1_impulse_design.png)

### Signal Examples

![FAST raw signal](images/exp1_raw_signal_fast.png)
![NO_CAR signal](images/exp1_dataset_no_car.png)
![SLOW signal](images/exp1_dataset_slow.png)

### Signal Comparison

![Signal Comparison](images/exp1_signals_comparison.png)

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

![Training Results](images/exp1_training_results.png)
![Test Results](images/exp1_test_results.png)


### Deployment

![Deployment Success](images/deployment_success.png)

![Data collection - FAST class](images/exp1_data_collection.jpg)
![Inference - NO_CAR](images/exp1_inference_no_car.jpg)
![Inference - SLOW](images/exp1_inference_slow.jpg)

---

## Experiment 2: Motion Pattern Classification

### Classes
- **STRAIGHT**: Steady movement in one direction.
- **PAUSE**: Movement, brief pause, then continuation.
- **OSCILLATE**: Back-and-forth movement within the window.
  
### Data Collection

![Signal grid - all classes](images/exp2_signals_comparison.png)

Each recording lasts 3 seconds (300 samples at 100 Hz). Below are 
examples of each class:

![STRAIGHT signal](images/exp2_signal_straight.png)
![PAUSE signal](images/exp2_signal_pause.png)


The dataset consists of 86 recordings split across three classes:
![Dataset overview](images/exp2_dataset_overview1.png)
![Dataset overview](images/exp2_dataset_overview.png)
![OSCILLATE signal](images/exp2_signal_oscillate.png)

### Processing
- **Window size**: 3000 ms (3 seconds at 100 Hz)
- **Processing block**: Raw Data (full 300-sample signal)
- **Learning block**: Classification (Keras with 1D CNN)
  
  ![Impulse design](images/exp2_impulse_design.png)
  
### Data Explorer

The feature explorer shows how the three classes cluster after processing:

![Data explorer](images/exp2_data_explorer.png)

### Neural Network

![Neural network architecture](images/exp2_architecture.png)

- Input: 300 raw samples → Reshape (300 × 1)
- 1D Conv/Pool (4 filters, kernel 3) → 1D Conv/Pool (16 filters, kernel 3)
- Flatten → Dense (20, ReLU) → Dropout (0.2)
- Output: 3 classes, Softmax

### Results
![Training results](images/exp2_training_results.png)

- Validation accuracy: **76.3%**
- Test accuracy: **66.67%**
- Dataset: 28–29 recordings per class (86 total)

  ![Test results](images/exp2_test_results.png)

**Finding**: Flatten was tested first and reached only 61.5%, confirming that
amplitude-based statistics are insufficient for temporal pattern recognition.

### Deployment

The model was deployed as an Arduino library using the **EON Compiler** 
for optimized on-device performance.

![Deployment](images/exp2_deployment.png)

**On-device performance:**
- Inference latency: **5 ms**
- Peak RAM usage: **13.8 KB**
- Flash usage: **68.5 KB**

![On-device performance](images/exp2_on_device_perf.png)


![Data collection - STRAIGHT](images/exp2_data_collection_straight.jpg)
![Data collection - OSCILLATE](images/exp2_data_collection_oscillate.jpg)
![Inference - PAUSE](images/exp2_inference_pause.jpg)
![Setup photo](images/exp2_inference_setup.jpg)

---
## Video Demos

**Experiment 1: Speed Classification (FAST / SLOW / NO_CAR)**
- [Live demo — take 1](https://www.youtube.com/shorts/AFlcssnsRv8?feature=share)
- [Live demo — take 2](https://www.youtube.com/shorts/ThNNQYDxOLE?feature=share)
- [FAST detection](https://www.youtube.com/shorts/ysYRykSbt78?feature=share)
- [SLOW / NO_CAR detection](https://www.youtube.com/shorts/SpL6P7tqWLs?feature=share)

**Experiment 2: Motion Pattern Classification (STRAIGHT / OSCILLATE / PAUSE)**
- [Live demo](https://www.youtube.com/shorts/MMltPG_8UW0?feature=share)

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

![Magnetic toy car](images/cover.jpg)

### Step 5: PC Logger (Optional)

If you want to collect your own dataset:

1. Install Python 3 and `pyserial`:
   ```bash
   pip install pyserial
   ```
2. Edit SERIAL_PORT in pc_tools/logger.py to match your COM port.
3. Run:
python pc_tools/logger.py
4. Press B1 on the Wio Terminal; the data will be saved automatically
to dataset/<CLASS>/.


## Repository Structure
```
Magnetic-Vision-Teaching-AI-to-See-Motion-Without-a-Camera/
│
├── firmware/              Arduino sketches
│   ├── experiment1_speed/      (FAST / SLOW / NO_CAR)
│   └── experiment2_patterns/   (STRAIGHT / OSCILLATE / PAUSE)
│
├── pc_tools/              Python scripts
│   ├── logger.py               (receives CSV data via Serial)
│   └── visualize.py            (plots time-series signals)
│
├── dataset/               Collected CSV recordings (organized by class)
│
├── images/                Project screenshots and diagrams
│
└── README.md              Project documentation
```

## Results Summary

| Experiment | Task | Method | Validation | Test |
|---|---|---|---|---|
| 1 | Speed classification | Flatten + Dense | 100% | 100% |
| 2 | Pattern classification | 1D CNN on Raw Data | 76.3% | 66.67% |

## Future Work

- **Multi-axis sensing**: Use 3-axis magnetometers to capture richer magnetic signatures.
- **Directional detection**: Add a second Hall-effect sensor to detect motion direction.
- **Larger datasets**: Vary magnet size, orientation, speed, and distance to test generalization.
  
## Credits

**Author**: Aula Jazmati  
**Platform**: Seeed Studio Wio Terminal  
**ML Pipeline**: Edge Impulse  
**License**: MIT

## Full Write-up

[View the complete project on Hackster](https://www.hackster.io/aula-jazmati/magnetic-vision-teaching-ai-to-see-motion-without-a-camera-e76650)


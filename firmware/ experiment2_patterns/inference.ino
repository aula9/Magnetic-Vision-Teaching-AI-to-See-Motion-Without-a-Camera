/*
 * motion_pattern_inference.ino
 *
 * Wio Terminal firmware for on-device inference of magnetic motion patterns.
 * Records 300 samples (3 seconds at 100 Hz), then runs the Edge Impulse
 * impulse locally and displays the result on screen.
 *
 * Classes: STRAIGHT, OSCILLATE, PAUSE
 * Buttons: B1 = Start Recording, B2 = Test Data, B3 = Reset
 */

#include <TFT_eSPI.h>
#include <SPI.h>
#include <Magnetic_Vision_inferencing.h>

// ============================================================
// Configuration
// ============================================================
#define HALL_PIN A0
#define SAMPLE_RATE_HZ 100
#define SAMPLE_INTERVAL_US (1000000UL / SAMPLE_RATE_HZ)
#define TOTAL_SAMPLES EI_CLASSIFIER_RAW_SAMPLE_COUNT

#define BTN_1 WIO_KEY_A
#define BTN_2 WIO_KEY_B
#define BTN_3 WIO_KEY_C
#define BUZZER_PIN WIO_BUZZER

#define SERIAL_BAUD 9600

// ============================================================
// UI Colors
// ============================================================
#define COLOR_BG TFT_NAVY
#define COLOR_TITLE TFT_CYAN
#define COLOR_LABEL TFT_WHITE
#define COLOR_VALUE TFT_YELLOW
#define COLOR_BORDER TFT_DARKGREY
#define COLOR_READY TFT_GREEN
#define COLOR_RECORD TFT_RED
#define COLOR_BUTTON TFT_ORANGE
#define COLOR_BOX_BG TFT_BLACK
#define COLOR_BOX_BORDER TFT_BLUE

TFT_eSPI tft = TFT_eSPI();

// ============================================================
// Edge Impulse Inference Buffer
// ============================================================
static float* inference_buffer = nullptr;
static size_t inference_buffer_len = 0;

int inference_get_data(size_t offset, size_t length, float *out_ptr) {
    if (offset + length > inference_buffer_len) {
        return -1;
    }
    memcpy(out_ptr, inference_buffer + offset, length * sizeof(float));
    return 0;
}

// ============================================================
// Sensor Manager
// ============================================================
class SensorManager {
private:
    int rawValue;
    unsigned long lastSampleMicros;
    float sampleBuffer[TOTAL_SAMPLES];
    int sampleIndex;
    bool isCollecting;

public:
    SensorManager()
        : rawValue(0), lastSampleMicros(0), sampleIndex(0), isCollecting(false) {}

    void begin() {
        pinMode(HALL_PIN, INPUT);
        analogReadResolution(12);
        rawValue = analogRead(HALL_PIN);
        lastSampleMicros = micros();
    }

    bool update() {
        unsigned long now = micros();
        if ((unsigned long)(now - lastSampleMicros) >= SAMPLE_INTERVAL_US) {
            lastSampleMicros += SAMPLE_INTERVAL_US;
            rawValue = analogRead(HALL_PIN);

            if (isCollecting && sampleIndex < TOTAL_SAMPLES) {
                sampleBuffer[sampleIndex++] = (float)rawValue;
                if (sampleIndex >= TOTAL_SAMPLES) {
                    isCollecting = false;
                }
            }
            return true;
        }
        return false;
    }

    void startCollection() {
        sampleIndex = 0;
        isCollecting = true;
        lastSampleMicros = micros();
    }

    void stopCollection() {
        isCollecting = false;
    }

    bool isCollectingSamples() const { return isCollecting; }
    bool isComplete() const { return sampleIndex >= TOTAL_SAMPLES; }
    int getRaw() const { return rawValue; }
    float* getSampleBuffer() { return sampleBuffer; }
    int getSampleCount() const { return sampleIndex; }
};

// ============================================================
// Button Manager (Fixed Debounce Logic)
// ============================================================
class ButtonManager {
private:
    bool stable1, stable2, stable3;
    bool raw1, raw2, raw3;
    unsigned long debounce1, debounce2, debounce3;
    const unsigned long debounceDelay = 50;

public:
    ButtonManager()
        : stable1(HIGH), stable2(HIGH), stable3(HIGH),
          raw1(HIGH), raw2(HIGH), raw3(HIGH),
          debounce1(0), debounce2(0), debounce3(0) {}

    void begin() {
        pinMode(BTN_1, INPUT_PULLUP);
        pinMode(BTN_2, INPUT_PULLUP);
        pinMode(BTN_3, INPUT_PULLUP);
    }

    bool pressed1() { return check(BTN_1, stable1, raw1, debounce1); }
    bool pressed2() { return check(BTN_2, stable2, raw2, debounce2); }
    bool pressed3() { return check(BTN_3, stable3, raw3, debounce3); }

private:
    bool check(int pin, bool &stableState, bool &lastRaw,
               unsigned long &lastDebounce) {
        bool reading = digitalRead(pin);
        bool event = false;

        if (reading != lastRaw) {
            lastDebounce = millis();
            lastRaw = reading;
        }

        if ((millis() - lastDebounce) > debounceDelay) {
            if (reading != stableState) {
                stableState = reading;
                if (stableState == LOW) {
                    event = true;
                }
            }
        }

        return event;
    }
};

// ============================================================
// Buzzer Manager
// ============================================================
class BuzzerManager {
public:
    void begin() { pinMode(BUZZER_PIN, OUTPUT); }
    void beep(int frequency, int duration) {
        tone(BUZZER_PIN, frequency, duration);
        delay(duration);
    }
};

// ============================================================
// UI Manager
// ============================================================
class UIManager {
private:
    TFT_eSprite spr;
    int lastRawValue;
    String inferenceResult;
    float confidence;
    bool isRecording;
    unsigned long lastUIUpdate;
    const unsigned long UI_INTERVAL = 100;

public:
    UIManager()
        : spr(&tft), lastRawValue(0), inferenceResult("--"),
          confidence(0.0), isRecording(false), lastUIUpdate(0) {}

    void begin() {
        tft.init();
        tft.setRotation(3);
        spr.createSprite(320, 240);
        draw();
    }

    void update(int raw, bool recording, const String &result,
                float conf, bool force = false) {
        lastRawValue = raw;
        isRecording = recording;
        inferenceResult = result;
        confidence = conf;

        unsigned long now = millis();
        if (!force && (now - lastUIUpdate < UI_INTERVAL)) {
            return;
        }
        lastUIUpdate = now;
        draw();
    }

private:
    void draw() {
        spr.fillSprite(COLOR_BG);

        // Title
        spr.setTextColor(COLOR_TITLE);
        spr.setTextFont(2);
        spr.setTextSize(2);
        spr.drawString("Motion Pattern AI", 10, 5);
        spr.drawLine(5, 35, 315, 35, TFT_DARKGREY);

        // Result
        spr.setTextFont(2);
        spr.setTextSize(1);
        spr.setTextColor(COLOR_LABEL);
        spr.drawString("Result", 20, 50);
        spr.drawRoundRect(110, 45, 180, 28, 4, COLOR_BOX_BORDER);
        spr.fillRoundRect(111, 46, 178, 26, 4, COLOR_BOX_BG);
        spr.setTextColor(COLOR_VALUE);
        spr.setTextSize(2);
        spr.drawString(inferenceResult, 120, 52);

        // Confidence
        spr.setTextFont(2);
        spr.setTextSize(1);
        spr.setTextColor(COLOR_LABEL);
        spr.drawString("Confidence", 20, 90);
        spr.drawRoundRect(110, 85, 150, 28, 4, COLOR_BOX_BORDER);
        spr.fillRoundRect(111, 86, 148, 26, 4, COLOR_BOX_BG);
        spr.setTextColor(COLOR_VALUE);
        spr.setTextSize(2);
        char confStr[10];
        sprintf(confStr, "%d%%", (int)(confidence * 100));
        spr.drawString(confStr, 120, 92);

        // Hall value
        spr.setTextFont(2);
        spr.setTextSize(1);
        spr.setTextColor(COLOR_LABEL);
        spr.drawString("Hall", 20, 130);
        spr.drawRoundRect(110, 125, 90, 28, 4, COLOR_BOX_BORDER);
        spr.fillRoundRect(111, 126, 88, 26, 4, COLOR_BOX_BG);
        spr.setTextColor(COLOR_VALUE);
        spr.setTextSize(2);
        char hallStr[10];
        sprintf(hallStr, "%4d", lastRawValue);
        spr.drawString(hallStr, 120, 132);

        // Status
        spr.setTextFont(2);
        spr.setTextSize(1);
        spr.setTextColor(COLOR_LABEL);
        spr.drawString("Status", 20, 175);
        uint16_t statusColor = isRecording ? COLOR_RECORD : COLOR_READY;
        spr.setTextColor(statusColor);
        spr.setTextSize(2);
        String statusText = isRecording ? "RECORDING" : "READY";
        spr.drawString(statusText, 110, 175);

        // Buttons
        spr.setTextFont(2);
        spr.setTextSize(1);
        spr.setTextColor(COLOR_BUTTON);
        spr.drawString("B1 START", 5, 215);
        spr.drawString("B2 TEST", 105, 215);
        spr.drawString("B3 RESET", 220, 215);

        spr.drawRect(2, 2, 316, 236, COLOR_BORDER);
        spr.pushSprite(0, 0);
    }
};

// ============================================================
// Global Objects
// ============================================================
SensorManager sensorManager;
ButtonManager buttonManager;
BuzzerManager buzzerManager;
UIManager uiManager;

String currentResult = "READY";
float currentConfidence = 0.0;
bool inferenceDone = false;

// ============================================================
// Setup
// ============================================================
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(100);

    sensorManager.begin();
    buttonManager.begin();
    buzzerManager.begin();
    uiManager.begin();

    uiManager.update(sensorManager.getRaw(), false, "READY", 0.0, true);

    Serial.println("================================");
    Serial.println("Motion Pattern Detector");
    Serial.println("Classes: STRAIGHT / OSCILLATE / PAUSE");
    Serial.print("Samples per recording: ");
    Serial.println(TOTAL_SAMPLES);
    Serial.println("================================");
}

// ============================================================
// Loop
// ============================================================
void loop() {
    bool newSample = sensorManager.update();

    // --- B1: Start Recording ---
    if (buttonManager.pressed1()) {
        Serial.println("[BTN] B1 pressed");
        if (!sensorManager.isCollectingSamples()) {
            inferenceDone = false;
            currentResult = "RECORDING...";
            currentConfidence = 0.0;

            sensorManager.startCollection();
            buzzerManager.beep(1000, 100);

            uiManager.update(sensorManager.getRaw(), true,
                             "RECORDING...", 0.0, true);

            Serial.println("Recording started...");
        }
    }

    // --- B2: Test ---
    if (buttonManager.pressed2()) {
        Serial.println("[BTN] B2 pressed");
        Serial.println("=== SENSOR TEST ===");
        Serial.print("Hall value: ");
        Serial.println(sensorManager.getRaw());
        buzzerManager.beep(1500, 100);
    }

    // --- B3: Reset ---
    if (buttonManager.pressed3()) {
        Serial.println("[BTN] B3 pressed");
        sensorManager.stopCollection();
        inferenceDone = false;
        currentResult = "READY";
        currentConfidence = 0.0;

        uiManager.update(sensorManager.getRaw(), false,
                         "READY", 0.0, true);

        buzzerManager.beep(600, 50);
        Serial.println("Reset");
    }

    // --- Update UI while recording ---
    if (sensorManager.isCollectingSamples() && newSample) {
        uiManager.update(sensorManager.getRaw(), true,
                         "RECORDING...", 0.0);
    }

    // --- Inference when recording completes ---
    if (sensorManager.isComplete() && !inferenceDone) {
        inferenceDone = true;
        sensorManager.stopCollection();

        int raw = sensorManager.getRaw();
        uiManager.update(raw, false, "PROCESSING", 0.0, true);

        inference_buffer = sensorManager.getSampleBuffer();
        inference_buffer_len = sensorManager.getSampleCount();

        signal_t features_signal;
        features_signal.total_length = inference_buffer_len;
        features_signal.get_data = std::function<int(size_t, size_t, float*)>(
            &inference_get_data
        );

        ei_impulse_result_t result = { 0 };
        EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);

        if (res != EI_IMPULSE_OK) {
            currentResult = "ERROR";
            currentConfidence = 0.0;
            Serial.print("Classifier error: ");
            Serial.println(res);
            uiManager.update(raw, false, "ERROR", 0.0, true);
        } else {
            float maxConf = 0.0;
            int maxIdx = -1;

            Serial.println("Classification:");
            for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
                Serial.print("  ");
                Serial.print(result.classification[i].label);
                Serial.print(" = ");
                Serial.println(result.classification[i].value, 4);

                if (result.classification[i].value > maxConf) {
                    maxConf = result.classification[i].value;
                    maxIdx = i;
                }
            }

            if (maxIdx >= 0) {
                currentResult = String(result.classification[maxIdx].label);
                currentConfidence = maxConf;
            } else {
                currentResult = "UNKNOWN";
                currentConfidence = 0.0;
            }

            uiManager.update(raw, false, currentResult, currentConfidence, true);

            Serial.print("RESULT: ");
            Serial.print(currentResult);
            Serial.print("  Confidence: ");
            Serial.print(currentConfidence * 100.0);
            Serial.println("%");

            buzzerManager.beep(2000, 200);
        }

        Serial.println("================================");
    }

    // --- Idle UI update ---
    if (!sensorManager.isCollectingSamples() && newSample) {
        uiManager.update(sensorManager.getRaw(), false,
                         currentResult, currentConfidence);
    }

    delayMicroseconds(100);
}

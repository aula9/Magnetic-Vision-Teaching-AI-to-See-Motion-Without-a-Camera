#include <TFT_eSPI.h>
#include <SPI.h>
#include <Magnetic_Vision_1_inferencing.h>

#define HALL_PIN A0
#define SAMPLE_RATE_HZ 100
#define SAMPLE_INTERVAL_MS (1000 / SAMPLE_RATE_HZ)
#define TOTAL_SAMPLES EI_CLASSIFIER_RAW_SAMPLE_COUNT

#define BTN_1 WIO_KEY_A
#define BTN_2 WIO_KEY_B
#define BTN_3 WIO_KEY_C
#define BUZZER_PIN WIO_BUZZER

#define SERIAL_BAUD 9600

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

static float* inference_buffer = nullptr;
static size_t inference_buffer_len = 0;

int inference_get_data(size_t offset, size_t length, float *out_ptr) {
    if (offset + length > inference_buffer_len) {
        return -1;
    }
    memcpy(out_ptr, inference_buffer + offset, length * sizeof(float));
    return 0;
}

class SensorManager {
private:
    int rawValue;
    unsigned long lastReadTime;
    float sampleBuffer[TOTAL_SAMPLES];
    int sampleIndex;
    bool isCollecting;

public:
    SensorManager() : rawValue(0), lastReadTime(0), sampleIndex(0), isCollecting(false) {}

    void begin() {
        pinMode(HALL_PIN, INPUT);
        analogReadResolution(12);
    }

    void startCollection() {
        sampleIndex = 0;
        isCollecting = true;
    }

    void stopCollection() {
        isCollecting = false;
    }

    bool isCollectingSamples() {
        return isCollecting;
    }

    bool update() {
        unsigned long now = millis();
        if (now - lastReadTime >= SAMPLE_INTERVAL_MS) {
            rawValue = analogRead(HALL_PIN);
            lastReadTime = now;
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

    int getRaw() const {
        return rawValue;
    }

    float* getSampleBuffer() {
        return sampleBuffer;
    }

    int getSampleCount() const {
        return sampleIndex;
    }

    bool isComplete() const {
        return sampleIndex >= TOTAL_SAMPLES;
    }
};

class ButtonManager {
private:
    bool last1, last2, last3;
    unsigned long debounce1, debounce2, debounce3;
    const unsigned long debounceDelay = 50;

public:
    ButtonManager() : last1(HIGH), last2(HIGH), last3(HIGH),
                       debounce1(0), debounce2(0), debounce3(0) {}

    void begin() {
        pinMode(BTN_1, INPUT_PULLUP);
        pinMode(BTN_2, INPUT_PULLUP);
        pinMode(BTN_3, INPUT_PULLUP);
    }

    bool pressed1() {
        return pressed(BTN_1, last1, debounce1);
    }

    bool pressed2() {
        return pressed(BTN_2, last2, debounce2);
    }

    bool pressed3() {
        return pressed(BTN_3, last3, debounce3);
    }

private:
    bool pressed(int pin, bool &last, unsigned long &debounce) {
        bool reading = digitalRead(pin);
        if (reading != last) {
            debounce = millis();
            last = reading;
            if (reading == LOW) {
                return true;
            }
        }
        return false;
    }
};

class BuzzerManager {
public:
    void begin() {
        pinMode(BUZZER_PIN, OUTPUT);
    }

    void beep(int frequency, int duration) {
        tone(BUZZER_PIN, frequency, duration);
        delay(duration);
    }
};

class UIManager {
private:
    TFT_eSprite spr;
    int lastRawValue;
    String inferenceResult;
    float confidence;
    bool isRecording;

public:
    UIManager() : spr(&tft), lastRawValue(0), inferenceResult("--"),
                   confidence(0.0), isRecording(false) {}

    void begin() {
        tft.init();
        tft.setRotation(3);
        spr.createSprite(320, 240);
        spr.fillSprite(COLOR_BG);
        spr.pushSprite(0, 0);
    }

    void update(int raw, bool recording, const String &result = "", float conf = 0.0) {
        lastRawValue = raw;
        isRecording = recording;
        if (result.length() > 0) {
            inferenceResult = result;
            confidence = conf;
        }

        spr.fillSprite(COLOR_BG);

        spr.setTextColor(COLOR_TITLE);
        spr.setTextFont(2);
        spr.setTextSize(2);
        spr.drawString("AI Motion Detector", 10, 5);
        spr.drawLine(5, 35, 315, 35, TFT_DARKGREY);

        spr.setTextFont(2);
        spr.setTextSize(1);
        spr.setTextColor(COLOR_LABEL);
        spr.drawString("Result", 20, 50);
        spr.drawRoundRect(110, 45, 150, 28, 4, COLOR_BOX_BORDER);
        spr.fillRoundRect(111, 46, 148, 26, 4, COLOR_BOX_BG);
        spr.setTextColor(COLOR_VALUE);
        spr.setTextSize(2);
        spr.drawString(inferenceResult, 120, 52);

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

        spr.setTextFont(2);
        spr.setTextSize(1);
        spr.setTextColor(COLOR_LABEL);
        spr.drawString("Status", 20, 175);
        uint16_t statusColor = isRecording ? COLOR_RECORD : COLOR_READY;
        spr.setTextColor(statusColor);
        spr.setTextSize(2);
        String statusText = isRecording ? "RECORDING" : "READY";
        spr.drawString(statusText, 110, 175);

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

SensorManager sensorManager;
ButtonManager buttonManager;
BuzzerManager buzzerManager;
UIManager uiManager;

bool isRecording = false;
String currentResult = "--";
float currentConfidence = 0.0;

bool waitingForRandomDelay = false;
unsigned long randomDelayStart = 0;
unsigned long randomDelayDuration = 0;

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(100);

    sensorManager.begin();
    buttonManager.begin();
    buzzerManager.begin();
    uiManager.begin();

    uiManager.update(0, false, "READY", 0.0);
    Serial.println("AI Motion Detector ready. Press B1 to start.");
    Serial.print("Expecting ");
    Serial.print(TOTAL_SAMPLES);
    Serial.println(" samples per inference.");
    Serial.print("Window duration: ");
    Serial.print(TOTAL_SAMPLES * 1000 / SAMPLE_RATE_HZ);
    Serial.println(" ms");
}

void loop() {
    if (buttonManager.pressed1()) {
        if (!isRecording && !waitingForRandomDelay && !sensorManager.isCollectingSamples()) {
            randomDelayDuration = random(300, 501);
            randomDelayStart = millis();
            waitingForRandomDelay = true;
            uiManager.update(sensorManager.getRaw(), false, "WAITING...", 0.0);
            Serial.print("Waiting random delay: ");
            Serial.print(randomDelayDuration);
            Serial.println(" ms");
            buzzerManager.beep(800, 50);
        }
    }

    if (waitingForRandomDelay) {
        if (millis() - randomDelayStart >= randomDelayDuration) {
            waitingForRandomDelay = false;
            isRecording = true;
            sensorManager.startCollection();
            buzzerManager.beep(1000, 100);
            uiManager.update(sensorManager.getRaw(), true, "RECORDING...", 0.0);
            Serial.println("Recording started...");
        }
    }

    if (buttonManager.pressed2()) {
        Serial.println("=== TEST DATA ===");
        for (int i = 0; i < 10; i++) {
            int fakeValue = 2000 + random(0, 500);
            Serial.print(i * 10);
            Serial.print(",");
            Serial.println(fakeValue);
            delay(20);
        }
        Serial.println("=== END TEST ===");
        buzzerManager.beep(1500, 100);
    }

    if (buttonManager.pressed3()) {
        isRecording = false;
        waitingForRandomDelay = false;
        sensorManager.stopCollection();
        currentResult = "--";
        currentConfidence = 0.0;
        uiManager.update(sensorManager.getRaw(), false, "RESET", 0.0);
        buzzerManager.beep(600, 50);
        Serial.println("Reset");
    }

    if (sensorManager.update()) {
        int raw = sensorManager.getRaw();

        if (isRecording) {
            uiManager.update(raw, true, "RECORDING...", 0.0);
        }

        if (sensorManager.isComplete() && isRecording) {
            isRecording = false;
            sensorManager.stopCollection();

            inference_buffer = sensorManager.getSampleBuffer();
            inference_buffer_len = sensorManager.getSampleCount();

            signal_t features_signal;
            features_signal.total_length = inference_buffer_len;
            features_signal.get_data = &inference_get_data;

            ei_impulse_result_t result = { 0 };
            EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);

            if (res != EI_IMPULSE_OK) {
                currentResult = "ERR";
                currentConfidence = 0.0;
                uiManager.update(raw, false, "ERROR", 0.0);
                Serial.print("Classifier error code: ");
                Serial.println(res);
            } else {
                Serial.println("=== Classification Results ===");
                for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
                    Serial.print("  ");
                    Serial.print(result.classification[i].label);
                    Serial.print(": ");
                    Serial.print(result.classification[i].value * 100.0);
                    Serial.println("%");
                }

                float maxConf = 0.0;
                int maxIdx = -1;
                for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
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

                uiManager.update(raw, false, currentResult, currentConfidence);
                buzzerManager.beep(2000, 200);
                Serial.print("Final inference: ");
                Serial.print(currentResult);
                Serial.print(" (Confidence: ");
                Serial.print(currentConfidence * 100.0);
                Serial.println("%)");
            }
        }

        if (!isRecording && !waitingForRandomDelay && !sensorManager.isCollectingSamples()) {
            uiManager.update(raw, false, currentResult, currentConfidence);
        }
    }

    delay(1);
}

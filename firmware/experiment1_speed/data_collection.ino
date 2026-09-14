#include <TFT_eSPI.h>
#include <SPI.h>

#define HALL_PIN           A0
#define SAMPLE_RATE_HZ     100
#define SAMPLE_INTERVAL_MS (1000 / SAMPLE_RATE_HZ)
#define RECORD_DURATION_MS 3000
#define TOTAL_SAMPLES      (RECORD_DURATION_MS / SAMPLE_INTERVAL_MS)

#define BTN_1              WIO_KEY_A
#define BTN_2              WIO_KEY_B
#define BTN_3              WIO_KEY_C
#define BUZZER_PIN         WIO_BUZZER

#define SERIAL_BAUD        9600

#define COLOR_BG           TFT_NAVY
#define COLOR_TITLE        TFT_CYAN
#define COLOR_LABEL        TFT_WHITE
#define COLOR_VALUE        TFT_YELLOW
#define COLOR_BORDER       TFT_DARKGREY
#define COLOR_READY        TFT_GREEN
#define COLOR_RECORD       TFT_RED
#define COLOR_COUNTDOWN    TFT_YELLOW
#define COLOR_BUTTON       TFT_ORANGE
#define COLOR_PROGRESS_BG  TFT_DARKGREY
#define COLOR_PROGRESS     TFT_GREEN
#define COLOR_BOX_BG       TFT_BLACK
#define COLOR_BOX_BORDER   TFT_BLUE

TFT_eSPI tft = TFT_eSPI();

class IInferenceEngine {
public:
  virtual ~IInferenceEngine() {}
  virtual void begin() = 0;
  virtual void addSample(float value) = 0;
  virtual void runInference() = 0;
  virtual float getResult() = 0;
};

class PlaceholderInference : public IInferenceEngine {
public:
  void begin() override {}
  void addSample(float value) override {}
  void runInference() override {}
  float getResult() override { return 0.0; }
};

class SensorManager {
private:
  int rawValue;
  unsigned long lastReadTime;
public:
  SensorManager() : rawValue(0), lastReadTime(0) {}
  void begin() {
    pinMode(HALL_PIN, INPUT);
    analogReadResolution(12);
  }
  bool update() {
    unsigned long now = millis();
    if (now - lastReadTime >= SAMPLE_INTERVAL_MS) {
      rawValue = analogRead(HALL_PIN);
      lastReadTime = now;
      return true;
    }
    return false;
  }
  int getRaw() const { return rawValue; }
};

class ButtonManager {
private:
  bool last1, last2, last3;
  unsigned long debounce1, debounce2, debounce3;
  const unsigned long delay = 50;
public:
  ButtonManager() { last1 = last2 = last3 = HIGH; debounce1 = debounce2 = debounce3 = 0; }
  void begin() {
    pinMode(BTN_1, INPUT_PULLUP);
    pinMode(BTN_2, INPUT_PULLUP);
    pinMode(BTN_3, INPUT_PULLUP);
  }
  bool pressed1() { return pressed(BTN_1, last1, debounce1); }
  bool pressed2() { return pressed(BTN_2, last2, debounce2); }
  bool pressed3() { return pressed(BTN_3, last3, debounce3); }
private:
  bool pressed(int pin, bool &last, unsigned long &debounce) {
    bool reading = digitalRead(pin);
    if (reading != last) debounce = millis();
    if ((millis() - debounce) > delay) {
      if (reading == LOW) { last = reading; return true; }
    }
    last = reading;
    return false;
  }
};

class BuzzerManager {
public:
  void begin() { pinMode(BUZZER_PIN, OUTPUT); }
  void beep(int frequency, int duration) {
    tone(BUZZER_PIN, frequency, duration);
    delay(duration);
  }
};

class UIManager {
private:
  TFT_eSprite spr;
  String currentClass;
  int sampleCount;
  int lastRawValue;
  bool isRecording;
  String extraMessage;
  unsigned long lastBlinkTime;
  bool blinkState;

  String lastDisplayedClass;
  int lastDisplayedCount;
  int lastDisplayedRaw;
  String lastDisplayedExtra;
  bool lastRecordingState;

public:
  UIManager() : spr(&tft), currentClass("NO_CAR"), sampleCount(0),
                lastRawValue(0), isRecording(false), extraMessage(""),
                lastBlinkTime(0), blinkState(false),
                lastDisplayedClass(""), lastDisplayedCount(-1),
                lastDisplayedRaw(-1), lastDisplayedExtra(""), lastRecordingState(false) {}

  void begin() {
    tft.init();
    tft.setRotation(3);
    spr.createSprite(320, 240);
    spr.fillSprite(COLOR_BG);
    spr.pushSprite(0, 0);
  }

  void update(int count, int raw, bool recording, const String &cls, const String &extra = "") {
    sampleCount = count;
    lastRawValue = raw;
    isRecording = recording;
    currentClass = cls;
    extraMessage = extra;

    bool changed = false;
    if (currentClass != lastDisplayedClass) { lastDisplayedClass = currentClass; changed = true; }
    if (sampleCount != lastDisplayedCount) { lastDisplayedCount = sampleCount; changed = true; }
    if (lastRawValue != lastDisplayedRaw) { lastDisplayedRaw = lastRawValue; changed = true; }
    if (extraMessage != lastDisplayedExtra) { lastDisplayedExtra = extraMessage; changed = true; }
    if (isRecording != lastRecordingState) { lastRecordingState = isRecording; changed = true; }

    unsigned long now = millis();
    if (now - lastBlinkTime >= 500) {
      lastBlinkTime = now;
      blinkState = !blinkState;
      changed = true;
    }

    if (!changed) return;

    spr.fillSprite(COLOR_BG);

    spr.setTextColor(COLOR_TITLE);
    spr.setTextFont(2);
    spr.setTextSize(2);
    spr.drawString("Magnetic Logger", 10, 5);
    
    if (isRecording && blinkState) {
      spr.fillCircle(290, 18, 8, TFT_RED);
    } else {
      spr.fillCircle(290, 18, 8, COLOR_BG);
      spr.drawCircle(290, 18, 8, TFT_DARKGREY);
    }
    spr.drawLine(5, 35, 315, 35, TFT_DARKGREY);

    spr.setTextFont(2);
    spr.setTextSize(1);
    spr.setTextColor(COLOR_LABEL);
    spr.drawString("Class", 20, 50);
    spr.drawRoundRect(110, 45, 120, 28, 4, COLOR_BOX_BORDER);
    spr.fillRoundRect(111, 46, 118, 26, 4, COLOR_BOX_BG);
    spr.setTextColor(COLOR_VALUE);
    spr.setTextSize(2);
    spr.drawString(currentClass, 120, 52);

    spr.setTextFont(2);
    spr.setTextSize(1);
    spr.setTextColor(COLOR_LABEL);
    spr.drawString("Hall", 20, 90);
    spr.drawRoundRect(110, 85, 90, 28, 4, COLOR_BOX_BORDER);
    spr.fillRoundRect(111, 86, 88, 26, 4, COLOR_BOX_BG);
    spr.setTextColor(COLOR_VALUE);
    spr.setTextSize(2);
    char hallStr[10];
    sprintf(hallStr, "%4d", lastRawValue);
    spr.drawString(hallStr, 120, 92);

    spr.setTextFont(2);
    spr.setTextSize(1);
    spr.setTextColor(COLOR_LABEL);
    spr.drawString("Samples", 20, 130);
    int barX = 110, barY = 130, barW = 160, barH = 18;
    spr.drawRoundRect(barX, barY, barW, barH, 4, COLOR_BORDER);
    int fillW = map(sampleCount, 0, TOTAL_SAMPLES, 0, barW - 4);
    if (fillW > 0) {
      spr.fillRoundRect(barX + 2, barY + 2, fillW, barH - 4, 3, COLOR_PROGRESS);
    }
    spr.setTextColor(COLOR_VALUE);
    char samplesStr[20];
    sprintf(samplesStr, "%d / %d", sampleCount, TOTAL_SAMPLES);
    spr.drawString(samplesStr, barX + barW + 10, barY + 2);

    spr.setTextFont(2);
    spr.setTextSize(1);
    spr.setTextColor(COLOR_LABEL);
    spr.drawString("Status", 20, 175);
    uint16_t statusColor;
    if (isRecording) {
      statusColor = COLOR_RECORD;
    } else if (extraMessage.indexOf("...") >= 0) {
      statusColor = COLOR_COUNTDOWN;
    } else {
      statusColor = COLOR_READY;
    }
    spr.setTextColor(statusColor);
    spr.setTextSize(2);
    String statusText = isRecording ? "RECORDING" : (extraMessage.length() > 0 ? extraMessage : "READY");
    spr.drawString(statusText, 110, 175);

    spr.setTextFont(2);
    spr.setTextSize(1);
    spr.setTextColor(COLOR_BUTTON);
    spr.drawString("B1 START", 5, 215);
    spr.drawString("B2 TEST", 105, 215);
    spr.drawString("B3 NEXT", 220, 215);

    spr.drawRect(2, 2, 316, 236, COLOR_BORDER);
    spr.pushSprite(0, 0);
  }
};

class DataManager {
private:
  uint32_t sampleCounter;
  bool isRecording;
  String currentClass;
  SensorManager* sensor;
  IInferenceEngine* inference;
  unsigned long startTime;

public:
  DataManager(SensorManager* s, IInferenceEngine* inf)
    : sampleCounter(0), isRecording(false), currentClass("NO_CAR"), sensor(s), inference(inf), startTime(0) {}

  void setClass(const String &cls) { currentClass = cls; }
  String getClass() const { return currentClass; }

  void start() {
    if (!isRecording) {
      isRecording = true;
      sampleCounter = 0;
      startTime = millis();
      Serial.print("START,");
      Serial.println(currentClass);
      Serial.println("[INFO] Recording started");
    }
  }

  void stop() {
    if (isRecording) {
      isRecording = false;
      Serial.println("END");
      Serial.println("[INFO] Recording stopped");
    }
  }

  bool isActive() const { return isRecording; }
  uint32_t getCounter() const { return sampleCounter; }

  void processSample(int rawValue) {
    if (!isRecording) return;

    unsigned long timestamp = millis() - startTime;
    sampleCounter++;

    Serial.print(timestamp);
    Serial.print(",");
    Serial.println(rawValue);

    if (inference) inference->addSample((float)rawValue);

    if (sampleCounter >= TOTAL_SAMPLES) {
      stop();
    }
  }

  void sendTestData() {
    Serial.println("=== TEST DATA ===");
    for (int i = 0; i < 10; i++) {
      int fakeValue = 2000 + random(0, 500);
      Serial.print(i * 10);
      Serial.print(",");
      Serial.println(fakeValue);
      delay(20);
    }
    Serial.println("=== END TEST ===");
  }
};

SensorManager sensorManager;
PlaceholderInference inferenceEngine;
ButtonManager buttonManager;
BuzzerManager buzzerManager;
UIManager uiManager;
DataManager dataManager(&sensorManager, &inferenceEngine);

const String classList[] = {"SLOW", "FAST", "NO_CAR"};
const uint8_t numClasses = 3;
uint8_t currentClassIndex = 0;

enum SystemState { STATE_IDLE, STATE_COUNTDOWN, STATE_RECORDING };
SystemState systemState = STATE_IDLE;
unsigned long countdownStartTime = 0;
int countdownValue = 3;

void setup() {
  delay(100);
  Serial.begin(SERIAL_BAUD);
  while (!Serial) {
    delay(10);
  }
  Serial.println("========================================");
  Serial.println("  Wio Terminal Magnetic Data Logger");
  Serial.println("  Serial Speed: " + String(SERIAL_BAUD));
  Serial.println("  Press B1 to start recording");
  Serial.println("  Press B2 to send test data");
  Serial.println("  Press B3 to change class");
  Serial.println("========================================");
  
  sensorManager.begin();
  buttonManager.begin();
  buzzerManager.begin();
  uiManager.begin();
  inferenceEngine.begin();

  dataManager.setClass(classList[currentClassIndex]);
  uiManager.update(0, 0, false, dataManager.getClass(), "READY");
}

void loop() {
  if (buttonManager.pressed1()) {
    if (systemState == STATE_IDLE) {
      systemState = STATE_COUNTDOWN;
      countdownStartTime = millis();
      countdownValue = 3;
      buzzerManager.beep(1000, 100);
      uiManager.update(dataManager.getCounter(), sensorManager.getRaw(), false, dataManager.getClass(), "3...");
      Serial.println("[BTN] B1 pressed - Starting countdown");
    }
  }

  if (buttonManager.pressed2()) {
    Serial.println("[BTN] B2 pressed - Sending test data");
    dataManager.sendTestData();
    buzzerManager.beep(1500, 100);
    uiManager.update(dataManager.getCounter(), sensorManager.getRaw(), false, dataManager.getClass(), "TEST SENT");
    delay(300);
    uiManager.update(dataManager.getCounter(), sensorManager.getRaw(), false, dataManager.getClass(), "READY");
  }

  if (buttonManager.pressed3()) {
    if (systemState == STATE_IDLE) {
      currentClassIndex = (currentClassIndex + 1) % numClasses;
      String newClass = classList[currentClassIndex];
      dataManager.setClass(newClass);
      uiManager.update(dataManager.getCounter(), sensorManager.getRaw(), false, newClass, "READY");
      buzzerManager.beep(600, 50);
      Serial.print("[BTN] B3 pressed - Class changed to: ");
      Serial.println(newClass);
    }
  }

  if (systemState == STATE_COUNTDOWN) {
    unsigned long elapsed = millis() - countdownStartTime;
    int newValue = 3 - (elapsed / 1000);
    if (newValue != countdownValue) {
      countdownValue = newValue;
      if (countdownValue > 0) {
        String msg = String(countdownValue) + "...";
        uiManager.update(dataManager.getCounter(), sensorManager.getRaw(), false, dataManager.getClass(), msg);
        buzzerManager.beep(1200, 80);
        Serial.println("[COUNTDOWN] " + String(countdownValue));
      } else {
        systemState = STATE_RECORDING;
        dataManager.start();
        buzzerManager.beep(2000, 200);
        uiManager.update(dataManager.getCounter(), sensorManager.getRaw(), true, dataManager.getClass(), "RECORDING");
        Serial.println("[COUNTDOWN] Recording started!");
      }
    }
  }

  if (sensorManager.update()) {
    int raw = sensorManager.getRaw();

    if (systemState == STATE_RECORDING) {
      dataManager.processSample(raw);
      if (!dataManager.isActive()) {
        systemState = STATE_IDLE;
        buzzerManager.beep(800, 300);
        uiManager.update(dataManager.getCounter(), raw, false, dataManager.getClass(), "DONE");
        Serial.println("[RECORDING] Recording finished!");
      } else {
        uiManager.update(dataManager.getCounter(), raw, true, dataManager.getClass(), "RECORDING");
      }
    } else {
      if (systemState == STATE_IDLE) {
        uiManager.update(dataManager.getCounter(), raw, false, dataManager.getClass(), "READY");
      } else {
        uiManager.update(dataManager.getCounter(), raw, false, dataManager.getClass(), String(countdownValue) + "...");
      }
    }
  }

  delay(1);
}

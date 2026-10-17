
#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

// ---------------- PIN CONFIGURATION ----------------
#define IR_PIN 27
#define DFPLAYER_RX 16
#define DFPLAYER_TX 17

// ---------------- SYSTEM SETTINGS ------------------
#define TOTAL_MANTRAS 4
#define VOLUME_LEVEL 25
#define MAX_PLAY_TIME 300000UL  // 5-minute safety timeout

// ---------------- DFPLAYER SETUP -------------------
DFRobotDFPlayerMini dfPlayer;
HardwareSerial mySerial(2);

// ---------------- SYSTEM VARIABLES -----------------
bool mantraPlaying = false;
bool waitingForSensorClear = false;

int currentMantra = 0;
int previousMantra = 0;

unsigned long mantraStartTime = 0;

// ---------------- SETUP ----------------------------
void setup() {
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT);

  mySerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);

  Serial.println();
  Serial.println("========================================");
  Serial.println("           DAN PETI SYSTEM");
  Serial.println("              JAY GANESH");
  Serial.println("========================================");

  delay(1000);

  Serial.println("Initializing DFPlayer...");

  if (!dfPlayer.begin(mySerial)) {
    Serial.println();
    Serial.println("DFPlayer NOT detected!");
    Serial.println();
    Serial.println("Check:");
    Serial.println("1. DFPlayer VCC -> 5V");
    Serial.println("2. DFPlayer GND -> GND");
    Serial.println("3. DFPlayer TX -> ESP32 GPIO16");
    Serial.println("4. DFPlayer RX -> ESP32 GPIO17 through 1K");
    Serial.println("5. SD card inserted");
    Serial.println("6. Files inside /mp3/");
    Serial.println();

    while (true) {
      delay(1000);
    }
  }

  Serial.println("DFPlayer connected successfully!");

  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  dfPlayer.volume(VOLUME_LEVEL);

  delay(500);

  randomSeed(esp_random());

  // Wait until the IR sensor is clear at startup.
  while (digitalRead(IR_PIN) == LOW) {
    Serial.println("Waiting for IR sensor to become clear...");
    delay(200);
  }

  Serial.println();
  Serial.println("SYSTEM READY");
  Serial.println("Waiting for donation...");
}

// ---------------- MAIN LOOP ------------------------
void loop() {

  // Check DFPlayer events.
  if (dfPlayer.available()) {
    uint8_t type = dfPlayer.readType();
    int value = dfPlayer.read();

    if (type == DFPlayerPlayFinished) {
      Serial.println();
      Serial.println("Mantra finished completely!");

      Serial.print("Finished file: 000");
      Serial.println(value);

      if (mantraPlaying) {
        mantraPlaying = false;

        Serial.println("Playback lock released.");

        waitingForSensorClear = true;

        Serial.println("Waiting for sensor to become clear...");
      }
    }
    else if (type == DFPlayerCardOnline) {
      Serial.println("SD card detected.");
    }
    else if (type == DFPlayerCardInserted) {
      Serial.println("SD card inserted.");
    }
    else if (type == DFPlayerCardRemoved) {
      Serial.println("SD card removed!");
    }
    else if (type == DFPlayerError) {
      Serial.print("DFPlayer Error: ");
      Serial.println(value);
    }
  }

  // Safety timeout in case playback-finished event is missed.
  if (mantraPlaying) {
    if (millis() - mantraStartTime >= MAX_PLAY_TIME) {
      Serial.println();
      Serial.println("Safety timeout reached.");
      Serial.println("Unlocking system.");

      mantraPlaying = false;
      waitingForSensorClear = true;
    }
  }

  // Ignore all new IR detections while audio is playing.
  if (mantraPlaying) {
    return;
  }

  // After playback, wait until the sensor is clear.
  if (waitingForSensorClear) {
    if (digitalRead(IR_PIN) == HIGH) {
      waitingForSensorClear = false;

      Serial.println("Sensor clear.");
      Serial.println("System READY for next donation.");
    }

    return;
  }

  // Detect a donation (IR sensor active LOW).
  if (digitalRead(IR_PIN) == LOW) {
    delay(50);  // Basic sensor debounce

    if (digitalRead(IR_PIN) == LOW) {
      Serial.println();
      Serial.println("DONATION DETECTED!");

      // Clear any pending DFPlayer events.
      while (dfPlayer.available()) {
        dfPlayer.readType();
        dfPlayer.read();
      }

      // Select a random mantra without immediate repetition.
      do {
        currentMantra = random(1, TOTAL_MANTRAS + 1);
      } while (
        TOTAL_MANTRAS > 1 &&
        currentMantra == previousMantra
      );

      previousMantra = currentMantra;

      Serial.print("Random mantra selected: 000");
      Serial.println(currentMantra);

      // Play /mp3/0001.mp3 through /mp3/0004.mp3.
      dfPlayer.playMp3Folder(currentMantra);

      mantraPlaying = true;
      mantraStartTime = millis();

      Serial.println("Mantra started!");
      Serial.println("PLAYBACK LOCK ACTIVE");
      Serial.println("All IR detections will be ignored.");
    }
  }

  delay(20);
}

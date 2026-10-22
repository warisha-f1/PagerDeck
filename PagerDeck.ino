#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- OLED Display Configuration ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Pin Definitions ---
#define BUTTON_MUTE_PIN 4      // Button 1
#define BUTTON_TOGGLE_PIN 12   // Button 2
#define BUZZER_PIN 5           

// --- Wi-Fi Credentials ---
#define WIFI_SSID ""YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// --- Adafruit IO Credentials ---
const char* aioUsername = "Your_Adafruit_Username";
const char* aioKey = "YOUR_AIO_KEY"; // Replace with your Adafruit IO Key

// --- Feed Endpoints ---
const char* servingTokenUrl = "https://io.adafruit.com/api/v2/Your_Adafruit_Username/feeds/servingtoken/data/last";
const char* currentTokenUrl = "https://io.adafruit.com/api/v2/Your_Adafruit_Username/feeds/currenttoken/data/last";

int currentToken = 0;
int servingToken = 0;

// Display States: 0 = Token View, 1 = "YOUR TURN", 2 = "BRING YOUR ID"
int displayState = 0; 

bool isMuted = false;
int lastMatchToken = -1;
bool isNetworkOnline = true;

// Non-blocking timing trackers
unsigned long lastFetchTime = 0;
const unsigned long fetchInterval = 3000; 

unsigned long lastBuzzerToggleTime = 0;
bool buzzerState = false;
const unsigned long buzzerOnTime = 150;
const unsigned long buzzerOffTime = 150;

unsigned long lastMutePressTime = 0;
unsigned long lastTogglePressTime = 0;
const unsigned long debounceDelay = 250;

// Function Declarations
void updateDisplay();
void handleContinuousBuzzer();
int fetchAdafruitFeedToken(const char* url);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== PAGERDECK DAY 7 FIRMWARE INITIALIZATION ===");

  pinMode(BUTTON_MUTE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_TOGGLE_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("  PagerDeck Booting...");
  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected successfully!");
  isNetworkOnline = true;

  updateDisplay();
}

int fetchAdafruitFeedToken(const char* url) {
  if (WiFi.status() != WL_CONNECTED) return -1;

  HTTPClient http;
  http.begin(url);
  http.addHeader("X-AIO-Key", aioKey);
  http.setTimeout(1000); // 1-second timeout

  int httpCode = http.GET();
  int tokenValue = -1;

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      const char* valStr = doc["value"];
      tokenValue = atoi(valStr);
    }
  } else {
    Serial.print("HTTP Error Code: ");
    Serial.println(httpCode);
  }
  http.end();
  return tokenValue;
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. NON-BLOCKING CLOUD FETCH & RAM MONITOR
  if (currentMillis - lastFetchTime >= fetchInterval) {
    lastFetchTime = currentMillis;

    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    }

    int newCurrent = fetchAdafruitFeedToken(currentTokenUrl);
    int newServing = fetchAdafruitFeedToken(servingTokenUrl);

    if (newCurrent == -1 && newServing == -1) {
      isNetworkOnline = false;
      Serial.println("Network fetch failed!");
    } else {
      isNetworkOnline = true;
      if (newCurrent != -1) currentToken = newCurrent;
      if (newServing != -1) servingToken = newServing;
    }

    if (isNetworkOnline && servingToken == currentToken && currentToken > 0 && currentToken != lastMatchToken) {
      isMuted = false;
      displayState = 0;
      lastMatchToken = currentToken;
    }

    // Output status & free RAM monitor to Serial
    Serial.print("Current: ");
    Serial.print(currentToken);
    Serial.print(" | Serving: ");
    Serial.print(servingToken);
    Serial.print(" | Free Heap RAM: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");

    updateDisplay();
  }

  // 2. BUTTON 1 (GPIO 4 - MUTE & "YOUR TURN")
  if (digitalRead(BUTTON_MUTE_PIN) == LOW) {
    if (currentMillis - lastMutePressTime > debounceDelay) {
      isMuted = true;
      digitalWrite(BUZZER_PIN, LOW);
      buzzerState = false;
      displayState = 1;
      updateDisplay();
      
      lastMutePressTime = currentMillis;
      Serial.println("BUTTON 1 PRESSED - Muted");
    }
  }

  // 3. BUTTON 2 (GPIO 12 - TOGGLE REMINDER)
  if (digitalRead(BUTTON_TOGGLE_PIN) == LOW) {
    if (currentMillis - lastTogglePressTime > debounceDelay) {
      displayState = (displayState == 2) ? 0 : 2;
      updateDisplay();
      lastTogglePressTime = currentMillis;
      
      Serial.print("BUTTON 2 PRESSED - State: ");
      Serial.println(displayState);
    }
  }

  // 4. CONTINUOUS BUZZER LOGIC
  handleContinuousBuzzer();
}

void handleContinuousBuzzer() {
  if (isNetworkOnline && servingToken == currentToken && currentToken > 0 && !isMuted) {
    unsigned long currentMillis = millis();
    unsigned long interval = buzzerState ? buzzerOnTime : buzzerOffTime;

    if (currentMillis - lastBuzzerToggleTime >= interval) {
      lastBuzzerToggleTime = currentMillis;
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;
  }
}

void updateDisplay() {
  display.clearDisplay();

  if (!isNetworkOnline) {
    display.setTextSize(1);
    display.setCursor(0, 15);
    display.println(" Wi-Fi Disconnected");
    display.setCursor(0, 35);
    display.println("   Reconnecting...");
  } 
  else if (displayState == 1) {
    display.setTextSize(2);
    display.setCursor(10, 20);
    display.println("YOUR TURN");
  } 
  else if (displayState == 2) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("--- REMINDER ---");
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.println("BRING YOUR");
    display.println(" ID / PASS");
  } 
  else {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("=== PAGERDECK ===");

    display.setCursor(0, 20);
    display.print("YOUR TOKEN: ");
    display.setTextSize(2);
    display.println(currentToken);

    display.setTextSize(1);
    display.setCursor(0, 45);
    display.print("SERVING:    ");
    display.setTextSize(2);
    display.println(servingToken);
  }

  display.display();
}

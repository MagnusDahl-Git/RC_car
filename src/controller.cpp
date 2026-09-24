#include <WiFi.h>
#include <esp_now.h>

// REPLACE with your Vehicle ESP32's MAC Address
uint8_t vehicleAddress[] = {0x70, 0x4b, 0xca, 0x04, 0x5d, 0x58};

// Pin definitions
const int PIN_JOY1_X = 34;
const int PIN_JOY1_Y = 35;
const int PIN_JOY1_BTN = 32;
const int PIN_JOY2_X = 33;
const int PIN_JOY2_Y = 36;
const int PIN_JOY2_BTN = 25;

// Data packet structure
struct ControlData {
  int16_t joy1_x;
  int16_t joy1_y;
  bool joy1_btn;
  int16_t joy2_x;
  int16_t joy2_y;
  bool joy2_btn;
} myData;

esp_now_peer_info_t peerInfo;

// Deadzone function: takes raw ADC (0-4095), maps to -100 to 100, and filters out noise
int16_t applyDeadzone(int rawValue, int deadzonePercent) {
  int val = map(rawValue, 0, 4095, -100, 100);
  if (abs(val) < deadzonePercent) {
    return 0;
  }
  // Rescale the active range past the deadzone to full 0-100 magnitude
  if (val > 0) {
    return map(val, deadzonePercent, 100, 0, 100);
  } else {
    return map(val, -deadzonePercent, -100, 0, -100);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_JOY1_BTN, INPUT_PULLUP);
  pinMode(PIN_JOY2_BTN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register peer
  memcpy(peerInfo.peer_addr, vehicleAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  // Read joysticks with a 10% deadzone applied
  myData.joy1_x = applyDeadzone(analogRead(PIN_JOY1_X), 10);
  myData.joy1_y = applyDeadzone(analogRead(PIN_JOY1_Y), 10);
  myData.joy1_btn = !digitalRead(PIN_JOY1_BTN); // Active low button
  
  myData.joy2_x = applyDeadzone(analogRead(PIN_JOY2_X), 10);
  myData.joy2_y = applyDeadzone(analogRead(PIN_JOY2_Y), 10);
  myData.joy2_btn = !digitalRead(PIN_JOY2_BTN); // Active low button

  // Print transmitted data to Serial Monitor
  Serial.print("TX -> J1 X: "); Serial.print(myData.joy1_x);
  Serial.print(" | Y: "); Serial.print(myData.joy1_y);
  Serial.print(" | B1: "); Serial.print(myData.joy1_btn);
  Serial.print(" || J2 X: "); Serial.print(myData.joy2_x);
  Serial.print(" | Y: "); Serial.print(myData.joy2_y);
  Serial.print(" | B2: "); Serial.println(myData.joy2_btn);

  // Send data via ESP-NOW
  esp_err_t result = esp_now_send(vehicleAddress, (uint8_t *) &myData, sizeof(myData));
  if (result != ESP_OK) {
    // Serial.println("Error sending data");
  }

  delay(20); // Send interval (~50fps)
}
#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

// Pin definitions
const int PIN_STBY = 22;
const int PIN_AIN1 = 26;
const int PIN_AIN2 = 27;
const int PIN_SERVO = 18;

// LEDC Channels for PWM
const int CHANNEL_AIN1 = 0;
const int CHANNEL_AIN2 = 1;

Servo steeringServo;

// Mirror data structure
struct ControlData {
  int16_t joy1_x;
  int16_t joy1_y;
  bool joy1_btn;
  int16_t joy2_x;
  int16_t joy2_y;
  bool joy2_btn;
} incomingData;

// Fail-safe (watchdog) variables
unsigned long lastReceiveTime = 0;
const unsigned long FAILSAFE_TIMEOUT = 500; // Stop if no signal for 500ms

void stopMotors() {
  ledcWrite(CHANNEL_AIN1, 0);
  ledcWrite(CHANNEL_AIN2, 0);
}

void driveMotor(int16_t throttle) {
  // throttle ranges from -100 to 100
  if (throttle > 0) {
    int pwmVal = map(throttle, 0, 100, 0, 255);
    ledcWrite(CHANNEL_AIN1, pwmVal);
    ledcWrite(CHANNEL_AIN2, 0);
  } else if (throttle < 0) {
    int pwmVal = map(abs(throttle), 0, 100, 0, 255);
    ledcWrite(CHANNEL_AIN1, 0);
    ledcWrite(CHANNEL_AIN2, pwmVal);
  } else {
    stopMotors();
  }
}

// Callback using the classic ESP-NOW signature (compatible with older ESP-IDF frameworks)
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  lastReceiveTime = millis(); // Refresh timeout tracker

  // Drive motor using Joystick 1 Y-axis
  driveMotor(incomingData.joy1_y);

  // Steer servo using Joystick 2 X-axis (mapped from -100/100 to 0/180 degrees)
  int servoAngle = map(incomingData.joy2_x, -100, 100, 0, 180);
  steeringServo.write(servoAngle);
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Give the serial monitor a moment to initialize

  // 1. Initialize Wi-Fi in Station Mode (Required for ESP-NOW and MAC reading)
  WiFi.mode(WIFI_STA);

  // Print the MAC Address to the Serial Monitor
  Serial.println();
  Serial.print("Vehicle ESP32 MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println();

  // 2. Initialize H-Bridge Standby Pin HIGH
  pinMode(PIN_STBY, OUTPUT);
  digitalWrite(PIN_STBY, HIGH);

  // 3. Setup classic LEDC channels for AIN1 & AIN2
  ledcSetup(CHANNEL_AIN1, 5000, 8); // 5 kHz frequency, 8-bit resolution
  ledcAttachPin(PIN_AIN1, CHANNEL_AIN1);
  
  ledcSetup(CHANNEL_AIN2, 5000, 8);
  ledcAttachPin(PIN_AIN2, CHANNEL_AIN2);
  
  stopMotors();

  // 4. Setup Micro Servo
  steeringServo.attach(PIN_SERVO);
  steeringServo.write(90); // Center position on boot

  // 5. Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // Fail-Safe Watchdog: cuts motor power and centers steering if connection drops
  if (millis() - lastReceiveTime > FAILSAFE_TIMEOUT) {
    stopMotors();
    steeringServo.write(90); // Center steering
  }
}
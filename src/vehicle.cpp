#include <esp_now.h>
#include <WiFi.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Define Motor Driver Control Pins
#define MOTOR1_IN1 16
#define MOTOR1_IN2 17
#define MOTOR2_IN1 18
#define MOTOR2_IN2 19

struct DataPacket {
    uint16_t pot[4];
};

DataPacket receivedData;

// Updated callback signature with (const uint8_t * mac_addr, ...)
void OnDataRecv(const uint8_t * mac_addr, const uint8_t * incomingData, int len) {
    memcpy(&receivedData, incomingData, sizeof(receivedData));

    // Toggle onboard LED on every incoming message to verify signal reception
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

    // Motor 1 Control (Potentiometer 0)
    int raw1 = receivedData.pot[0];
    if (raw1 > 2150) { // Forward
        int pwmVal = map(raw1, 2150, 4095, 0, 255);
        analogWrite(MOTOR1_IN1, pwmVal);
        analogWrite(MOTOR1_IN2, 0);
    } else if (raw1 < 1950) { // Reverse
        int pwmVal = map(raw1, 1950, 0, 0, 255);
        analogWrite(MOTOR1_IN1, 0);
        analogWrite(MOTOR1_IN2, pwmVal);
    } else { // Deadzone / Stop
        analogWrite(MOTOR1_IN1, 0);
        analogWrite(MOTOR1_IN2, 0);
    }

    // Motor 2 Control (Potentiometer 1)
    int raw2 = receivedData.pot[1];
    if (raw2 > 2150) { // Forward
        int pwmVal = map(raw2, 2150, 4095, 0, 255);
        analogWrite(MOTOR2_IN1, pwmVal);
        analogWrite(MOTOR2_IN2, 0);
    } else if (raw2 < 1950) { // Reverse
        int pwmVal = map(raw2, 1950, 0, 0, 255);
        analogWrite(MOTOR2_IN1, 0);
        analogWrite(MOTOR2_IN2, pwmVal);
    } else { // Deadzone / Stop
        analogWrite(MOTOR2_IN1, 0);
        analogWrite(MOTOR2_IN2, 0);
    }

    Serial.printf("[RX] Received Pots: [%d, %d, %d, %d]\n", 
                  receivedData.pot[0], receivedData.pot[1], receivedData.pot[2], receivedData.pot[3]);
}

void setup() {
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(MOTOR1_IN1, OUTPUT);
    pinMode(MOTOR1_IN2, OUTPUT);
    pinMode(MOTOR2_IN1, OUTPUT);
    pinMode(MOTOR2_IN2, OUTPUT);

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Failed");
        return;
    }

    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("Receiver Ready.");
}

void loop() {
    // All receiver logic is handled inside OnDataRecv callback
}
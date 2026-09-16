#include <esp_now.h>
#include <WiFi.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // GPIO 2 is standard on most ESP32 dev boards (GPIO 4 on some)
#endif

// Define ADC pins for 4 Potentiometers (Use ADC1 pins: GPIO 32, 33, 34, 35)
const int POT_PINS[4] = {34, 35, 32, 33};

struct DataPacket {
    uint16_t pot[4];
};

DataPacket sensorData;

// Broadcast address (Sends to all listening devices)
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Failed");
        return;
    }

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    for (int i = 0; i < 4; i++) {
        pinMode(POT_PINS[i], INPUT);
    }

    Serial.println("Transmitter Ready.");
}

void loop() {
    // Read all 4 potentiometers
    for (int i = 0; i < 4; i++) {
        sensorData.pot[i] = analogRead(POT_PINS[i]);
    }

    // Transmit data packet over ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sensorData, sizeof(sensorData));

    if (result == ESP_OK) {
        // Blink LED briefly to verify transmitter activity
        digitalWrite(LED_BUILTIN, HIGH);
        delay(10);
        digitalWrite(LED_BUILTIN, LOW);
        
        Serial.printf("[TX] Sent: %d, %d, %d, %d\n", sensorData.pot[0], sensorData.pot[1], sensorData.pot[2], sensorData.pot[3]);
    } else {
        Serial.println("[TX] Error sending data");
    }

    delay(90); // Total loop delay ~100ms
}
#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>  // Include the CAN library

// Define CAN interface pins (use your specific ESP32 pins)
#define CAN_TX 5
#define CAN_RX 4

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Initializing CAN...");

    // Set up CAN configuration
    ESP32Can.setPins(CAN_TX, CAN_RX); // Set TX and RX pins
    ESP32Can.setSpeed(TWAI_SPEED_125KBPS); // Set CAN speed to 500 Kbps

    // // Optional: set filter (accept all)
    // twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    // ESP32Can.setFilter(&filter);
    
    ESP32Can.begin(); // Initialize CAN interface

    Serial.println("CAN Initialized");

}

void loop() {
    // Sending a CAN message every 2 seconds
    // static unsigned long lastSent = 0;
    // if (millis() - lastSent > 5000) {
    //     lastSent = millis();

    //     CanFrame frame;
    //     frame.identifier = 0x123; // Message ID
    //     frame.extd = false; // Standard frame
    //     frame.data_length_code = 8; // Data length
    //     for (int i = 0; i < 8; i++) {
    //         frame.data[i] = i;
    //     }

    //     // Send the CAN frame
    //     if (ESP32Can.writeFrame(frame)) {
    //         Serial.println("CAN frame sent!");
    //     } else {
    //         Serial.println("Failed to send CAN frame.");
    //     }
    // }

    // Check if a CAN frame is received
    CanFrame incoming;
    if (ESP32Can.readFrame(incoming)) {
        Serial.print("Received CAN frame. ID: 0x");
        Serial.print(incoming.identifier, HEX);
        Serial.print(" Data: ");
        for (int i = 0; i < incoming.data_length_code; i++) {
            Serial.print(incoming.data[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    delay(10);
}

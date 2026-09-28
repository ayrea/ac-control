#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp> // Include the CAN library
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled for this build
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Bluetooth Classic SPP is only available on the original ESP32
#endif

// Define CAN interface pins (use your specific ESP32 pins)
#define CAN_TX 5
#define CAN_RX 4

// Name shown when pairing from Windows or Serial Bluetooth Terminal
#define BT_DEVICE_NAME "AC-CAN-Sniffer"

// Deeper than the library default of 5 so bursts survive a slow Bluetooth write
static const uint16_t CAN_RX_QUEUE_SIZE = 32;

// Drains the RX queue, then yields while waiting for the next frame
static const uint32_t CAN_READ_TIMEOUT_MS = 10;

BluetoothSerial SerialBT;

void emitLine(const char *line)
{
    Serial.print(line);

    if (SerialBT.hasClient())
    {
        SerialBT.print(line);
    }
}

void printFrame(const CanFrame &frame)
{
    char line[128];

    int written = snprintf(line, sizeof(line), "%8lu ms  ID: 0x%0*X  DLC: %u  Data:",
                           (unsigned long)millis(),
                           frame.extd ? 8 : 3,
                           (unsigned int)frame.identifier,
                           (unsigned int)frame.data_length_code);
    if (written < 0 || (size_t)written >= sizeof(line))
    {
        return;
    }

    size_t pos = (size_t)written;
    for (uint8_t i = 0; i < frame.data_length_code && pos + 4 < sizeof(line); i++)
    {
        written = snprintf(line + pos, sizeof(line) - pos, " %02X", frame.data[i]);
        if (written < 0)
        {
            break;
        }
        pos += (size_t)written;
    }

    // CR+LF keeps PuTTY from stair-stepping the output
    snprintf(line + pos, sizeof(line) - pos, "\r\n");

    emitLine(line);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Initializing CAN...");

    // Set up CAN configuration
    ESP32Can.setPins(CAN_TX, CAN_RX);            // Set TX and RX pins
    ESP32Can.setSpeed(TWAI_SPEED_125KBPS);       // Set CAN speed to 125 Kbps
    ESP32Can.setRxQueueSize(CAN_RX_QUEUE_SIZE);  // Must be set before begin()

    ESP32Can.begin(); // Initialize CAN interface

    Serial.println("CAN Initialized");

    if (SerialBT.begin(BT_DEVICE_NAME))
    {
        Serial.println("Bluetooth SPP started as \"" BT_DEVICE_NAME "\"");
    }
    else
    {
        Serial.println("Failed to start Bluetooth SPP");
    }
}

void loop()
{
    CanFrame incoming;
    while (ESP32Can.readFrame(incoming, CAN_READ_TIMEOUT_MS))
    {
        printFrame(incoming);
    }
}

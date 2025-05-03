#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp> // Include the CAN library

// Define CAN interface pins (use your specific ESP32 pins)
#define CAN_TX 5
#define CAN_RX 4

#define FRAME_DATETIME 0x4040001
#define FRAME_CURRENT_TEMP 0x4040001

#define FRAME_ZONE0 0x140C0003
#define FRAME_ZONE1 0x140C2003
#define FRAME_ZONE2 0x140C4003
#define FRAME_ZONE3 0x140C6003
#define FRAME_ZONE4 0x140C8003
#define FRAME_ZONE5 0x140CA003

#define FRAME_SET_TEMP 0x140C0017
#define FRAME_FAN_SPEED 0x140C0015
#define FRAME_MODE 0x140C0014

enum sendStepEnum
{
    Zone0Closed,
    Zone0Open,
    Zone1Closed,
    Zone1Open,
    Zone2Closed,
    Zone2Open,
    Zone3Closed,
    Zone3Open,
    Zone4Closed,
    Zone4Open,
    Zone5Closed,
    Zone5Open,
    Temp20,
    Temp21,
    Temp22,
    Temp23,
    Temp24,
    Temp25,
    Temp26,
    Temp27,
    Temp28,
    Temp29,
    TempReset,
    FanHigh,
    FanMedium,
    FanLow,
    ModeHeat,
    ModeVent,
    ModeDry,
    ModeAuto,
    ModeCool
};

static uint32_t delayTime = 30000;
static unsigned long lastSent = 0;
static sendStepEnum sendStep = Zone0Closed;

void displayFrame(CanFrame &frame, bool sending)
{
    if (sending)
    {
        Serial.print("Sending ");

        Serial.print("Frame ID: 0x");
        Serial.print(frame.identifier, HEX);

        Serial.print(" extd: ");
        Serial.print(frame.extd, HEX);

        Serial.print(" flags: ");
        Serial.print(frame.flags, HEX);

        Serial.print(" rtr: ");
        Serial.print(frame.rtr, HEX);

        Serial.print(" self: ");
        Serial.print(frame.self, HEX);

        Serial.print(" Data: ");
        for (int i = 0; i < frame.data_length_code; i++)
        {
            Serial.print(frame.data[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
    else
    {
        if (frame.identifier == FRAME_DATETIME)
        {
            // Date/Time
            Serial.print("Date/Time 20");
            Serial.print(frame.data[4]); // yyyy
            Serial.print("/");
            Serial.print(frame.data[5]); // mmm
            Serial.print("/");
            Serial.print(frame.data[6]); // dd

            Serial.print(" ");

            Serial.print(frame.data[0]); // HH
            Serial.print(":");
            Serial.print(frame.data[1]); // mm
            Serial.print(":");
            Serial.println(frame.data[2]); // ss
        }

        if (frame.identifier == FRAME_CURRENT_TEMP)
        {
            // Current temperature
            uint16_t binTemp = (frame.data[5] << 8) + frame.data[4];
            Serial.print("Hex Temp ");
            Serial.print(binTemp, HEX);
            Serial.print(" (");
            Serial.print(binTemp);
            Serial.print(") ");

            float temperature = (float)binTemp / 100.0;
            Serial.print("Current Temperature ");
            Serial.print(temperature);
            Serial.print(" (");
            Serial.print(temperature, 1);
            Serial.println(")");
        }
    }
}

void sendFrame(CanFrame &frame)
{
    displayFrame(frame, true);
    if (ESP32Can.writeFrame(frame))
    {
        Serial.println("CAN frame sent!");
    }
    else
    {
        Serial.println("Failed to send CAN frame.");
    }
}

void setZone(CanFrame &frame, uint8_t zone, bool zoneOpen)
{
    switch (zone)
    {
    case 0:
        frame.identifier = FRAME_ZONE0;
        break;

    case 1:
        frame.identifier = FRAME_ZONE1;
        break;

    case 2:
        frame.identifier = FRAME_ZONE2;
        break;

    case 3:
        frame.identifier = FRAME_ZONE3;
        break;

    case 4:
        frame.identifier = FRAME_ZONE4;
        break;

    case 5:
        frame.identifier = FRAME_ZONE5;
        break;

    default:
        frame.identifier = 0; // Should never get here
    }

    if (zoneOpen)
    {
        frame.data[0] = 1;
    }
    else
    {
        frame.data[0] = 2;
    }

    frame.data[1] = 9;
    frame.data[2] = 0;
    frame.data[3] = 0;
    frame.data[4] = 0;
    frame.data[5] = 0;
    frame.data[6] = 0;
    frame.data[7] = 0;

    Serial.print("Zone ");
    Serial.print(zone);
    if (zoneOpen)
    {
        Serial.println(" Open");
    }
    else
    {
        Serial.println(" Closed");
    }

    sendFrame(frame);
}

void setTemperature(CanFrame &frame, uint8_t temperature)
{
    if (temperature < 15)
    {
        temperature = 15;
    }

    if (temperature > 30)
    {
        temperature = 30;
    }

    int longTemp = temperature * 100;
    uint8_t binTempL = longTemp & 0xFF;
    uint8_t binTempH = (longTemp >> 8) & 0xFF;

    frame.identifier = FRAME_SET_TEMP; // Set temperature

    frame.data[0] = binTempL;
    frame.data[1] = binTempH;
    frame.data[2] = 0;
    frame.data[3] = 0;
    frame.data[4] = 0;
    frame.data[5] = 0;
    frame.data[6] = 0;
    frame.data[7] = 0;

    Serial.print("Set temperature ");
    Serial.println(temperature);

    sendFrame(frame);
}

void setFanSpeed(CanFrame &frame, uint8_t speed)
{
    if (speed < 1)
    {
        speed = 1;
    }

    if (speed > 3)
    {
        speed = 3;
    }

    frame.identifier = FRAME_FAN_SPEED; // Set fan speed

    frame.data[0] = speed;
    frame.data[1] = 9;
    frame.data[2] = 0;
    frame.data[3] = 0;
    frame.data[4] = 0;
    frame.data[5] = 0;
    frame.data[6] = 0;
    frame.data[7] = 0;

    Serial.print("Fan speed ");
    switch (speed)
    {
    case 1:
        Serial.println("Low");
        break;

    case 2:
        Serial.println("Medium");
        break;

    case 3:
        Serial.println("High");
        break;

    default:
        break;
    }

    sendFrame(frame);
}

void setMode(CanFrame &frame, uint8_t mode)
{
    if (mode < 1)
    {
        mode = 1;
    }

    if (mode > 5)
    {
        mode = 5;
    }

    frame.identifier = FRAME_MODE; // Set mode

    frame.data[0] = mode;
    frame.data[1] = 9;
    frame.data[2] = 0;
    frame.data[3] = 0;
    frame.data[4] = 0;
    frame.data[5] = 0;
    frame.data[6] = 0;
    frame.data[7] = 0;

    Serial.print("Mode ");
    switch (mode)
    {
    case 1:
        Serial.println("Cool");
        break;

    case 2:
        Serial.println("Heat");
        break;

    case 3:
        Serial.println("Vent");
        break;

    case 4:
        Serial.println("Dry");
        break;

    case 5:
        Serial.println("Auto");
        break;

    default:
        break;
    }

    sendFrame(frame);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Initializing CAN...");

    // Set up CAN configuration
    ESP32Can.setPins(CAN_TX, CAN_RX);      // Set TX and RX pins
    ESP32Can.setSpeed(TWAI_SPEED_125KBPS); // Set CAN speed to 500 Kbps

    // // Optional: set filter (accept all)
    // twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    // ESP32Can.setFilter(&filter);

    ESP32Can.begin(); // Initialize CAN interface

    Serial.println("CAN Initialized");

    lastSent = millis();
}

void loop()
{

    if (millis() - lastSent > delayTime)
    {
        lastSent = millis();

        CanFrame outgoingFrame;
        outgoingFrame.extd = 1;
        outgoingFrame.flags = 1;
        outgoingFrame.data_length_code = 8;

        switch (sendStep)
        {
        case Zone0Closed:
            setZone(outgoingFrame, 0, false);
            sendStep = Zone0Open;
            break;

        case Zone0Open:
            setZone(outgoingFrame, 0, true);
            sendStep = Zone1Closed;
            break;

        case Zone1Closed:
            setZone(outgoingFrame, 1, false);
            sendStep = Zone1Open;
            break;

        case Zone1Open:
            setZone(outgoingFrame, 1, true);
            sendStep = Zone2Closed;
            break;

        case Zone2Closed:
            setZone(outgoingFrame, 2, false);
            sendStep = Zone2Open;
            break;

        case Zone2Open:
            setZone(outgoingFrame, 2, true);
            sendStep = Zone3Closed;
            break;

        case Zone3Closed:
            setZone(outgoingFrame, 3, false);
            sendStep = Zone3Open;
            break;

        case Zone3Open:
            setZone(outgoingFrame, 3, true);
            sendStep = Zone4Closed;
            break;

        case Zone4Closed:
            setZone(outgoingFrame, 4, false);
            sendStep = Zone4Open;
            break;

        case Zone4Open:
            setZone(outgoingFrame, 4, true);
            sendStep = Zone5Closed;
            break;

        case Zone5Closed:
            setZone(outgoingFrame, 5, false);
            sendStep = Zone5Open;
            break;

        case Zone5Open:
            setZone(outgoingFrame, 5, true);
            sendStep = Temp20;
            break;

        case Temp20:
            setTemperature(outgoingFrame, 20);
            sendStep = Temp21;
            break;

        case Temp21:
            setTemperature(outgoingFrame, 21);
            sendStep = Temp22;
            break;

        case Temp22:
            setTemperature(outgoingFrame, 22);
            sendStep = Temp23;
            break;

        case Temp23:
            setTemperature(outgoingFrame, 23);
            sendStep = Temp24;
            break;

        case Temp24:
            setTemperature(outgoingFrame, 24);
            sendStep = Temp25;
            break;

        case Temp25:
            setTemperature(outgoingFrame, 25);
            sendStep = Temp26;
            break;

        case Temp26:
            setTemperature(outgoingFrame, 26);
            sendStep = Temp27;
            break;

        case Temp27:
            setTemperature(outgoingFrame, 27);
            sendStep = Temp28;
            break;

        case Temp28:
            setTemperature(outgoingFrame, 28);
            sendStep = Temp29;
            break;

        case Temp29:
            setTemperature(outgoingFrame, 29);
            sendStep = TempReset;
            break;

        case TempReset:
            setTemperature(outgoingFrame, 24);
            sendStep = FanHigh;
            break;

        case FanHigh:
            setFanSpeed(outgoingFrame, 3);
            sendStep = FanMedium;
            break;

        case FanMedium:
            setFanSpeed(outgoingFrame, 2);
            sendStep = FanLow;
            break;

        case FanLow:
            setFanSpeed(outgoingFrame, 1);
            sendStep = ModeHeat;
            break;

        case ModeHeat:
            setMode(outgoingFrame, 2);
            sendStep = ModeVent;
            break;

        case ModeVent:
            setMode(outgoingFrame, 3);
            sendStep = ModeDry;
            break;

        case ModeDry:
            setMode(outgoingFrame, 4);
            sendStep = ModeAuto;
            break;

        case ModeAuto:
            setMode(outgoingFrame, 5);
            sendStep = ModeCool;
            break;

        case ModeCool:
            setMode(outgoingFrame, 1);
            sendStep = Zone0Closed;
            break;

        default:
            Serial.println("SendStep invalid");
        }
    }

    // Check if a CAN frame is received
    CanFrame incoming;
    if (ESP32Can.readFrame(incoming))
    {
        displayFrame(incoming, false);
    }
}

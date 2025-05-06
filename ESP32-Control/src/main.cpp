#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp> // Include the CAN library
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"

// Define CAN interface pins (use your specific ESP32 pins)
#define CAN_TX 5
#define CAN_RX 4

const char *ssid = "ssid";
const char *password = "password";

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
#define FRAME_POWER 0x140C0013

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

static float acCurrentTemp = 24.0;
static bool acZone0 = true;
static bool acZone1 = true;
static bool acZone2 = true;
static bool acZone3 = true;
static bool acZone4 = true;
static bool acZone5 = true;
static int acSetTemp = 24;
static uint8_t acFanSpeed = 1;
static uint8_t acMode = 1;
static bool acPower = false;

static CanFrame outgoingFrame;

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        Serial.println("WebSocket client connected");
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.println("WebSocket client disconnected");
    }
}

void displayFrame(CanFrame &frame, bool sending)
{
    if (sending)
    {
        Serial.print("Sending ");
    }
    else
    {
        Serial.print("Received ");
    }

    Serial.print("ID: 0x");
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

void processReceivedFrame(CanFrame &frame)
{
    bool printFrameData = false;

    if (frame.identifier == FRAME_DATETIME)
    {
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
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_CURRENT_TEMP)
    {
        uint16_t binTemp = (frame.data[5] << 8) + frame.data[4];
        acCurrentTemp = (float)binTemp / 100.0;
        Serial.print("Current Temperature ");
        Serial.print(acCurrentTemp, 1);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_ZONE0)
    {
        acZone0 = frame.data[0] == 1;
        Serial.print("Zone 0 ");
        Serial.println(acZone0);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_ZONE1)
    {
        acZone1 = frame.data[0] == 1;
        Serial.print("Zone 1 ");
        Serial.println(acZone1);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_ZONE2)
    {
        acZone2 = frame.data[0] == 1;
        Serial.print("Zone 2 ");
        Serial.println(acZone2);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_ZONE3)
    {
        acZone3 = frame.data[0] == 1;
        Serial.print("Zone 3 ");
        Serial.println(acZone3);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_ZONE4)
    {
        acZone4 = frame.data[0] == 1;
        Serial.print("Zone 4 ");
        Serial.println(acZone4);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_ZONE5)
    {
        acZone5 = frame.data[0] == 1;
        Serial.print("Zone 5 ");
        Serial.println(acZone5);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_SET_TEMP)
    {
        acSetTemp = ((frame.data[1] << 8) + frame.data[0]) / 100;
        Serial.print("Set Temperature ");
        Serial.println(acSetTemp);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_FAN_SPEED)
    {
        acFanSpeed = frame.data[0];
        Serial.print("Fan Speed ");
        Serial.println(acFanSpeed);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_MODE)
    {
        acMode = frame.data[0];
        Serial.print("Mode ");
        Serial.println(acMode);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_POWER)
    {
        acPower = frame.data[0] == 1;
        Serial.print("Power ");
        Serial.println(acPower);
        printFrameData = true;
    }

    if (printFrameData)
    {
        displayFrame(frame, false);
    }
}

void sendFrame()
{
    if (ESP32Can.writeFrame(outgoingFrame, 200))
    {
        displayFrame(outgoingFrame, true);
        Serial.println("CAN frame sent!");
    }
    else
    {
        Serial.println("Failed to send CAN frame.");
        twai_status_info_t status_info;
        twai_get_status_info(&status_info);
        Serial.println(status_info.state); // TWAI_STATE_STOPPED, TWAI_STATE_RUNNING, TWAI_STATE_BUS_OFF
        Serial.println(status_info.tx_error_counter);
        Serial.println(status_info.rx_error_counter);
    }
}

void setZone(uint8_t zone)
{
    switch (zone)
    {
    case 0:
        outgoingFrame.identifier = FRAME_ZONE0;
        outgoingFrame.data[0] = acZone0 ? 1 : 2;
        break;

    case 1:
        outgoingFrame.identifier = FRAME_ZONE1;
        outgoingFrame.data[0] = acZone1 ? 1 : 2;
        break;

    case 2:
        outgoingFrame.identifier = FRAME_ZONE2;
        outgoingFrame.data[0] = acZone2 ? 1 : 2;
        break;

    case 3:
        outgoingFrame.identifier = FRAME_ZONE3;
        outgoingFrame.data[0] = acZone3 ? 1 : 2;
        break;

    case 4:
        outgoingFrame.identifier = FRAME_ZONE4;
        outgoingFrame.data[0] = acZone4 ? 1 : 2;
        break;

    case 5:
        outgoingFrame.identifier = FRAME_ZONE5;
        outgoingFrame.data[0] = acZone5 ? 1 : 2;
        break;

    default:
        outgoingFrame.identifier = 0; // Should never get here
        return;
    }

    outgoingFrame.data[1] = 9;
    outgoingFrame.data[2] = 0;
    outgoingFrame.data[3] = 0;
    outgoingFrame.data[4] = 0;
    outgoingFrame.data[5] = 0;
    outgoingFrame.data[6] = 0;
    outgoingFrame.data[7] = 0;

    Serial.print("Setting Zone ");

    sendFrame();
}

void setTemperature()
{
    if (acSetTemp < 15)
    {
        acSetTemp = 15;
    }

    if (acSetTemp > 30)
    {
        acSetTemp = 30;
    }

    int longTemp = acSetTemp * 100;
    uint8_t binTempL = longTemp & 0xFF;
    uint8_t binTempH = (longTemp >> 8) & 0xFF;

    outgoingFrame.identifier = FRAME_SET_TEMP; // Set temperature

    outgoingFrame.data[0] = binTempL;
    outgoingFrame.data[1] = binTempH;
    outgoingFrame.data[2] = 0;
    outgoingFrame.data[3] = 0;
    outgoingFrame.data[4] = 0;
    outgoingFrame.data[5] = 0;
    outgoingFrame.data[6] = 0;
    outgoingFrame.data[7] = 0;

    Serial.print("Set temperature ");
    Serial.println(acSetTemp);

    sendFrame();
}

void setFanSpeed()
{
    if (acFanSpeed < 1)
    {
        acFanSpeed = 1;
    }

    if (acFanSpeed > 3)
    {
        acFanSpeed = 3;
    }

    outgoingFrame.identifier = FRAME_FAN_SPEED; // Set fan speed

    outgoingFrame.data[0] = acFanSpeed;
    outgoingFrame.data[1] = 9;
    outgoingFrame.data[2] = 0;
    outgoingFrame.data[3] = 0;
    outgoingFrame.data[4] = 0;
    outgoingFrame.data[5] = 0;
    outgoingFrame.data[6] = 0;
    outgoingFrame.data[7] = 0;

    Serial.print("Fan speed ");
    switch (acFanSpeed)
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

    sendFrame();
}

void setMode()
{
    if (acMode < 1)
    {
        acMode = 1;
    }

    if (acMode > 5)
    {
        acMode = 5;
    }

    outgoingFrame.identifier = FRAME_MODE; // Set mode

    outgoingFrame.data[0] = acMode;
    outgoingFrame.data[1] = 9;
    outgoingFrame.data[2] = 0;
    outgoingFrame.data[3] = 0;
    outgoingFrame.data[4] = 0;
    outgoingFrame.data[5] = 0;
    outgoingFrame.data[6] = 0;
    outgoingFrame.data[7] = 0;

    Serial.print("Mode ");
    switch (acMode)
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

    sendFrame();
}

void setPower()
{
    outgoingFrame.extd = 1;
    outgoingFrame.flags = 1;
    outgoingFrame.data_length_code = 8;

    outgoingFrame.identifier = FRAME_POWER; // Set power on/off

    outgoingFrame.data[0] = acPower ? 1 : 0;
    outgoingFrame.data[1] = 9;
    outgoingFrame.data[2] = 0;
    outgoingFrame.data[3] = 0;
    outgoingFrame.data[4] = 0;
    outgoingFrame.data[5] = 0;
    outgoingFrame.data[6] = 0;
    outgoingFrame.data[7] = 0;

    Serial.print("Power ");
    acPower ? Serial.println("On") : Serial.println("Off");

    sendFrame();
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());

    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    server.on("/api", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println("GET");
        JsonDocument doc;
        doc["onOff"] = acPower;
        doc["mode"] = acMode;
        doc["fanSpeed"] = acFanSpeed;
        doc["currentTemp"] = acCurrentTemp;
        doc["setTemp"] = acSetTemp;
        doc["zone0"] = acZone0;
        doc["zone1"] = acZone1;
        doc["zone2"] = acZone2;
        doc["zone3"] = acZone3;
        doc["zone4"] = acZone4;
        doc["zone5"] = acZone5;
        String json;
        serializeJson(doc, json);

        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });

    server.on("/api", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(204); // No Content
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type");
        request->send(response);
    });

    server.on("/api", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
    {
        Serial.println("POST");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data);
        if (error)
        {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        bool rxPower = doc["onOff"];
        uint8_t rxMode = doc["mode"];
        uint8_t rxFanSpeed = doc["fanSpeed"];
        int rxSetTemp = doc["setTemp"];
        bool rxZone0 = doc["zone0"];
        bool rxZone1 = doc["zone1"];
        bool rxZone2 = doc["zone2"];
        bool rxZone3 = doc["zone3"];
        bool rxZone4 = doc["zone4"];
        bool rxZone5 = doc["zone5"];

        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"Status\":\"OK\"}");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);

        if (rxPower != acPower)
        {
            acPower = rxPower;
            setPower();
        }

        if (rxMode != acMode)
        {
            acMode = rxMode;
            setMode();
        }

        if (rxFanSpeed != acFanSpeed)
        {
            acFanSpeed = rxFanSpeed;
            setFanSpeed();
        }

        if (rxSetTemp != acSetTemp)
        {
            acSetTemp = rxSetTemp;
            setTemperature();
        }

        if (rxZone0 != acZone0)
        {
            acZone0 = rxZone0;
            setZone(0);
        }

        if (rxZone1 != acZone1)
        {
            acZone1 = rxZone1;
            setZone(1);
        }

        if (rxZone2 != acZone2)
        {
            acZone2 = rxZone2;
            setZone(2);
        }

        if (rxZone3 != acZone3)
        {
            acZone3 = rxZone3;
            setZone(3);
        }

        if (rxZone4 != acZone4)
        {
            acZone4 = rxZone4;
            setZone(4);
        }

        if (rxZone5 != acZone5)
        {
            acZone5 = rxZone5;
            setZone(5);
        }
    });

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    server.begin();

    Serial.println("Initializing CAN...");

    // Set up CAN configuration
    ESP32Can.setPins(CAN_TX, CAN_RX);      // Set TX and RX pins
    ESP32Can.setSpeed(TWAI_SPEED_125KBPS); // Set CAN speed to 500 Kbps
    ESP32Can.begin();                      // Initialize CAN interface

    Serial.println("CAN Initialized");

    outgoingFrame.extd = 1;
    outgoingFrame.flags = 1;
    outgoingFrame.data_length_code = 8;
}

void loop()
{
    // Check if a CAN frame is received
    CanFrame incoming;
    if (ESP32Can.readFrame(incoming))
    {
        processReceivedFrame(incoming);

        JsonDocument doc;
        doc["onOff"] = acPower;
        doc["mode"] = acMode;
        doc["fanSpeed"] = acFanSpeed;
        doc["currentTemp"] = acCurrentTemp;
        doc["setTemp"] = acSetTemp;
        doc["zone0"] = acZone0;
        doc["zone1"] = acZone1;
        doc["zone2"] = acZone2;
        doc["zone3"] = acZone3;
        doc["zone4"] = acZone4;
        doc["zone5"] = acZone5;
        String json;
        serializeJson(doc, json);
        ws.textAll(json);
    }
}

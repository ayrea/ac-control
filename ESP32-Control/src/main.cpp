#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp> // Include the CAN library
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include "ESPAsyncDNSServer.h"
#include <ESPAsync_WiFiManager.h>
#include <Preferences.h>

// NVS namespace/key for "enter WiFi config portal on next boot" (set by POST /config)
static const char *const kPrefsNamespace = "app";
static const char *const kPrefsConfigMode = "configMode";

// SSID broadcast by the config portal SoftAP, and the hostname used once joined
static const char *const kConfigPortalSsid = "AC-Control";
static const char *const kHostname = "AC-Control";

// Define CAN interface pins (use your specific ESP32 pins)
#define CAN_TX 5
#define CAN_RX 4

// Momentary button to GND; held during startup to force the WiFi config portal.
// Keep holding past the reset threshold to also clear stored credentials.
#define CONFIG_BUTTON_PIN 13
#define CONFIG_BUTTON_RESET_HOLD_MS 10000

#define FRAME_DATETIME 0x4040001
#define FRAME_CURRENT_TEMP 0x414000B

// Zone frames are laid out on a fixed stride: zone N (1-based) is base + (N - 1) * stride.
// The name arrives as a pair of consecutive identifiers holding 8 ASCII bytes each.
#define MAX_ZONES 14
#define ZONE_FRAME_STRIDE 0x2000
#define FRAME_ZONE_BASE 0x140C0003      // on/off, zone 1
#define FRAME_ZONE_NAME_BASE 0x04040032 // name part A, zone 1
#define ZONE_NAME_MAX_LEN 16

// Zones reported until the AC system next broadcasts its zone names
#define DEFAULT_ZONE_COUNT 6

#define FRAME_SET_TEMP 0x140C0017
#define FRAME_FAN_SPEED 0x140C0015
#define FRAME_MODE 0x140C0014
#define FRAME_POWER 0x140C0013

AsyncWebServer server(80);
AsyncDNSServer dnsServer;
AsyncWebSocket ws("/ws");

Preferences preferences;

struct ZoneInfo
{
    char name[ZONE_NAME_MAX_LEN + 1];
    bool enabled;
    bool present;   // part B was not FF x8
    bool nameKnown; // both halves decoded
};

static float acCurrentTemp = 24.0;
static ZoneInfo acZones[MAX_ZONES];
static bool anyZoneNameKnown = false;
static int acSetTemp = 24;
static uint8_t acFanSpeed = 1;
static uint8_t acMode = 1;
static bool acPower = false;

static bool stateDirty = false;

static CanFrame outgoingFrame;

// Returns the zone index for an on/off frame, or -1 when the identifier is not one
int zoneIndexFromControlFrame(uint32_t identifier)
{
    if (identifier < FRAME_ZONE_BASE)
    {
        return -1;
    }

    uint32_t delta = identifier - FRAME_ZONE_BASE;
    if (delta % ZONE_FRAME_STRIDE != 0)
    {
        return -1;
    }

    uint32_t index = delta / ZONE_FRAME_STRIDE;
    return index < MAX_ZONES ? (int)index : -1;
}

// Returns the zone index for a name frame, or -1 when the identifier is not one.
// isPartB reports whether this is the second half of the name.
int zoneIndexFromNameFrame(uint32_t identifier, bool &isPartB)
{
    if (identifier < FRAME_ZONE_NAME_BASE)
    {
        return -1;
    }

    uint32_t delta = identifier - FRAME_ZONE_NAME_BASE;
    uint32_t remainder = delta % ZONE_FRAME_STRIDE;
    if (remainder > 1)
    {
        return -1;
    }

    uint32_t index = delta / ZONE_FRAME_STRIDE;
    if (index >= MAX_ZONES)
    {
        return -1;
    }

    isPartB = remainder == 1;
    return (int)index;
}

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

// Copies 8 name bytes into the buffer, substituting a space for anything unprintable
// so a corrupt frame cannot break JSON serialization
void copyNameBytes(char *dest, const uint8_t *source)
{
    for (int i = 0; i < 8; i++)
    {
        uint8_t value = source[i];
        if (value == 0)
        {
            dest[i] = '\0';
        }
        else if (value < 0x20 || value > 0x7E)
        {
            dest[i] = ' ';
        }
        else
        {
            dest[i] = (char)value;
        }
    }
}

bool isUnusedZoneMarker(const CanFrame &frame)
{
    for (int i = 0; i < 8; i++)
    {
        if (frame.data[i] != 0xFF)
        {
            return false;
        }
    }
    return true;
}

void processZoneNameFrame(CanFrame &frame, int zoneIndex, bool isPartB)
{
    ZoneInfo &zone = acZones[zoneIndex];

    if (!isPartB)
    {
        copyNameBytes(zone.name, frame.data);
        return;
    }

    anyZoneNameKnown = true;
    zone.nameKnown = true;
    stateDirty = true;

    if (isUnusedZoneMarker(frame))
    {
        zone.present = false;
        zone.name[0] = '\0';
        Serial.print("Zone ");
        Serial.print(zoneIndex + 1);
        Serial.println(" unused");
        return;
    }

    copyNameBytes(zone.name + 8, frame.data);
    zone.name[ZONE_NAME_MAX_LEN] = '\0';
    zone.present = true;

    Serial.print("Zone ");
    Serial.print(zoneIndex + 1);
    Serial.print(" name ");
    Serial.println(zone.name);
}

void processReceivedFrame(CanFrame &frame)
{
    bool printFrameData = false;

    bool isNamePartB = false;
    int nameZoneIndex = zoneIndexFromNameFrame(frame.identifier, isNamePartB);
    if (nameZoneIndex >= 0)
    {
        processZoneNameFrame(frame, nameZoneIndex, isNamePartB);
        displayFrame(frame, false);
        return;
    }

    int controlZoneIndex = zoneIndexFromControlFrame(frame.identifier);
    if (controlZoneIndex >= 0)
    {
        bool enabled = frame.data[0] == 1;
        if (acZones[controlZoneIndex].enabled != enabled)
        {
            acZones[controlZoneIndex].enabled = enabled;
            stateDirty = true;
        }

        Serial.print("Zone ");
        Serial.print(controlZoneIndex + 1);
        Serial.print(" ");
        Serial.println(enabled);
        displayFrame(frame, false);
        return;
    }

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
        float currentTemp = (float)binTemp / 100.0;
        stateDirty = stateDirty || acCurrentTemp != currentTemp;
        acCurrentTemp = currentTemp;
        Serial.print("Current Temperature ");
        Serial.print(acCurrentTemp, 1);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_SET_TEMP)
    {
        int setTemp = ((frame.data[1] << 8) + frame.data[0]) / 100;
        stateDirty = stateDirty || acSetTemp != setTemp;
        acSetTemp = setTemp;
        Serial.print("Set Temperature ");
        Serial.println(acSetTemp);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_FAN_SPEED)
    {
        stateDirty = stateDirty || acFanSpeed != frame.data[0];
        acFanSpeed = frame.data[0];
        Serial.print("Fan Speed ");
        Serial.println(acFanSpeed);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_MODE)
    {
        stateDirty = stateDirty || acMode != frame.data[0];
        acMode = frame.data[0];
        Serial.print("Mode ");
        Serial.println(acMode);
        printFrameData = true;
    }

    else if (frame.identifier == FRAME_POWER)
    {
        bool power = frame.data[0] == 1;
        stateDirty = stateDirty || acPower != power;
        acPower = power;
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

void setZone(uint8_t zoneIndex)
{
    if (zoneIndex >= MAX_ZONES)
    {
        return;
    }

    outgoingFrame.identifier = FRAME_ZONE_BASE + zoneIndex * ZONE_FRAME_STRIDE;
    outgoingFrame.data[0] = acZones[zoneIndex].enabled ? 1 : 2;

    outgoingFrame.data[1] = 9;
    outgoingFrame.data[2] = 0;
    outgoingFrame.data[3] = 0;
    outgoingFrame.data[4] = 0;
    outgoingFrame.data[5] = 0;
    outgoingFrame.data[6] = 0;
    outgoingFrame.data[7] = 0;

    Serial.print("Setting Zone ");
    Serial.println(zoneIndex + 1);

    sendFrame();
}

// Serializes the current state. Until the AC system broadcasts its zone names the
// zone list falls back to DEFAULT_ZONE_COUNT placeholders.
String buildStateJson()
{
    JsonDocument doc;
    doc["onOff"] = acPower;
    doc["mode"] = acMode;
    doc["fanSpeed"] = acFanSpeed;
    doc["currentTemp"] = acCurrentTemp;
    doc["setTemp"] = acSetTemp;

    JsonArray zones = doc["zones"].to<JsonArray>();

    if (!anyZoneNameKnown)
    {
        for (int i = 0; i < DEFAULT_ZONE_COUNT; i++)
        {
            JsonObject zone = zones.add<JsonObject>();
            zone["id"] = i + 1;
            zone["name"] = String("Zone ") + String(i + 1);
            zone["enabled"] = acZones[i].enabled;
        }
    }
    else
    {
        for (int i = 0; i < MAX_ZONES; i++)
        {
            if (!acZones[i].present)
            {
                continue;
            }

            JsonObject zone = zones.add<JsonObject>();
            zone["id"] = i + 1;
            zone["name"] = acZones[i].name;
            zone["enabled"] = acZones[i].enabled;
        }
    }

    String json;
    serializeJson(doc, json);
    return json;
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

    for (int i = 0; i < MAX_ZONES; i++)
    {
        acZones[i].name[0] = '\0';
        acZones[i].enabled = true;
        acZones[i].present = false;
        acZones[i].nameKnown = false;
    }

    // --- Phase 1: WiFi only (no SPIFFS, web server, or CAN until connected) ---
    WiFi.mode(WIFI_STA);

    pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);
    delay(50); // debounce and let the pull-up settle

    bool buttonHeld = digitalRead(CONFIG_BUTTON_PIN) == LOW;
    bool factoryReset = false;

    if (buttonHeld)
    {
        Serial.println("Config button held - keep holding 10s to clear stored WiFi credentials");

        unsigned long holdStart = millis();
        while (digitalRead(CONFIG_BUTTON_PIN) == LOW)
        {
            if (millis() - holdStart >= CONFIG_BUTTON_RESET_HOLD_MS)
            {
                factoryReset = true;
                break;
            }
            delay(50);
        }
    }

    preferences.begin(kPrefsNamespace, true);
    bool configMode = preferences.getBool(kPrefsConfigMode, false) || buttonHeld;
    preferences.end();

    ESPAsync_WiFiManager wifiManager(&server, &dnsServer, kHostname);

    // Must run before the stored credentials are read below
    if (factoryReset)
    {
        Serial.println("Clearing stored WiFi credentials");
        wifiManager.resetSettings();
    }

    String storedSsid = wifiManager.WiFi_SSID();
    String storedPass = wifiManager.WiFi_Pass();

    if (configMode)
    {
        Serial.print("Config mode requested (");
        Serial.print(buttonHeld ? "button" : "NVS");
        Serial.println("). Skipping stored WiFi connect; will start portal.");
    }

    if (!configMode && storedSsid.length() > 0)
    {
        Serial.print("Connecting to stored WiFi: ");
        Serial.println(storedSsid);

        WiFi.begin(storedSsid.c_str(), storedPass.c_str());

        const unsigned long connectTimeoutMs = 15000;
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < connectTimeoutMs)
        {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
    }

    if (configMode || WiFi.status() != WL_CONNECTED)
    {
        if (configMode)
        {
            Serial.print("Starting config portal on request. SSID: ");
        }
        else
        {
            Serial.print("WiFi not available. Starting config portal. SSID: ");
        }
        Serial.println(kConfigPortalSsid);

        wifiManager.setSaveConfigCallback([]()
                                          {
                                                Serial.println("WiFi credentials saved. Clearing config flag and restarting...");
                                                preferences.begin(kPrefsNamespace, false);
                                                preferences.remove(kPrefsConfigMode);
                                                preferences.end();
                                                delay(100);
                                                ESP.restart(); });

        if (!wifiManager.startConfigPortal(kConfigPortalSsid))
        {
            Serial.println("Config portal exited without connection. Restarting to retry...");
            delay(500);
            ESP.restart();
        }
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected. Restarting...");
        delay(500);
        ESP.restart();
    }

    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());

    // --- Phase 2: normal application (SPIFFS, web UI, CAN) ---
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    server.on("/api", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        Serial.println("GET");
        String json = buildStateJson();

        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response); });

    server.on("/api", HTTP_OPTIONS, [](AsyncWebServerRequest *request)
              {
        AsyncWebServerResponse *response = request->beginResponse(204); // No Content
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type");
        request->send(response); });

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

        for (JsonObject rxZone : doc["zones"].as<JsonArray>())
        {
            int id = rxZone["id"] | 0;
            if (id < 1 || id > MAX_ZONES)
            {
                continue;
            }

            uint8_t zoneIndex = (uint8_t)(id - 1);
            bool enabled = rxZone["enabled"];

            if (enabled != acZones[zoneIndex].enabled)
            {
                acZones[zoneIndex].enabled = enabled;
                setZone(zoneIndex);
            }
        } });

    server.on("/config", HTTP_OPTIONS, [](AsyncWebServerRequest *request)
              {
        AsyncWebServerResponse *response = request->beginResponse(204); // No Content
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type");
        request->send(response); });

    server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        preferences.begin(kPrefsNamespace, false);
        preferences.putBool(kPrefsConfigMode, true);
        preferences.end();
        Serial.println("POST /config: configMode set; restarting...");
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"status\":\"OK\"}");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        delay(500);
        ESP.restart(); });

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

        // Only push when something actually changed, otherwise the burst of zone
        // name frames at AC startup would flood the socket
        if (stateDirty)
        {
            stateDirty = false;
            ws.textAll(buildStateJson());
        }
    }
}

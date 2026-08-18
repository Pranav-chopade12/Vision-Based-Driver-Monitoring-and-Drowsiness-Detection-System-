

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "webpage.h"   // Dashboard HTML (PROGMEM)

// ============================================================
//  ORIGINAL CODE — COMPLETELY UNCHANGED
// ============================================================
#define RXD2 16  // GPIO16 (RX from STM32 TX)
#define TXD2 17  // GPIO17 (TX to STM32 RX)

volatile char driver_state = 'W';
// ============================================================

// ============================================================
//  WiFi Credentials — EDIT THESE
// ============================================================
const char* WIFI_SSID     = "hotspot";
const char* WIFI_PASSWORD = "12345678";
// ============================================================

// Dashboard state tracking (additions only — original not touched)
char     prev_state    = 'W';
uint32_t wakeCount     = 0;
uint32_t drowsyCount   = 0;
uint32_t criticalCount = 0;
uint32_t stateStartMs  = 0;

WebServer        httpServer(80);
WebSocketsServer wsServer(81);

// ============================================================
//  Broadcast current state to all WebSocket clients as JSON
// ============================================================
void sendStateUpdate() {
    StaticJsonDocument<256> doc;
    doc["state"]       = String(driver_state);
    doc["wakeCount"]   = wakeCount;
    doc["drowsyCount"] = drowsyCount;
    doc["alertCount"]  = criticalCount;
    doc["duration"]    = (millis() - stateStartMs) / 1000UL;

    String payload;
    serializeJson(doc, payload);
    wsServer.broadcastTXT(payload);
}

// ============================================================
//  WebSocket event handler
// ============================================================
void onWebSocketEvent(uint8_t num, WStype_t type,
                      uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            Serial.printf("[WS] Client #%u connected\n", num);
            sendStateUpdate();   // push current state immediately
            break;
        case WStype_DISCONNECTED:
            Serial.printf("[WS] Client #%u disconnected\n", num);
            break;
        default:
            break;
    }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
    // ── ORIGINAL SETUP CODE (UNCHANGED) ───────────────────
    Serial.begin(115200);  // USB Serial (for debugging)

    // UART2 initialization for STM32 communication
    Serial.println("Initializing UART2...");
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

    // LED pins
    pinMode(2, OUTPUT);   // GPIO2 - Green LED
    pinMode(4, OUTPUT);   // GPIO4 - Red LED
    digitalWrite(2, LOW);
    digitalWrite(4, LOW);

    Serial.println("\nESP32 WROOM32 UART Ready!");
    Serial.println("Waiting for STM32 data on UART2...\n");
    // ── END ORIGINAL SETUP CODE ────────────────────────────

    // ── WiFi Setup ─────────────────────────────────────────
        // ── WiFi Setup ─────────────────────────────────────────
    WiFi.disconnect(true);
    delay(200);
    WiFi.mode(WIFI_STA);
    Serial.printf("Connecting to WiFi: %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint8_t attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        Serial.print("  Dashboard  -> http://");
        Serial.println(WiFi.localIP());
        Serial.print("  WebSocket  -> ws://");
        Serial.print(WiFi.localIP());
        Serial.println(":81");

        // Serve dashboard HTML at GET /
        httpServer.on("/", HTTP_GET, []() {
            httpServer.send_P(200, "text/html", DASHBOARD_HTML);
        });
        httpServer.begin();
        Serial.println("  HTTP server  started (port 80)");

        wsServer.begin();
        wsServer.onEvent(onWebSocketEvent);
        Serial.println("  WebSocket    started (port 81)");
    } else {
        Serial.println("\n[WARN] WiFi failed - dashboard unavailable");
        Serial.println("       Check SSID/password and restart");
    }

    stateStartMs = millis();
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
    // Handle HTTP and WebSocket clients
    if (WiFi.status() == WL_CONNECTED) {
        httpServer.handleClient();
        wsServer.loop();
    }

    // ── ORIGINAL LOOP CODE (UNCHANGED) ────────────────────
    // Check for data on UART2 (from STM32)
    if (Serial2.available() >= 3) {
        uint8_t byte1 = Serial2.read();

        if (byte1 == 0xAA) {
            uint8_t state = Serial2.read();
            uint8_t byte3 = Serial2.read();

            if (byte3 == 0x55) {
                driver_state = state;

                // Debug print on USB Serial
                Serial.print("Received from STM32: ");
                Serial.println(driver_state);

                // LED Control
                if (driver_state == 'W') {
                    digitalWrite(2, LOW);
                    digitalWrite(4, LOW);
                    Serial.println("Status: NORMAL (Wide Awake)");
                }
                else if (driver_state == 'D') {
                    digitalWrite(2, LOW);
                    digitalWrite(4, HIGH);
                    Serial.println("Status: DROWSY");
                }
                else if (driver_state == 'A') {
                    digitalWrite(2, HIGH);
                    digitalWrite(4, HIGH);
                    Serial.println("Status: ALERT");
                }
                Serial.println("---");
            }
        }
    }
    // ── END ORIGINAL LOOP CODE ────────────────────────────

    // ── State-change tracking for dashboard ───────────────
    if (driver_state != prev_state) {
        stateStartMs = millis();
        if      (driver_state == 'W') wakeCount++;
        else if (driver_state == 'D') drowsyCount++;
        else if (driver_state == 'A') criticalCount++;
        prev_state = driver_state;

        if (WiFi.status() == WL_CONNECTED) {
            sendStateUpdate();   // instant push on state change
        }
    }

    // Periodic duration ping to dashboard (every 1 s)
    static uint32_t lastPing = 0;
    if (millis() - lastPing >= 1000UL) {
        lastPing = millis();
        if (WiFi.status() == WL_CONNECTED) {
            sendStateUpdate();
        }
    }

    delay(50);
}

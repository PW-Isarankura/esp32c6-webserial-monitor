#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WebServer.h>          // ใช้ตัวมาตรฐาน ไม่ Crash
#include <WebSocketsServer.h>   // ใช้ตัวที่เสถียรกับ Core 3.0

// ตัวแปร Config
String ssid = "";
String password = "";
int serverPort = 80;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81); // WebSocket แยกไป Port 81

// ฟังก์ชันโหลด Config (เหมือนเดิมของคุณ)
bool loadConfiguration() {
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) return false;
    JsonDocument doc;
    deserializeJson(doc, configFile);
    configFile.close();
    ssid = doc["wifi_ssid"] | "";
    password = doc["wifi_password"] | "";
    serverPort = doc["port"] | 80;
    return true;
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    if (!LittleFS.begin(false, "/littlefs", 10, "storage")) {
        Serial.println("LittleFS Failed!"); return;
    }

    if (!loadConfiguration()) return;

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500); Serial.print(".");
    }
    Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());

    // --- ตั้งค่า Web Server ---
    server.on("/", []() {
        File file = LittleFS.open("/index.html", "r");
        server.streamFile(file, "text/html");
        file.close();
    });

    server.begin();
    webSocket.begin();
    Serial.println("Server Started!");
}

void loop() {
    server.handleClient(); // จัดการ HTTP
    webSocket.loop();      // จัดการ WebSocket

    // อ่าน Serial แล้วพ่นออกหน้าเว็บ
    if (Serial.available()) {
        String msg = Serial.readStringUntil('\n');
        msg.trim();
        webSocket.broadcastTXT(msg); // ส่งหาทุก Client ที่ต่ออยู่
        Serial.println("Sent to Web: " + msg);
    }
}
#include <WiFi.h>
#include <WebServer.h>

// ====== WiFi Setup ======
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

// ====== Pin Definitions ======
#define IR_SLOT1 34     // IR sensor digital output
#define LED_RED 25
#define LED_GREEN 27
#define BUZZER 12
#define TRIG_PIN 5
#define ECHO_PIN 18

// ====== Variables ======
bool slot1 = false;

// ====== Helper Functions ======
long readUltrasonicCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

String getStatusJSON() {
  int freeSlots = slot1 ? 0 : 1;
  String json = "{";
  json += "\"slot1\":" + String(slot1 ? 1 : 0) + ",";
  json += "\"free\":" + String(freeSlots) + ",";
  json += "\"total\":1";
  json += "}";
  return json;
}

// ====== Web Dashboard HTML ======
void handleRoot() {
  String html = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Smart Parking Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; background: #f7f9fc; color: #333; }
    h1 { text-align: center; }
    .slot { padding: 20px; border-radius: 12px; width: 180px; margin: auto;
            text-align: center; font-size: 20px; font-weight: bold;
            box-shadow: 0 4px 10px rgba(0,0,0,0.1); }
    .free { background: #d4fcd4; }
    .occupied { background: #fcd4d4; }
    .info { text-align: center; margin-top: 20px; font-size: 20px; }
    .alert { text-align: center; margin-top: 20px; color: red; font-weight: bold; font-size: 22px; }
  </style>
</head>
<body>
  <h1>🚗 Smart Parking Dashboard</h1>
  <div id="slot1" class="slot">Loading...</div>
  <div class="info" id="info">Checking...</div>
  <div class="alert" id="alert"></div>

  <script>
    async function refresh() {
      try {
        const res = await fetch('/status');
        const data = await res.json();

        document.getElementById("slot1").textContent =
          data.slot1 ? "Slot: Occupied ❌" : "Slot: Free ✅";
        document.getElementById("slot1").className =
          "slot " + (data.slot1 ? "occupied" : "free");

        document.getElementById("info").textContent =
          Free Slots: ${data.free} / ${data.total};

        document.getElementById("alert").textContent =
          data.free === 0 ? "🚨 Parking Full!" : "";
      } catch (err) {
        console.error(err);
        document.getElementById("info").textContent = "❌ Error fetching data!";
      }
    }
    refresh();
    setInterval(refresh, 2000);
  </script>
</body>
</html>
)HTML";

  server.send(200, "text/html", html);
}

void handleStatus() {
  server.send(200, "application/json", getStatusJSON());
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);

  pinMode(IR_SLOT1, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP: ");
  Serial.println(WiFi.localIP());

  // Web server routes
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
}

// ====== Loop ======
void loop() {
  // Read IR sensor (LOW = occupied)
  slot1 = (digitalRead(IR_SLOT1) == LOW);

  // LEDs
  digitalWrite(LED_RED, slot1 ? HIGH : LOW);
  digitalWrite(LED_GREEN, slot1 ? LOW : HIGH);

  // Buzzer if occupied
  if (slot1) digitalWrite(BUZZER, HIGH);
  else digitalWrite(BUZZER, LOW);

  // Optional: ultrasonic at gate
  long dist = readUltrasonicCM();
  if (dist > 0 && dist < 15) {
    Serial.println("🚘 Car detected at entrance!");
  }

  server.handleClient();
}

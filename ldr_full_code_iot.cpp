#include <WiFi.h>
#include <PubSubClient.h>

#define PIR_PIN 14
#define LIGHT_SENSOR_PIN 26
#define RELAY_PIN 13

#define RED_PIN 25
#define GREEN_PIN 33
#define BLUE_PIN 32

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "demo.thingsboard.io";
const int mqtt_port = 1883;
const char* access_token = "boIQk98csjl1ujKEVOlL";

WiFiClient espClient;
PubSubClient client(espClient);

float onTimeSeconds = 0.0;
unsigned long lastUpdateTime = 0;
bool prevRelayStatus = false;

void setRGB(int r, int g, int b) {
  digitalWrite(RED_PIN, r);
  digitalWrite(GREEN_PIN, g);
  digitalWrite(BLUE_PIN, b);
}

void updateConnectionLED() {
  if (WiFi.status() != WL_CONNECTED) {
    setRGB(255, 0, 0);  // RED
  } else if (!client.connected()) {
    setRGB(0, 0, 255);  // BLUE
  } else {
    setRGB(0, 255, 0);  // GREEN
  }
}

void sendRelayStatus(bool status,  float time) {
  String payload = "{";
  payload += "\"relay_status\":";
  payload += status ? "true" : "false";
  payload += ",";
  payload += "\"on_time\":";
  payload += String(time, 3);
  payload += "}";

  Serial.println("Sending payload: " + payload);

  if (client.publish("v1/devices/me/telemetry", payload.c_str())) {
    Serial.println("Data sent to ThingsBoard");
  } else {
    Serial.println("Failed to send data");
  }
}

void connectToMQTT() {
  while (!client.connected()) {
    updateConnectionLED();  
    Serial.println("Connecting to ThingsBoard...");
    if (client.connect("ESP32Client", access_token, NULL)) {
      Serial.println("Connected to ThingsBoard!");
    } else {
      Serial.print("MQTT connect failed, rc=");
      Serial.print(client.state());
      Serial.println(". Trying again in 2s...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // RGB LED setup
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  setRGB(0, 0, 0); // Start with LED off

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    updateConnectionLED();  // RED while waiting
  }
  Serial.println("\nConnected to WiFi");

  client.setServer(mqtt_server, mqtt_port);
  connectToMQTT();

  lastUpdateTime = millis();
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  updateConnectionLED();  // Update RGB status in real-time
  unsigned long now = millis();

  bool pirState = digitalRead(PIR_PIN);
  int lightIntensity = analogRead(LIGHT_SENSOR_PIN);
  
  // Check light conditions
  bool relayState = pirState;
  if (lightIntensity < 500) {  // Assume 500 as the threshold for insufficient light
    analogWrite(RELAY_PIN, 255);  // Full intensity if light is not sufficient
  } else {
    analogWrite(RELAY_PIN, 127);  // Half intensity if sufficient light is present
  }

  float timeDiffSec = (now - lastUpdateTime) / 1000.0;

  if (relayState) {
    onTimeSeconds += timeDiffSec;
  }

  lastUpdateTime = now; 
  prevRelayStatus = relayState;

  sendRelayStatus(relayState, onTimeSeconds);
  delay(1000);
}

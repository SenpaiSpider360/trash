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

#include "AS5600.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include <ArduinoJson.h> // Recommended for easy JSON creation

// --- Network Configuration ---
// !! CHANGE THESE TO YOUR WI-FI CREDENTIALS !!
const char *ssid = "SophiaS24FE";
const char *password = "sophi123";

// !! CHANGE THIS TO THE IP ADDRESS OF THE COMPUTER RUNNING SERVER.PY !!
// If the server is running on port 5000, the URL is:
const char* serverUrl = "http://192.168.234.186:5000/data"; 

// --- Sensor Configuration ---
AS5600 as5600;

const long postInterval = 10; // Post data every 1000 milliseconds (1 second)
unsigned long lastPostTime = 0;

/**
 * @brief Connects the ESP32 to the configured Wi-Fi network.
 */
void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to Wi-Fi. Check credentials/range.");
    // Optionally enter deep sleep or restart here
  }
}

/**
 * @brief Sends the current RPM data to the server via HTTP POST.
 * @param rpm The measured RPM value.
 */
void postData(float rpm) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting to WiFi...");
    connectToWiFi();
    return;
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  // Create JSON payload
  StaticJsonDocument<100> doc;
  doc["rpm"] = rpm;
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);

  Serial.print("Posting data: ");
  Serial.println(jsonPayload);

  // Send the request
  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0) {
    Serial.printf("[HTTP] POST... code: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.printf("[HTTP] POST failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  http.end(); // Free resources
}

void setup()
{
  Serial.begin(115200);

  // --- AS5600 Setup ---
  Wire.begin(21, 22); // I2C SDA=21, SCL=22 (typical for ESP32)
  as5600.begin(23); // Direction pin (GPIO 23)
  as5600.setDirection(AS5600_CLOCK_WISE);
  as5600.setZPosition(0);
  as5600.setMPosition(4095);
  as5600.resetCumulativePosition();
  delay(1000); // Wait for sensor init

  // --- Wi-Fi Setup ---
  connectToWiFi();
}

void loop()
{
  unsigned long now = millis();

  if (now - lastPostTime >= postInterval) {
    // Read the RPM value. Use 'true' to ensure a fresh read
    float rpm = as5600.getAngularSpeed(AS5600_MODE_RPM, true); 

    // Send the data to the server
    postData(rpm);

    // Update the timer
    lastPostTime = now;
  }
}
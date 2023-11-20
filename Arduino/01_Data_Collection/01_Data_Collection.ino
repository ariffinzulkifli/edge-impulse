#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_MPU6050.h>

// Replace with your network credentials
const char* ssid = "myinvententerprise";
const char* password = "04222682";

// Replace with the IP address of your laptop and the Flask port
const char* serverName = "http://192.168.0.242:5050/data-catcher";

Adafruit_MPU6050 mpu;

int repeat = 0;
int total_data = 3;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  // Wait for connection
  Serial.println("\n\nConnecting to Wi-Fi ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected to <" + String(ssid) + ">\n");

  Serial.println("Say hello to Data-Catcher server ...");

  // Check connection to Flask API
  HTTPClient http;
  http.begin("http://192.168.0.242:5050/hello"); // Adjust URL as necessary
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response: " + String(httpResponseCode) + ", " + response);
  } else {
    Serial.println("Failed to connect to Flask server. Halting data collection!");
    http.end();
    while(1); // Stop further execution
  }

  http.end();
}

void loop() {
  // Start time for the total batch
  unsigned long totalBatchStartTime = millis();

  // Check for Serial command
  if (Serial.available()) {
    String serialInput = Serial.readStringUntil('\n'); // Read the incoming data as a String until newline
    serialInput.trim(); // Remove any leading/trailing whitespace
    if (serialInput.equals("START")) {
      repeat = 0; // Reset repeat counter if START command is received
      Serial.println("Received START command. Resetting data collection.");
    }
  }

  while (repeat < total_data) {

    // Capture the start time of the batch
    unsigned long batchStartTime = millis();

    Serial.println("Collecting Data #" + String(repeat + 1));
    String payload = "{\"data\":[";

    // Collect data for 10 seconds at 100Hz (1000 samples)
    for (int i = 0; i < 1000; ++i) {
      unsigned long timestamp = millis() - batchStartTime;

      // Read accelerometer and gyroscope data
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);

      // Append data to payload
      payload += "[" + String(timestamp) + "," + String(a.acceleration.x) + "," + String(a.acceleration.y) + "," + String(a.acceleration.z);
      payload += "," + String(g.gyro.x) + "," + String(g.gyro.y) + "," + String(g.gyro.z) + "]";
      if (i < 999) payload += ",";

      delay(10); // Delay for 10 milliseconds for 100Hz sampling rate
    }

    payload += "]}";

    // Send data to Flask API
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "application/json");
      
      int httpResponseCode = http.POST(payload);
      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.print("HTTP Response: " + String(httpResponseCode) + ", " + response);
      } else {
        Serial.println("HTTP Error!");
      }

      http.end();
    }

    unsigned long batchEndTime = millis(); // End time for each data collection
    unsigned long batchDuration = batchEndTime - batchStartTime;
    Serial.print("Time taken: ");
    Serial.print(batchDuration / 60000); // Minutes
    Serial.print(" minutes ");
    Serial.print((batchDuration % 60000) / 1000); // Seconds
    Serial.println(" seconds\n");

    repeat++;

    if(repeat == total_data){
      unsigned long totalBatchEndTime = millis(); // End time for the total batch
      unsigned long totalDuration = totalBatchEndTime - totalBatchStartTime;
      Serial.print("Total time taken: ");
      Serial.print(totalDuration / 60000); // Minutes
      Serial.print(" minutes ");
      Serial.print((totalDuration % 60000) / 1000); // Seconds
      Serial.println(" seconds\n\n\n");
    }
  }
}

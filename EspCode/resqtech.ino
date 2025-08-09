/*********************************************************************************************************************
 * Description: This sketch reads data from DHT11 (Temperature & Humidity), a flame sensor, and an MQ-2 gas sensor.
 * It then sends these values to a Firebase Realtime Database using an ESP8266 (NodeMCU).
 *
 * THIS VERSION USES THE Firebase_ESP_Client.h and dht11.h LIBRARIES.
 *
 * Dependencies:
 * 1. ESP8266WiFi Library (comes with ESP8266 board support)
 * 2. Firebase ESP Client Library by Mobizt: https://github.com/mobizt/Firebase-ESP-Client
 * - In Arduino IDE, go to Sketch > Include Library > Manage Libraries... and search for "Firebase ESP Client".
 * 3. dht11 library by Dhruba Saha:
 * - In Arduino IDE, search for "dht11" by Dhruba Saha and install it.
 *
 * Hardware:
 * - ESP8266 (NodeMCU)
 * - DHT11 Temperature and Humidity Sensor
 * - Flame Sensor (with a digital output pin, DO)
 * - MQ-2 Gas Sensor (with an analog output pin, A0)
 *
 * Wiring:
 * - DHT11 Data Pin -> D4 on NodeMCU
 * - Flame Sensor DO Pin -> D5 on NodeMCU
 * - MQ-2 Sensor A0 Pin -> A0 on NodeMCU
 * - Connect VCC and GND of all sensors appropriately (e.g., to 3.3V/5V and GND on NodeMCU).
 *********************************************************************************************************************/

// --- Library Includes ---
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "DHT.h"
// Provide the token generation process info.
#include "addons/TokenHelper.h"

// --- Firebase Configuration ---
// IMPORTANT: Replace with your Firebase Project API Key, Database URL, and User Credentials.
#define WIFI_SSID "FTTH-bsnl"
#define WIFI_PASSWORD "adavichira@1"
#define API_KEY "AIzaSyCwbAvPWJA9iPpJXFyb5osGHD001lT2ve0"
#define DATABASE_URL "resqtech-929a9-default-rtdb.firebaseio.com"
#define USER_EMAIL "andrewjos2004@gmail.com"
#define USER_PASSWORD "andrew@1"

// --- Sensor Pin Definitions ---
#define DHTPIN D4           // Digital pin connected to the DHT sensor
#define FLAME_PIN D5       // Digital pin connected to the flame sensor's DO
#define GAS_SENSOR_PIN A0  // Analog pin connected to the gas sensor's A0

#define DHTTYPE DHT11

// --- Global Object Declarations ---
DHT dht(DHTPIN, DHTTYPE);

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to hold the authentication status
bool firebase_auth_ok = false;

void setup() {
  // Start serial communication for debugging purposes
  Serial.begin(115200);
  Serial.println("\n--- ESP8266 Firebase Sensor Monitor (Firebase_ESP_Client) ---");

  // The dht11 library does not require a setup() call.

  // Set sensor pin modes
  pinMode(FLAME_PIN, INPUT);  // Flame sensor will provide an input signal

  // --- WiFi Connection ---
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected! IP Address: ");
  Serial.println(WiFi.localIP());

  // --- Firebase Initialization ---
  // Assign the API key
  config.api_key = API_KEY;

  // Assign the user account credentials for authentication
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL
  config.database_url = DATABASE_URL;

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  // Initialize Firebase with the config and auth objects
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  dht.begin();
}

void loop() {
  // --- Check Firebase Authentication and Readiness ---
  if (Firebase.ready() && !firebase_auth_ok) {
    // This block runs once after Firebase is initialized and authenticated.
    firebase_auth_ok = true;
    Serial.println("Firebase is ready and authenticated.");
  }

  // Don't proceed if Firebase is not ready
  if (!firebase_auth_ok) {
    Serial.println("Waiting for Firebase authentication...");
    delay(2000);
    return;
  }

  // --- Read Sensor Data ---

  float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float temperature = dht.readTemperature();


  // Read Flame Sensor (Digital Output)
  int flame_detected = !digitalRead(FLAME_PIN);

  // Read Gas Sensor (Analog Output)
  int gas_value = analogRead(GAS_SENSOR_PIN);

  // --- Print Sensor Values to Serial Monitor (for debugging) ---
  Serial.println("--------------------");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C");
  Serial.print("Flame Detected: ");
  Serial.println(flame_detected == 1 ? "YES" : "NO");
  Serial.print("Gas Sensor Value: ");
  Serial.println(gas_value);
  Serial.println("--------------------");


  // --- Prepare and Send Data to Firebase ---


  // Set Temperature
  if (Firebase.RTDB.setFloat(&fbdo, "/sensor_data/temperature", temperature)) {
    Serial.println("-> Temperature sent OK");
  } else {
    Serial.println("-> ERROR sending temperature: " + fbdo.errorReason());
  }

  // Set Humidity
  if (Firebase.RTDB.setFloat(&fbdo, "/sensor_data/humidity", humidity)) {
    Serial.println("-> Humidity sent OK");
  } else {
    Serial.println("-> ERROR sending humidity: " + fbdo.errorReason());
  }

  // Set Flame Detection
  if (Firebase.RTDB.setInt(&fbdo, "/sensor_data/flame_detected", flame_detected)) {
    Serial.println("-> Flame status sent OK");
  } else {
    Serial.println("-> ERROR sending flame status: " + fbdo.errorReason());
  }

  // Set Gas Level
  if (Firebase.RTDB.setInt(&fbdo, "/sensor_data/gas_level", gas_value)) {
    Serial.println("-> Gas level sent OK");
  } else {
    Serial.println("-> ERROR sending gas level: " + fbdo.errorReason());
  }

  // Wait for 5 seconds before sending the next batch of data.
  delay(5000);
}

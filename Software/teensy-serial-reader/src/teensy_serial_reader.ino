#include <Arduino.h>

// --- Port Definitions ---
// Define the serial port for receiving the sensor data stream
#define SERIAL_IN Serial2
// Define the serial port for the LTE modem
// MODIFIED: Changed from Serial3 to Serial1 as requested.
// NOTE: For Teensy 4.0, Serial1 is on pins 1 (TX) and 0 (RX).
#define SERIAL_LTE Serial1

/**
 * @file teensy40_serial_parser_mqtt.ino
 * @author Your Name
 * @brief This program uses a non-blocking state machine in the main loop to
 * parse sensor data and send it via MQTT without interruption.
 * @version 14.1 - Re-assigned LTE modem to Serial1
 * @date 2025-06-10
 */

// --- Protocol and Timing Constants ---
const char* SYNC_KEYWORD = "uinject";
const int KEYWORD_LEN = 7;
const int PAYLOAD_LEN = 9; // 2 (data) + 2 (ID) + 5 (counter)

const long SEND_INTERVAL = 10000; // 10 seconds in milliseconds
unsigned long previousSendTime = 0;

// --- Data Structure ---
struct MessageData {
  byte id[2];
  uint16_t data;
  uint64_t counter;
};

// --- Global Variables for State Machine ---
const int MAX_IDS = 20;
MessageData dataStore[MAX_IDS];
int dataStoreCount = 0;

enum ParserState { SEARCHING_FOR_KEYWORD, READING_PAYLOAD };
ParserState currentState = SEARCHING_FOR_KEYWORD;
int keywordIndex = 0;
int payloadIndex = 0;
byte payloadBuffer[PAYLOAD_LEN];


// --- Function Prototypes ---
void parsePayload();
void updateDataStore(const byte id[2], uint16_t data, uint64_t counter);
void printDataStore();
void sendDataOverLTE();
void readLTESerial();

//==============================================================================
// 1. SETUP: Initialize Serial Ports
//==============================================================================
void setup() {
  Serial.begin(115200);
  while (!Serial) { } // Wait for USB Serial

  SERIAL_IN.begin(115200);
  SERIAL_LTE.begin(115200); // Initialize LTE modem serial port
  SERIAL_IN.setTimeout(10); // Reduce timeout as we read byte-by-byte

  Serial.println("\n--- RUNNING VERSION 14.1: Corrected State Machine ---");
  Serial.println("Continuously searching for 'uinject' keyword on Serial2...");
  Serial.println("LTE Modem is on Serial1.");
}

//==============================================================================
// 2. MAIN LOOP: Non-Blocking State Machine
//==============================================================================
void loop() {
  // --- Task 1: Process one character of incoming data, if available ---
  if (SERIAL_IN.available() > 0) {
    char c = SERIAL_IN.read();

    if (currentState == SEARCHING_FOR_KEYWORD) {
      if (c == SYNC_KEYWORD[keywordIndex]) {
        keywordIndex++; // Matched a character, advance
        if (keywordIndex >= KEYWORD_LEN) {
          // Full keyword found! Switch state to read the payload.
          currentState = READING_PAYLOAD;
          payloadIndex = 0;
          keywordIndex = 0; // Reset for the next search
        }
      } else {
        keywordIndex = 0; // Mismatch, reset the search
      }
    } 
    else if (currentState == READING_PAYLOAD) {
      payloadBuffer[payloadIndex++] = c;
      if (payloadIndex >= PAYLOAD_LEN) {
        // We have received the full payload
        parsePayload();
        // Reset state to search for the next keyword
        currentState = SEARCHING_FOR_KEYWORD;
      }
    }
  }

  // --- Task 2: Check the timer for sending data (runs on every loop) ---
  if (millis() - previousSendTime >= SEND_INTERVAL) {
    previousSendTime = millis();
    sendDataOverLTE();
  }
}

//==============================================================================
// 3. CORE LOGIC
//==============================================================================

/**
 * @brief Parses the data in the global payloadBuffer.
 * This is only called after a full payload has been received.
 */
void parsePayload() {
  uint16_t dataValue = ((uint16_t)payloadBuffer[1] << 8) | (uint16_t)payloadBuffer[0];
  byte idValue[2] = {payloadBuffer[2], payloadBuffer[3]};
  uint64_t counterValue = 0;
  for (int i = 4; i >= 0; --i) {
    counterValue = (counterValue << 8) | (uint64_t)payloadBuffer[4 + i];
  }
  updateDataStore(idValue, dataValue, counterValue);
  printDataStore();
}

void updateDataStore(const byte id[2], uint16_t data, uint64_t counter) {
  for (int i = 0; i < dataStoreCount; ++i) {
    if (memcmp(dataStore[i].id, id, 2) == 0) {
      dataStore[i].data = data;
      dataStore[i].counter = counter;
      return;
    }
  }
  if (dataStoreCount < MAX_IDS) {
    memcpy(dataStore[dataStoreCount].id, id, 2);
    dataStore[dataStoreCount].data = data;
    dataStore[dataStoreCount].counter = counter;
    dataStoreCount++;
  }
}

void printDataStore() {
  Serial.println("\n--- Parsed Packet Data ---");
  if (dataStoreCount > 0) {
    for (int i=0; i < dataStoreCount; i++) {
        Serial.print("  ID: ");
        if (dataStore[i].id[0] < 0x10) Serial.print("0");
        Serial.print(dataStore[i].id[0], HEX);
        Serial.print(":");
        if (dataStore[i].id[1] < 0x10) Serial.print("0");
        Serial.print(dataStore[i].id[1], HEX);
        Serial.print("\t | Data: ");
        Serial.println(dataStore[i].data);
    }
  }
  Serial.println("--------------------------");
}

//==============================================================================
// 4. LTE MQTT TRANSMISSION LOGIC
//==============================================================================

void sendDataOverLTE() {
  if (dataStoreCount == 0) {
    Serial.println("\nNo data in store, skipping LTE transmission.");
    return;
  }
  
  Serial.println("\n--- Preparing to send data to ThingsBoard ---");

  String telemetryKey = "sensor_data";
  String jsonPayload = "{\"" + telemetryKey + "\":{";

  for (int i = 0; i < dataStoreCount; i++) {
      char id_hex_buffer[6];
      sprintf(id_hex_buffer, "%02X:%02X", dataStore[i].id[0], dataStore[i].id[1]);
      jsonPayload += "\"";
      jsonPayload += id_hex_buffer;
      jsonPayload += "\":";
      jsonPayload += String(dataStore[i].data);
      if (i < dataStoreCount - 1) {
          jsonPayload += ",";
      }
  }
  jsonPayload += "}}";

  Serial.print("Constructed ThingsBoard JSON: ");
  Serial.println(jsonPayload);

  String publishCmd = "AT+SQNSMQTTPUBLISH=0,\"v1/devices/me/telemetry\","
                      + String(jsonPayload.length()) + ",0,0\r";

  // --- AT Command Sequence ---
  Serial.println("Waking up LTE module...");
  SERIAL_LTE.write("at+cfun=1\r");
  delay(10000);
  readLTESerial();

  Serial.println("Connecting to MQTT broker...");
  SERIAL_LTE.write("AT+SQNSMQTTCONNECT=0,\"35.199.178.152\",1883,60\r");
  delay(5000);
  readLTESerial();

  Serial.println("Publishing data...");
  SERIAL_LTE.print(publishCmd);
  delay(1000); 
  readLTESerial();

  SERIAL_LTE.print(jsonPayload);
  delay(5000);
  readLTESerial();

  Serial.println("Disconnecting from MQTT broker...");
  SERIAL_LTE.write("AT+SQNSMQTTDISCONNECT=0\r");
  delay(2000);
  readLTESerial();

  Serial.println("Putting LTE module to sleep...");
  SERIAL_LTE.write("at+cfun=0\r");
  delay(2000);
  readLTESerial();

  Serial.println("--- LTE sequence complete. ---");
}

void readLTESerial() {
  delay(50);
  while (SERIAL_LTE.available()) {
    Serial.write(SERIAL_LTE.read());
  }
}


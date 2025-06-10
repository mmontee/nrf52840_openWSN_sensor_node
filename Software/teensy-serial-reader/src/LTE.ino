#include <Arduino.h> // Standard Arduino header (good practice, especially for PlatformIO)

// --- Configuration ---
// Define the Hardware Serial port connected to the Nektar module's level shifter
#define SERIAL_NEKTAR Serial1 // Teensy 4.0/4.1: Use Serial1 (pins 0/1), Serial2 (pins 7/8), etc.

// Define the baud rate for communication with the Nektar module
#define NEKTAR_BAUD 9600

// Define the baud rate for the USB Serial connection to the PC
#define PC_BAUD 115200 // Match your Serial Monitor setting

// Define the interval between sending command sequences (in milliseconds)
// Adjust this based on the total time your delays take + desired off time
// Current delays: 5000+30000+5000+5000+1000+5000+5000 = 56000 ms (56 seconds)
// Setting interval to 60000 gives ~4 seconds between end of one sequence and start of next
const unsigned long sendInterval = 000; // Send sequence approx every 60 seconds
// --- End Configuration ---

// --- Global Variables ---
// Variable to store the last time the command sequence was sent
unsigned long lastSendTime = 0;

// Counter for the variable data in the JSON payload
int count = 0;



/**
 * @brief Pauses execution for a specified duration while continuously checking
 * for incoming data on SERIAL_NEKTAR and forwarding it to the main Serial port.
 * @param durationMs The number of milliseconds to "delay".
 */
void delayAndReadSerial(unsigned long durationMs) {
  unsigned long start = millis(); // Record the start time

  // Loop until the desired duration has passed
  while (millis() - start < durationMs) {
    // Check if any data is available from the Nektar module (Serial1)
    if (SERIAL_NEKTAR.available() > 0) {
      // Read one byte/character
      char receivedChar = SERIAL_NEKTAR.read();
      // Print the received character immediately to the PC's Serial Monitor
      Serial.print(receivedChar);
    }
    // Small delay to prevent busy-waiting consuming 100% CPU unnecessarily
    // and allow other background tasks (like USB serial handling) to run.
    delay(1); // Yield CPU time
  }
}

/**
 * @brief Initial setup function, runs once on boot/reset.
 */
void setup() {
  // Initialize serial communication with the PC (Serial Monitor via USB)
  Serial.begin(PC_BAUD);

  // Wait up to 4 seconds for the Serial Monitor to open (optional)
  unsigned long startWait = millis();
  while (!Serial && (millis() - startWait < 4000)) {
    ; // Wait for Serial port connection
  }

  Serial.println("---------------------------------------------");
  Serial.println("Teensy <-> Nektar AT Command Sender w/ Active Delay");
  Serial.println("---------------------------------------------");
  Serial.print("Initializing Nektar Serial Port (");
  Serial.print(NEKTAR_BAUD);
  Serial.println(" baud)...");

  // Initialize the hardware serial port connected to the Nektar module
  SERIAL_NEKTAR.begin(NEKTAR_BAUD); // Default 8N1 (8 data bits, No parity, 1 stop bit)

  // Use the custom delay function to allow ports to initialize fully
  Serial.println("Allowing ports to initialize...");
  delayAndReadSerial(500);

  Serial.println("Setup complete. Starting periodic AT command sends.");
  Serial.print("Will attempt command sequence approx every ");
  Serial.print(sendInterval / 1000.0); // Print interval in seconds
  Serial.println(" seconds.");
  Serial.println("Listening for responses from Nektar (even during delays)...");
  Serial.println("---------------------------------------------");

  lastSendTime = millis() - sendInterval;

  Serial.println("Setup finished. Entering loop...");
  Serial.println("---------------------------------------------");
  //handshake
  digitalWrite(14,HIGH);
  delay(1000);
  digitalWrite(14,LOW);
  delay(1000);
  digitalWrite(14,HIGH);
  delay(1000);
  digitalWrite(14,LOW);
  delay(1000);
}

/**
 * @brief Main program loop, runs repeatedly after setup().
 */
void loop() {
  // Get the current time AT THE START of each loop iteration
  unsigned long currentTime = millis();

  // --- Check if it's time to send the AT command sequence again ---
  // Uses unsigned long arithmetic, which handles millis() rollover correctly
  if (currentTime - lastSendTime >= sendInterval) {
    // Add a newline for better readability in the Serial Monitor
    Serial.println();
    // Print a timestamp marker
    Serial.print("[");
    Serial.print(currentTime / 1000.0); // Show time in seconds
    Serial.println("s] Sending AT command sequence to Nektar...");


    // --- AT Command Sequence ---
    Serial.println("  Sending: at\\r");
    SERIAL_NEKTAR.write("at\r"); // Basic check if module responds
    delayAndReadSerial(2000);

    Serial.println("  Sending: at+cfun=1\\r"); // Activate full functionality
    SERIAL_NEKTAR.write("at+cfun=1\r");
    delayAndReadSerial(10000); // This command can take a while

    Serial.println("  Sending: AT+SQNSMQTTCFG=..."); // Configure MQTT client ID and secret
    SERIAL_NEKTAR.write("AT+SQNSMQTTCFG=0,\"F0f66bb0-1a81-11f0-8ae5-17fa5f753f9b\",\"dxfMug4KeZvKgIldXGWj\"\r");
    delayAndReadSerial(2000);

    Serial.println("  Sending: AT+SQNSMQTTCONNECT=..."); // Connect to MQTT broker
    SERIAL_NEKTAR.write("AT+SQNSMQTTCONNECT=0,\"35.199.178.152\",1883,60\r");
    delayAndReadSerial(2000);

    Serial.println("  Sending: AT+SQNSMQTTPUBLISH=..."); // Prepare to publish
    SERIAL_NEKTAR.write("AT+SQNSMQTTPUBLISH=0,\"v1/devices/me/telemetry\",,12\r"); // Topic, empty message (data follows), QoS=2? Check docs. Wait 20s for prompt?
    delayAndReadSerial(1000); // Short delay before sending data

    // Prepare JSON payload
    char payloadBuffer[64]; // Ensure buffer is large enough for JSON + variable value
    // Format the JSON string with the current 'count' value
    snprintf(payloadBuffer, sizeof(payloadBuffer), "{\"rms\": %d}", count);

    Serial.print("  Sending Payload: "); // Log the payload being sent
    Serial.println(payloadBuffer);
    SERIAL_NEKTAR.write(payloadBuffer); // Write the JSON data
    SERIAL_NEKTAR.write("\r\n"); // Send CRLF - Check Nektar docs if this is needed after PUBLISH data or if the module sends a prompt first. Some modules expect Ctrl+Z (ASCII 26) instead.
    delayAndReadSerial(5000); // Wait after sending payload

    Serial.println("  Sending: AT+SQNSMQTTDISCONNECT=..."); // Disconnect from MQTT broker
    SERIAL_NEKTAR.write("AT+SQNSMQTTDISCONNECT=0\r");
    delayAndReadSerial(5000);

    Serial.println("  Sending: at+cfun=0\\r"); // Deactivate radio (power saving)
    SERIAL_NEKTAR.write("at+cfun=0\r");
    delayAndReadSerial(2000); // Allow time for command execution
    // No delay needed after the last command in the sequence before loop repeats check

    Serial.println("Command sequence complete.");

    // Update the timestamp of the last send using the time when this sequence STARTED
    lastSendTime = millis();
    count++; 
  } 
} // End of loop()

// teensy_serial_reader.ino
#include <Arduino.h>

#define SERIAL_MOTE Serial2 // Alias for Serial
#define SERIAL_LTE Serial1 // Alias for Serial 
const char* user_keyword = "uinject"; // Default user keyword
const char internal_keywords[] = "D"; // Internal keywords
const char end_character = '~'; // End character for internal messages
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 1000; // Send sequence approx every 60 seconds
unsigned int count = 0; // Counter for the variable data in the JSON payload
struct IDData {
    String id_hex;
    int data_value;
};

// Define a fixed-size array for storing IDData
const int MAX_IDS = 50; // Maximum number of IDs to store
IDData id_dict[MAX_IDS];
int id_count = 0; // Current number of stored IDs

void setup() {
    Serial.begin(115200);
    SERIAL_MOTE.begin(115200);
    while (!SERIAL_MOTE) {
        ; // Wait for serial port to connect. Needed for native USB
    }

    SERIAL_LTE.begin(115200);
    while (!SERIAL_LTE) {
        ; // Wait for serial port to connect. Needed for native USB
    }
}

void loop() {
    static String buffer = "";
    while (SERIAL_MOTE.available()) {
        char charRead = SERIAL_MOTE.read();
        buffer += charRead;

        // Check for internal keywords
        if (buffer.indexOf(internal_keywords) != -1) {
            int start_index = buffer.indexOf(internal_keywords);
            int end_index = buffer.indexOf(end_character, start_index);
            if (end_index != -1) {
                String message = buffer.substring(start_index, end_index + 1);

                // Check for user keyword
                if (buffer.indexOf(user_keyword) != -1) {
                    int keyword_index = buffer.indexOf(user_keyword);
                    String after_keyword = buffer.substring(keyword_index + strlen(user_keyword), keyword_index + strlen(user_keyword) + 9);
                    if (after_keyword.length() == 9) {
                        String data_bytes = after_keyword.substring(0, 2);
                        String id_bytes = after_keyword.substring(2, 4);

                        int data_value = data_bytes.toInt(); // Convert to integer
                        String id_hex = id_bytes; // Assuming id_bytes is already in hex format

                        // Check if the ID already exists
                        bool id_exists = false;
                        for (int i = 0; i < id_count; i++) {
                            if (id_dict[i].id_hex == id_hex) {
                                // Update the data for the existing ID
                                id_dict[i].data_value = data_value;
                                id_exists = true;
                                break;
                            }
                        }

                        if (!id_exists && id_count < MAX_IDS) {
                            // Add the new ID to the array
                            id_dict[id_count] = {id_hex, data_value};
                            id_count++;
                            Serial.print("New ID added: ");
                            Serial.println(id_hex);
                        }

                        // Print the current list of IDs and their values
                        Serial.println("\nCurrent ID List:");
                        for (int i = 0; i < id_count; i++) {
                            Serial.print("ID: ");
                            Serial.print(id_dict[i].id_hex);
                            Serial.print(" | Data: ");
                            Serial.println(id_dict[i].data_value);
                        }
                        Serial.println("--------------------");
                    }
                }

                // Remove the processed message from the buffer
                buffer = buffer.substring(end_index + 1);
            }
        }

                // Get the c)urrent time AT THE START of each loop iteration
        unsigned long currentTime = millis();

        // --- Check if it's time to send the AT command sequence again ---
        // Uses unsigned long arithmetic, which handles millis() rollover correctly
        if (currentTime - lastSendTime >= sendInterval) {
            if (id_count > 0){
            send_data(); // Call the function to send data
            // Update the timestamp of the last send using the time when this sequence STARTED
            lastSendTime = millis();
        }
    }
}




/**
 * @brief Main program loop, runs repeatedly after setup().
 */
void send_data(){

    // --- AT Command Sequence ---
    SERIAL_LTE.write("at\r"); // Basic check if module responds
    delay(2000);

    // Activate full functionality
    SERIAL_LTE.write("at+cfun=1\r");
    delay(10000); // This command can take a while

    // Configure MQTT client ID and secret
    SERIAL_LTE.write("AT+SQNSMQTTCFG=0,\"F0f66bb0-1a81-11f0-8ae5-17fa5f753f9b\",\"dxfMug4KeZvKgIldXGWj\"\r");
    delay(2000);

    // Connect to MQTT broker
    SERIAL_LTE.write("AT+SQNSMQTTCONNECT=0,\"35.199.178.152\",1883,60\r");
    delay(2000);

    SERIAL_NEKTAR.write("AT+SQNSMQTTPUBLISH=0,\"v1/devices/me/telemetry\",,12\r"); // Topic, empty message (data follows), QoS=2? Check docs. Wait 20s for prompt?
    delayAndReadSerial(1000); // Short delay before sending data

    // Prepare JSON payload from id_dict
    String jsonPayload = "wsn:{";
    if (id_count > 0) {
        for (int i = 0; i < id_count; i++) {
            jsonPayload += "\"" + id_dict[i].id_hex + "\":";
            jsonPayload += String(id_dict[i].data_value);
            if (i < id_count - 1) {
                jsonPayload += ",";
            }
        }
    }
    jsonPayload += "}";

    SERIAL_LTE.write(publishCmd);
    delay(1000); // Wait for modem prompt (e.g., '>') or time for it to be ready for payload


    SERIAL_LTE.write(jsonPayload.c_str()); // Write the JSON data
    SERIAL_LTE.write("\r\n"); // Send CRLF - Check Nektar docs if this is needed after PUBLISH data or if the module sends a prompt first. Some modules expect Ctrl+Z (ASCII 26) instead.
    delay(5000); // Wait after sending payload
    // Disconnect from MQTT broker
    SERIAL_LTE.write("AT+SQNSMQTTDISCONNECT=0\r");
    delay(5000);

    // Deactivate radio (power saving)
    SERIAL_LTE.write("at+cfun=0\r");
    delay(2000); // Allow time for command execution
    // No delay needed after the last command in the sequence before loop repeats check

    Serial.println("Command sequence complete.");
    // Removed count++ as 'count' variable is no longer used for this payload
}





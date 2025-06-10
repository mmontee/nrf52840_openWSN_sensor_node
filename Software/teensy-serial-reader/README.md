# Teensy Serial Reader

IMPORTANT NOTE: This program has not been successfully tested with the LTE-M modem. As it stand the program correctly parses the openWSN serial stream. The project team currently does not have a working the modem for further testing.

## Introduction

This project implements a serial data reader for the Teensy 4.0 microcontroller. It reads incoming serial data, checks for specific keywords, and processes the data similarly to a Python script designed for serial communication.

## Project Structure

```
teensy-serial-reader
├── src
│   └── teensy_serial_reader.ino
├── README.md
```

## Setup Instructions

1. **Hardware Requirements:**
   - Teensy 4.0 microcontroller
   - USB cable for connecting the Teensy to your computer
   - Optional: Additional sensors or devices that communicate over serial

2. **Software Requirements:**
   - Arduino IDE (with Teensyduino add-on installed)
   - Serial Monitor (for viewing output)

3. **Installation Steps:**
   - Connect your Teensy 4.0 to your computer using a USB cable.
   - Open the Arduino IDE and ensure that the Teensy 4.0 board is selected under `Tools > Board`.
   - Open the `src/teensy_serial_reader.ino` file in the Arduino IDE.
   - Upload the sketch to your Teensy 4.0 by clicking the upload button.

## Usage

1. After uploading the sketch, open the Serial Monitor in the Arduino IDE.
2. Set the baud rate to `115200` to match the Teensy serial communication settings.
3. The Teensy will start listening for incoming serial data. You can send data from another device or a computer.
4. The Teensy will process the incoming data, looking for specific keywords and displaying detected IDs and their associated values in the Serial Monitor.

## Functionality

- The Teensy reads incoming serial data byte by byte.
- It checks for specific internal keywords and a user-defined keyword to process the data.
- Detected IDs and their values are stored in a dictionary-like structure and displayed in the Serial Monitor.

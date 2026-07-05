# Alpha Node Firmware

## Description

The Alpha Node is built on an ESP32 and serves as one endpoint of the bidirectional LoRa communication system.

## Responsibilities

- Hosts the embedded Wi-Fi web interface
- Sends and receives LoRa messages
- Displays communication status on the OLED display
- Processes acknowledgment (ACK) packets
- Monitors RSSI and link quality
- Updates the web dashboard in real time

## Hardware

- ESP32
- SX1278 LoRa Module (433 MHz)
- OLED Display

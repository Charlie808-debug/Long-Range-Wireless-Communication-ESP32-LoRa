# Delta Node Firmware

## Description

The Delta Node operates as the second ESP32 communication endpoint.

## Responsibilities

- Receives LoRa packets from Alpha Node
- Sends acknowledgment packets
- Transmits user messages
- Displays communication statistics on the OLED
- Maintains reliable bidirectional communication

## Hardware

- ESP32
- SX1278 LoRa Module (433 MHz)
- OLED Display

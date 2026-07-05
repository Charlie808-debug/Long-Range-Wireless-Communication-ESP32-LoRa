# Firmware

This folder contains the firmware developed for the ESP32-based bidirectional LoRa communication system.

## Directory Structure

```text
firmware/
├── Alpha_Node/
└── Delta_Node/
```

Each node is programmed independently and communicates over the SX1278 LoRa module.

Both firmware implementations support:

- Bidirectional messaging
- ACK-based communication
- RSSI monitoring
- OLED status display
- Embedded web interface

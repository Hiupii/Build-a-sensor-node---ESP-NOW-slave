
# ESP-NOW Slave Sensor Node

A wireless sensor node implementation using ESP-NOW protocol for building distributed sensor networks.

## Overview

This project implements an ESP-NOW slave node that can communicate wirelessly with a master device, enabling low-power, lightweight mesh networking for IoT sensor applications.

## Features

- ESP-NOW wireless communication protocol
- Sensor data transmission
- Low-power operation
- Simple master-slave architecture

## Project Structure

```
├── src/
│   └── main.cpp           # Main application code
├── include/
│   └── types.h            # Data type definitions
└── README.md              # This file
```

## Getting Started

1. Clone the repository
2. Configure your ESP32 board settings
3. Build and flash the firmware to your ESP microcontroller
4. Use the CLI interface to configure sensor nodes

## Usage

The slave node listens for commands from the master device and processes sensor data accordingly. Configure the master MAC address in your code for pairing.

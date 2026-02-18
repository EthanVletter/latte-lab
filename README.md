# Latte-Lab

A miniature conveyor belt system demonstrating the future of shopfloor automation. This project showcases a full-scale IIoT (Industrial Internet of Things) pipeline: from ESP32-driven physical motor control to real-time data visualization within SAP via MQTT and OPC-UA

## Visuals

- [Insert Image of Wiring Setup]
- [Insert Image of Dashboard]
- [Wiring Diagrams](./images/wiring.png)

## Components Used

- ESP32 Development Board
- L298N/TB6612FNG Motor Driver
- DC Gear Motor
- Infrared/Motion Sensors
- Industrial Push Buttons

## Installation Manual

- [Full Installation Manual](./INSTALLATION.md)

## Wiring & Grounding Tips

Correct grounding is critical to prevent ESP32 "brownouts" or sensor interference.

> **Crucial Note:** Ensure the ESP32 Ground and the Motor Power Supply Ground are connected (Common Ground).

### Wire Color Standards used in Latte Lab:

| Type           | Colors (In order of priority) |
| :------------- | :---------------------------- |
| **Ground**     | Black, Brown, Purple          |
| **VCC (3.3V)** | Red, Orange, White            |
| **Functions**  | Yellow, Blue, Green           |

_Note: Button wiring is pre-configured where Red is Power and Black is Ground._

## Resource Links

- [Insert Resource Link]
- [Insert Resource Link]
- [Insert Resource Link]
- [Insert Resource Link]

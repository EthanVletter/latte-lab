# Latte-Lab

A miniature conveyor belt system demonstrating the future of shopfloor automation. This project showcases a full-scale IIoT (Industrial Internet of Things) pipeline: from ESP32-driven physical motor control to real-time data visualization within SAP via MQTT and OPC-UA

## Visuals

- [Insert Image of Wiring Setup]
- [Insert Image of Dashboard]
- [Wiring Diagrams](./images/wiring.png)

## Components Used

| Component                   | Description                                   | Purchase Link                                                   |
| :-------------------------- | :-------------------------------------------- | :-------------------------------------------------------------- |
| **ESP32 DevKit V1**         | Main microcontroller with WiFi/Bluetooth.     | [View on Amazon](https://www.amazon.com/s?k=esp32+devkit+v1)    |
| **L298N Motor Driver**      | Dual H-Bridge driver for DC motor control.    | [View on Amazon](https://www.amazon.com/s?k=L298N+motor+driver) |
| **DC Gear Motor**           | 6V-12V Motor for conveyor belt drive.         | [Insert link]()                                                 |
| **PIR Motion Sensors**      | Infrared sensors for motion/object detection. | [Insert link]()                                                 |
| **Industrial Push Buttons** | Manual override and start/stop controls.      | [Insert link]()                                                 |

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

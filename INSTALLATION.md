# Technical Installation Guide

## 1. Software & Drivers

To get the **Latte Lab** ecosystem running, ensure the following software is installed on your local machine and/or the server.

### Software Download Table

| Software          | Version                   | Purpose                               | Download Link                                                                                      |
| :---------------- | :------------------------ | :------------------------------------ | :------------------------------------------------------------------------------------------------- |
| **Arduino IDE**   | 2.x+                      | Firmware development & uploading.     | [Download](https://www.arduino.cc/en/software)                                                     |
| **CP210x Driver** | Universal                 | ESP32 USB-to-Serial communication.    | [Download](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads) |
| **Mosquitto**     | 2.1.2-install-windows.exe | MQTT Broker to handle messaging.      | [Download](https://mosquitto.org/download/)                                                        |
| **MQTT Explorer** | Latest                    | Visualizing MQTT traffic & debugging. | [Download](http://mqtt-explorer.com/)                                                              |

<!-- | **OpenVPN**       | Latest    | Secure remote connection to the server. | [Download](https://openvpn.net/community-downloads/)                                               | -->

## Driver Installation

If your computer doesn't detect the ESP32 via USB:

1. Download the **CP210x_Universal_Windows_Driver**.
2. Extract the folder, right-click the `.inf` file, and select **Install**.
3. Restart the Arduino IDE.

---

## 2. Arduino IDE Setup

### Required Libraries

Install these via the Library Manager (**Ctrl + Shift + I**):

| Library Name     | Author / Source    | Category | Purpose                                   |
| :--------------- | :----------------- | :------- | :---------------------------------------- |
| **ArduinoJson**  | Benoit Blanchon    | External | Formatting and parsing JSON data for SAP. |
| **PubSubClient** | Nick O'Leary       | External | MQTT protocol communication.              |
| **WiFi**         | Espressif Systems  | Built-in | Connecting to the local WiFi network.     |
| **WebServer**    | Espressif Systems  | Built-in | Hosting the local HTML Dashboard.         |
| **ESPmDNS**      | Espressif Systems  | Built-in | Hostname discovery (`latte-lab.local`).   |
| **Time**         | Standard C Library | Built-in | System timestamps & runtime tracking.     |

### Configuration Changes (Credentials)

To connect a new device to your network, update the `latte-lab/config.h` file.

> **Note:** If `config.h` does not exist, create it in the same folder as your `.ino` file and use the following template:

```cpp
// config.h - PRIVATE CREDENTIALS
#ifndef CONFIG_H
#define CONFIG_H

const char* ssid = "Wifi@123";               // Your WiFi Name
const char* password = "password@123";       // Your WiFi Password
// const char* mqtt_server = "192.168.56.1";
const char* mqtt_server = "my-pc.local";     // Your PC Name or IP Address

#endif
```

## 3. Server & Network Configuration

### Firewall Setup (Port 1883)

To allow the ESP32 to communicate with Mosquitto, you must open the port:

1. Open **Windows Firewall with Advanced Security**.
2. Navigate to **Inbound Rules** -> **New Rule**.
3. Select **Port** -> **TCP**.
4. Set the specific local port to **1883**.
5. Select **Allow the connection** and name it: `"MQTT Broker (Mosquitto)"`.

### Remote Access

- **VPN:** Connect via **OpenVPN** to gain access to the internal factory network.
- **RDP:** Use **Remote Desktop Connection** to access the server where Kepware and Matrikon are hosted.

---

## 4. Kepware & SAP Integration

### Kepware Configuration

- **Add IP:** In the Kepware MQTT Client Driver, ensure the **URL** matches your Local device IP (e.g., `10.8.0.4`).
- **Verification:** Use **Matrikon OPC Explorer** to verify that the tags are updating in real-time.

### Restarting Kepware (2-Hour Trial Fix)

The Kepware trial service automatically stops every 2 hours. Use **Method A** first; if the Admin Tool is unresponsive, use **Method B**.

#### **Method A: Via System Tray (Standard)**

1. Locate the **Kepware Admin Tool** icon in the Windows System Tray (bottom right).
2. **Right-click** -> Select **Stop Runtime Service** (Select **Yes** when prompted about disconnecting clients).
3. **Right-click** again -> Select **Start Runtime Service**.

#### **Method B: Via Windows Services (Fail-safe)**

1. Press **`Win + R`**, type `services.msc`, and hit Enter.
2. Scroll down to find **KEPServerEX 6.xx Runtime**.
3. **Right-click** the service and select **Restart**.

#### **After Restarting:**

In **MatrikonOPC Explorer**, navigate to `File -> Open -> latte-lab.XML`.

- _Note: Select **No** when asked to save changes to the session._

## 5. SAP Integration (PCo)

- **PCo Console:** Open the **SAP Plant Connectivity (PCo) Management Console** and ensure the `Latte_Lab` Agent is **Running** (indicated by a Green status).
- **SAP Transaction Code:** Use T-Code **`/n coopc1`** in the SAP GUI to monitor the OPC Items and verify live shopfloor data updates.

---

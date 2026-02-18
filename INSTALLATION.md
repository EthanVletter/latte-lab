# Technical Installation Guide

## 1. Software & Drivers

### Arduino IDE

- **Download:** [Arduino IDE Official Link](https://www.arduino.cc/en/software)
- **ESP32 Driver (COM Port Fix):** If your computer doesn't see the ESP32 when plugged in via USB:
  1. Download the **CP210x_Universal_Windows_Driver** driver from [Download and Install VCP Drivers](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads)
  2. Extract and right-click-install the `.inf` installer.
  3. Restart Arduino IDE.

### MQTT & Troubleshooting

- **Mosquitto Broker:** [Download Link](https://mosquitto.org/download/) - Required to handle messaging.
  1. Download the **mosquitto-2.1.2-install-windows-x64.exe**
- **MQTT Explorer:** [Download Link](http://mqtt-explorer.com/) - Use this to visualize the data coming from the ESP32.

---

## 2. Arduino IDE Setup

### Required Libraries

Install these via the Library Manager (**Ctrl + Shift + I**):

| Library Name | Author / Source    | Category | Purpose                                                      |
| :----------- | ------------------ | -------- | ------------------------------------------------------------ |
| ArduinoJson  | Benoit Blanchon    | External | Formatting and parsing JSON data for SAP & Web API.          |
| PubSubClient | Nick O'Leary       | External | MQTT protocol for communication with the Mosquitto Broker.   |
| WiFi         | Espressif Systems  | Built-in | Establishing connection to the local WiFi network.           |
| WebServer    | Espressif Systems  | Built-in | Hosting the local HTML Dashboard on the ESP32.               |
| ESPmDNS      | Espressif Systems  | Built-in | "Hostname discovery (e.g. accessing via saphexmotor.local)." |
| Time         | Standard C Library | Built-in | Managing system timestamps and motor runtime tracking.       |

### Configuration Changes for New Devices

To move the code to a new device/network, update these lines in `latte-lab/config.h`:

- `ssid`: Change to the new WiFi Name.
- `password`: Change to the WiFi Password.
- `mqtt_server`: Change to the IP Address of the computer running Mosquitto.

  > **Example ssid:** Wifi@123

  > **Example password:** password@123

  > **Example mqtt_server:** my-pc.local or 192.168.56.1

  ```bash
  // config.h - PRIVATE CREDENTIALS
    #ifndef CONFIG_H
    #define CONFIG_H

    const char* ssid = "Wifi@123";
    const char* password = "password@123";
    // const char* mqtt_server = "192.168.56.1";
    const char* mqtt_server = "my-pc.local";

    #endif
  ```

---

## 3. Server & Network Configuration

### Mosquitto Port Unblocking (Port 1883)

1. Open **Windows Firewall with Advanced Security**.
2. Go to **Inbound Rules** -> **New Rule**.
3. Select **Port** -> **TCP** -> Specific local ports: **1883**.
4. **Allow the connection** and name it "MQTT Broker (Mosquitto)".

### Remote Connection

- **VPN:** Connect via OpenVPN (Example: `10.8.x.x`).
- **Remote Desktop (RDP):** Use the server IP provided to access the Matrikon OPC/Kepware environment.

---

## 4. Kepware & SAP Integration

### Kepware Configuration

1. **Add IP:** In the MQTT Client Driver, ensure the "URL" matches the Mosquitto Broker IP.

   > **Example Host** 10.8.0.4

   > **Example Port** 1883

2. **Browsing Tags:** Use **Matrikon OPC Explorer** to verify that tags are flowing. If you see the tags in Matrikon, they are ready for PCo.

### Restarting Kepware (2-Hour Limit Fix)

Kepware trial versions stop the runtime service every 2 hours.

- **How to restart:**
  1. Look for the **Kepware Admin Tool** in the Windows System Tray (bottom right).
  2. Right-click and select **Stop Runtime Service**.
  - This operation will cause x active clients to be disconnected. Are you sure you want to continue? - `Yes`
  3. In MatrikonOPC Explorer click on `File->Open->latte-lab.XML`
  - Save Changes to session 'latte-lab.XML' - `No`

### SAP Integration (PCo)

1. **PCo Management Console:** Ensure the `Latte_Lab` "Agent" is Running and the status is "Green".
2. **SAP T-Codes:**

- Use `/n coopc1` to view the OPC Items that PCO is subscribed to view the live updates from the system.

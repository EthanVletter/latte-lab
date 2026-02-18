# Technical Installation Guide

## 1. Software & Drivers

### Arduino IDE

- **Download:** [Arduino IDE Official Link](https://www.arduino.cc/en/software)
- **ESP32 Driver (COM Port Fix):** If your computer doesn't see the ESP32 when plugged in via USB:
  1. Download the **CP210x** or **CH340** driver (refer to the `.zip` in this repo).
  2. Extract and run the `.exe` installer.
  3. Restart Arduino IDE.

### MQTT & Troubleshooting

- **Mosquitto Broker:** Required to handle messaging.
- **MQTT Explorer:** [Download Link](http://mqtt-explorer.com/) - Use this to visualize the data coming from the ESP32.

---

## 2. Arduino IDE Setup

### Required Libraries

Install these via the Library Manager (**Ctrl + Shift + I**):

1. **PubSubClient** (by Nick O'Leary)
2. **ArduinoJson** (by Benoit Blanchon)
3. **WiFi** (Built-in)
4. **WebServer** (Built-in)

### Configuration Changes for New Devices

To move the code to a new device/network, update these lines in `LatteLab.ino`:

- `ssid`: Change to the new WiFi Name.
- `password`: Change to the WiFi Password.
- `mqtt_server`: Change to the IP Address of the computer running Mosquitto.

---

## 3. Server & Network Configuration

### Mosquitto Port Unblocking (Port 1883)

1. Open **Windows Firewall with Advanced Security**.
2. Go to **Inbound Rules** -> **New Rule**.
3. Select **Port** -> **TCP** -> Specific local ports: **1883**.
4. **Allow the connection** and name it "Mosquitto".

### Remote Connection

- **VPN:** Connect via OpenVPN (Example: `10.8.x.x`).
- **Remote Desktop (RDP):** Use the server IP provided in the Latte Lab manual to access the Kepware environment.

---

## 4. Kepware & SAP Integration

### Kepware Configuration

1. **Add IP:** In the MQTT Client Driver, ensure the "URL" matches the Mosquitto Broker IP.
2. **Browsing Tags:** Use **Matrikon OPC Explorer** to verify that tags are flowing. If you see the tags in Matrikon, they are ready for PCo.

### Restarting Kepware (2-Hour Limit Fix)

Kepware trial versions stop the runtime service every 2 hours.

- **How to restart:**
  1. Look for the **Kepware Admin Tool** in the Windows System Tray (bottom right).
  2. Right-click and select **Stop Runtime Service**.
  3. Right-click again and select **Start Runtime Service**.

### SAP Integration (PCo)

1. **PCo Management Console:** Ensure the "Agent" is set to "Inbound" and the status is "Green".
2. **SAP T-Codes:** \* Use `/n/OEE/DASHBOARD` or specific plant maintenance codes to view real-time equipment status.

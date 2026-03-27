# Technical Installation Guide

## 1. Software & Drivers

To get the **Latte Lab** ecosystem running, ensure the following software is installed on your local machine and/or the server.

### Software Download Table

| Software          | Version                   | Purpose                                 | Download Link                                                                                      |
| :---------------- | :------------------------ | :-------------------------------------- | :------------------------------------------------------------------------------------------------- |
| **Arduino IDE**   | 2.x+                      | Firmware development & uploading.       | [Download](https://www.arduino.cc/en/software)                                                     |
| **CP210x Driver** | Universal                 | ESP32 USB-to-Serial communication.      | [Download](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads) |
| **Mosquitto**     | 2.1.2-install-windows.exe | MQTT Broker to handle messaging.        | [Download](https://mosquitto.org/download/)                                                        |
| **MQTT Explorer** | Latest                    | Visualizing MQTT traffic & debugging.   | [Download](http://mqtt-explorer.com/)                                                              |
| **Bonjour**       | Print Services for Win    | Enables mDNS (latte-lab.local).         | [Download](https://support.apple.com/kb/DL999)                                                     |
| **OpenVPN**       | Latest                    | Secure remote connection to the server. | [Download](https://openvpn.net/community-downloads/)                                               |

## Driver & Bonjour Installation

If your computer doesn't detect the ESP32 via USB:

1. Download the **CP210x_Universal_Windows_Driver**.
2. Extract the folder, right-click the `.inf` file, and select **Install**.
3. Restart the Arduino IDE.

To ensure your Windows PC can resolve local network hostnames (like `latte-lab.local`):

1. Download **Bonjour Print Services for Windows** from the Apple Support link above.
2. Run the installer and follow the standard on-screen prompts.
3. Restart your computer to ensure the mDNS responder service is actively running.

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

1. To connect a new device to your network, update the `latte-lab/config.h` file using **Arduino IDE**.

> **Note:** If `config.h` does not exist, create it in the same folder as your `.ino` file and use the following template:

```cpp
// config.h - PRIVATE CREDENTIALS
#ifndef CONFIG_H
#define CONFIG_H

const char* ssid = "Wifi@123";               // Your WiFi Name
const char* password = "password@123";       // Your WiFi Password
// const char* mqtt_server = "192.168.56.1";
const char* mqtt_server_1 = "my-pc.local";     // Your PC Name or IP Address
const char* mqtt_server_2 = "my-pc2.local";     // Your PC Name or IP Address

#endif
```

2. Verify/Compile: Before uploading, ensure there are no syntax errors.

- Shortcut: Press `Ctrl + R` (or click the Checkmark icon).
- Watch the output console at the bottom; it should say "Done compiling" in white text.

3. Upload to ESP32:

- Connect the ESP32 to your computer using a Micro-USB cable.
- Ensure the correct Port is selected under Tools > Port.
- Shortcut: Press `Ctrl + U` (or click the Right-Arrow icon).
- Once the console says "Done uploading," the ESP32 will reboot and attempt to connect to your network.

4. Monitor Live Output:

- While the ESP32 is connected via Micro-USB, you can view real-time logs (IP address, MQTT status, sensor triggers) directly from the device.
- Shortcut: Press `Ctrl + Shift + M` to open the Serial Monitor.
- Baud Rate: Ensure the dropdown in the bottom-right of the Serial Monitor is set to 115200 baud.

> If you see garbled text, double-check that the baud rate matches 115200.

## 3. Server & Network Configuration

### Mosquitto Broker Configuration (mosquitto.conf)

By default, newer versions of Mosquitto block external connections and require authentication. You must edit the configuration file to allow the ESP32 to connect.

1. Click the Windows Start Menu, type **Notepad**, right-click it, and select **Run as administrator**.
2. Go to **File > Open** and navigate to your Mosquitto installation directory (usually `C:\Program Files\mosquitto`).
3. Change the file type dropdown in the bottom right from "Text Documents" to "**All Files**".
4. Open the `mosquitto.conf` file.
5. Scroll to the bottom of the document and add the following two lines to enable the listener and allow external devices to connect without a password:

```bash
listener 1883
allow_anonymous true
```

6. Save the file (**Ctrl + S**) and close Notepad.
7. Open Windows Services (`services.msc`), find **Mosquitto Broker**, right-click it, and select **Restart** to apply the changes.

### Firewall Setup (Port 1883)

To allow the ESP32 to communicate with Mosquitto, you must open the port:

1. Open **Windows Firewall with Advanced Security**.
2. Navigate to **Inbound Rules** -> **New Rule**.
3. Select **Port** -> **TCP**.
4. Set the specific local port to **1883**.
5. Select **Allow the connection** and name it: `"MQTT Broker (Mosquitto)"`.

### Fallback: Restarting Port 1883 via CMD

If Mosquitto fails to connect, or if the port gets stuck in a "**listening**" state from a crashed session, you can forcefully restart the service using the Command Prompt.

1. Click the Windows Start Menu, type **cmd**, right-click **Command Prompt**, and select **Run as administrator**.
2. Type the following command to find the service:
    ```bash
    netstat -ano | findstr :1883
    ```
3. Type the following command to kill the service:
    ```bash
    taskkill /PID <YourPIDNumber>
    ```

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
4. Update the runtime with the loaded project following connect? (**Yes**)
5. Save changes? (**No**)

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

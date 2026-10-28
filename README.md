# GravyMaker CrowPanel LCD UI

## 1. Project overview

This repository contains the **CrowPanel DIS08070H ESP32-S3 touchscreen interface** for the GravyMaker project. The display is an 800 × 480 touch LCD running Arduino-based firmware with **PlatformIO, LovyanGFX, and LVGL**.

The LCD is the user-interface side of the system. The separate **main GravyMaker ESP32** handles machine-side operations such as dispensing and spice-level checks. The two ESP32 boards communicate over a local Wi-Fi network using TCP.

### UI features

- Main menu with **DISPENSE** and **CHECK SPICE**.
- Dispense screen for selecting **ONION**, **TOMATO**, or both.
- Serving selection for **1** or **2** servings.
- Confirmation and progress/status screens.
- Spice-check screen showing the status of 12 compartments: `INSUFFICIENT`, `ENOUGH FOR 2 SERVINGS`, or `FULL`.
- Status updates received from the main ESP32.

> The LCD firmware does not itself perform the machine's motor or sensor operations. Those functions are implemented on the main GravyMaker ESP32.

## 2. Hardware and software

### Hardware

- CrowPanel DIS08070H ESP32-S3 touch display, 800 × 480.
- Separate main GravyMaker ESP32.
- Both boards must connect to the **same Wi-Fi network/hotspot**.
- USB data cables suitable for programming each board.

### Development software

- Visual Studio Code.
- PlatformIO IDE extension.
- PlatformIO Espressif 32 platform / Arduino framework.
- Libraries used by the display project include LovyanGFX and LVGL. Keep the versions and configuration specified by the project unless deliberately updating and testing them.

## 3. Repository structure

The important files in the LCD project are:

```text
CrowPanel LCD project/
├── platformio.ini          # PlatformIO board, framework, libraries and build settings
├── src/
│   ├── main.cpp            # Firmware entry point; display/touch setup and UI loop
│   ├── gravy_ui.h          # LVGL screens, widgets, selections and UI status handling
│   └── gravy_wifi.h        # Wi-Fi credentials, main ESP32 IP, TCP client and messages
├── touch.h                 # CrowPanel touch-controller configuration/helpers (if present here)
└── README.md               # This documentation
```

The exact repository may contain additional vendor files, examples, libraries, or configuration files. **Do not delete unfamiliar CrowPanel/vendor files simply because they are not listed above.** Display and touch initialisation can depend on the supplied board configuration.

### File responsibilities

| File | Responsibility | Edit when… |
|---|---|---|
| `platformio.ini` | Board target, Arduino framework, build flags and dependencies | Changing build environment or board configuration |
| `src/main.cpp` | Program entry point, LCD/touch initialisation and main loop | Changing startup or top-level UI/task integration |
| `src/gravy_ui.h` | LVGL screens, buttons, selection logic, spice display and status-to-UI mapping | Changing layout, labels, colours, navigation or UI behaviour |
| `src/gravy_wifi.h` | Wi-Fi settings, target IP/port, outgoing commands and incoming status parsing | Changing hotspot/network or communication protocol |
| `touch.h` (if present) | Touch-controller setup or helper definitions | Only when modifying touch behaviour or board configuration |

The main machine firmware is a **separate project/repository**. Its corresponding Wi-Fi header is commonly named `gravy_wifi.h`, and its main sketch may be an `.ino` file. Confirm the actual filenames in the main ESP32 repository before editing.

## 4. How the two boards communicate

The current design uses a **TCP client/server connection on port `8080`**:

1. The main GravyMaker ESP32 connects to the selected Wi-Fi network.
2. The main ESP32 starts a TCP server on port `8080` and prints its assigned local IP address to its Serial Monitor.
3. The CrowPanel connects to the same Wi-Fi network.
4. When the user requests dispensing or a spice check, the CrowPanel connects to the main ESP32's IP address on port `8080` and sends a text command.
5. The main ESP32 processes the request and sends status lines back over the TCP connection.
6. The CrowPanel parses those status lines and updates the UI.

Both boards should be on the same local network, and the network must allow devices to communicate with each other. Some guest Wi-Fi networks enable client/AP isolation and prevent this; use a hotspot or network where device-to-device communication is permitted.

### Commands sent by the CrowPanel

Dispense command format:

```text
DISPENSE|ONION=<0-or-1>|TOMATO=<0-or-1>|SERVINGS=<1-or-2>
```

Example:

```text
DISPENSE|ONION=1|TOMATO=0|SERVINGS=2
```

This requests two servings of onion only. `1` means selected and `0` means not selected.

Spice-check command:

```text
SPICECHECK
```

### Status lines handled by the UI

The main ESP32 may send status lines such as:

```text
ONION_START
ONION_DONE
TOMATO_START
TOMATO_DONE
DISPENSE_DONE
SPICE_LEVEL|<compartment-number>|<level>
SPICECHECK_DONE
ERROR|<error-code>
```

For spice levels, the compartment number is **1–12**. The current UI mapping is:

| Level value | Display meaning |
|---:|---|
| `-1` | `INSUFFICIENT` |
| `0` | `ENOUGH FOR 2 SERVINGS` |
| `1` | `FULL` |

Keep command and status strings consistent between both projects. If one side changes a message format, update and test the other side too.

## 5. Moving to a different hotspot or Wi-Fi network

When transferring the LCD project to another team or changing networks, update the settings in **both the CrowPanel firmware and the main ESP32 firmware**. Changing only one side is not enough.

### Step 1 — Choose the network

Choose a hotspot/network that both ESP32 boards can join. Record the network name (SSID) and password. Avoid committing real Wi-Fi credentials to a public repository.

### Step 2 — Update the main GravyMaker ESP32 firmware

In the **main ESP32 project's** `gravy_wifi.h`, locate the Wi-Fi settings near the top:

```cpp
static const char* GM_WIFI_SSID = "YOUR_WIFI_NAME";
static const char* GM_WIFI_PASS = "YOUR_WIFI_PASSWORD";
```

Replace the placeholders with the new network's SSID and password.

Keep the server port consistent with the CrowPanel:

```cpp
static constexpr uint16_t GM_WIFI_PORT = 8080;
```

Do not change the port on only one board.

### Step 3 — Upload and obtain the main ESP32's IP address

1. Connect the main ESP32 to the computer.
2. Open the **main ESP32 firmware project** in PlatformIO or the IDE used by that project.
3. Build and upload the firmware.
4. Open its Serial Monitor at the baud rate configured by that project (commonly `115200`; check the code).
5. Reset the board and wait for the Wi-Fi connection message.
6. Find the line similar to:

   ```text
   GravyMaker IP: 192.168.1.123
   GravyMaker TCP server ready on port 8080
   ```

7. Write down the actual IP shown by the board. The example IP above is illustrative; **do not copy it unless your board prints that exact address**.

The assigned IP can change after switching hotspots, restarting the router, or reconnecting.

### Step 4 — Update the CrowPanel Wi-Fi settings

In the LCD repository, open `src/gravy_wifi.h`. Update the credentials near the top:

```cpp
static const char* CROW_WIFI_SSID = "YOUR_WIFI_NAME";
static const char* CROW_WIFI_PASS = "YOUR_WIFI_PASSWORD";
```

These values must match the network settings used by the main ESP32.

### Step 5 — Update the main ESP32 IP address in the CrowPanel code

In the same `src/gravy_wifi.h`, find:

```cpp
static IPAddress MAIN_ESP32_IP(192, 168, 1, 123);
static constexpr uint16_t GRAVY_PORT = 8080;
```

Replace the four IP numbers with the IP printed by the main ESP32 Serial Monitor. For example, if the board prints `192.168.43.75`, use:

```cpp
static IPAddress MAIN_ESP32_IP(192, 168, 43, 75);
```

Use the actual IP for the current network. Keep `GRAVY_PORT` at `8080` unless the server port is intentionally changed in **both** firmware projects.

### Step 6 — Build and upload the CrowPanel firmware

1. Open the LCD project folder in VS Code.
2. Wait for PlatformIO to finish loading the project.
3. Connect the CrowPanel by USB.
4. Select the correct upload port for that computer/board in PlatformIO. The COM port may differ on another computer.
5. Build the project.
6. Upload the firmware.
7. Open the CrowPanel Serial Monitor using the configured baud rate (commonly `115200`; verify in `setup()` or `platformio.ini`).
8. Reset the display and check the Serial Monitor for Wi-Fi and TCP connection messages.

### Step 7 — Verify communication

Test in this order:

1. **Wi-Fi:** both boards connect to the same SSID.
2. **IP:** the main ESP32 prints a valid local IP.
3. **Server:** the main ESP32 reports that the TCP server is ready on port `8080`.
4. **TCP:** the CrowPanel connects to the main ESP32 at the configured IP and port.
5. **Dispense UI:** select an ingredient and servings; verify that the main ESP32 receives the expected `DISPENSE|...` command and that status messages update the display.
6. **Spice check:** select CHECK SPICE; verify that the main ESP32 receives `SPICECHECK` and returns `SPICE_LEVEL` messages followed by `SPICECHECK_DONE`.

## 6. PlatformIO setup from a fresh clone

1. Install Visual Studio Code.
2. Install the **PlatformIO IDE** extension.
3. Clone/download this repository.
4. In VS Code, choose **File → Open Folder** and select the folder containing `platformio.ini` (not just the `src` folder).
5. Allow PlatformIO to install the platform, framework and dependencies specified by the project.
6. Connect the CrowPanel using a USB data cable.
7. Select the correct serial/upload port if PlatformIO does not select it automatically.
8. Use **Build** to compile and check for errors.
9. Use **Upload** to flash the board.
10. Open the PlatformIO Serial Monitor to check startup and network logs.

If dependencies fail to install, check internet access and the PlatformIO terminal output. Avoid randomly deleting project configuration or changing library versions before recording the original error.

## 7. Troubleshooting

### CrowPanel cannot join Wi-Fi

- Check `CROW_WIFI_SSID` and `CROW_WIFI_PASS` in `src/gravy_wifi.h`.
- Confirm the hotspot is active and its SSID/password are correct.
- Confirm the hotspot supports the Wi-Fi band/security mode supported by the ESP32-S3.
- Restart both boards after changing credentials.

### Main ESP32 connects, but the LCD says it cannot connect to GravyMaker

- Confirm both boards are on the **same network**.
- Read the main ESP32's current IP from its Serial Monitor.
- Update `MAIN_ESP32_IP` in `src/gravy_wifi.h`.
- Confirm the main server port and CrowPanel port are both `8080`.
- Check that the network does not block device-to-device traffic through client isolation.
- If the main ESP32 reconnects or the hotspot restarts, check whether its IP changed.

### Works on one hotspot but not another

- Some hotspots use different IP ranges or assign new addresses after reconnecting.
- Repeat Steps 2–5 above; do not assume an old IP remains valid.
- Ensure both boards connect to the same hotspot, not one to a hotspot and the other to a different Wi-Fi network.
- Confirm that the new network permits local TCP connections between clients.

### UI opens but statuses do not update

- Check the CrowPanel Serial Monitor for outgoing commands and received status lines.
- Check the main ESP32 Serial Monitor for incoming requests and `[WiFi status]` messages.
- Ensure message strings and formats match the protocol in Section 4.
- For spice-check, verify that the main ESP32 actually sends `SPICE_LEVEL|...` messages and finishes with `SPICECHECK_DONE`.

### Upload fails

- Check the selected COM port.
- Use a USB cable that supports data, not charging only.
- Close any other serial monitor/application using the port.
- Confirm the project is opened at the directory containing `platformio.ini`.

## 8. Handover checklist for the receiving team

- [ ] Clone the repository and open the folder containing `platformio.ini`.
- [ ] Confirm the CrowPanel model and display/touch configuration.
- [ ] Identify and obtain access to the separate main GravyMaker ESP32 firmware repository.
- [ ] Set the same Wi-Fi SSID/password in both firmware projects.
- [ ] Upload the main ESP32 firmware and record its current IP address.
- [ ] Set `MAIN_ESP32_IP` in `src/gravy_wifi.h`.
- [ ] Keep the TCP port consistent (`8080` by default).
- [ ] Build and upload the CrowPanel firmware.
- [ ] Test Wi-Fi, TCP connection, dispense status updates and spice-check updates.
- [ ] Remove any personal hotspot credentials from source before making the repository public.

## 9. Important maintenance notes

- **Network changes:** normally require edits to the SSID/password in both projects and the main ESP32 IP in the CrowPanel project.
- **Protocol changes:** update both the sender and receiver. Preserve exact separators and status names unless both sides are changed together.
- **UI changes:** primarily belong in `src/gravy_ui.h`.
- **Display/touch setup changes:** review `src/main.cpp`, `touch.h` (if present), and `platformio.ini` carefully.
- **Machine behaviour:** dispensing sequences, sensor acquisition, motor control and spice-level classification belong to the main ESP32 firmware, not the LCD UI repository.
- **Credentials:** use local/private configuration or placeholders for secrets. Do not publish hotspot passwords in a public Git repository.
- **IP assignment:** the current implementation uses a manually configured main ESP32 IP on the CrowPanel. It does not automatically discover the server, so update the IP when the network assigns a different one.

## 10. Quick reference

| Setting | Where to change it | Requirement |
|---|---|---|
| Wi-Fi SSID/password for main ESP32 | Main ESP32 `gravy_wifi.h` | Must match the selected network |
| Wi-Fi SSID/password for CrowPanel | LCD `src/gravy_wifi.h` | Must match the selected network |
| Main ESP32 IP | LCD `src/gravy_wifi.h` → `MAIN_ESP32_IP` | Must match the IP printed by the main ESP32 |
| TCP port | Both `gravy_wifi.h` files | Must match; default is `8080` |
| UI screens and labels | LCD `src/gravy_ui.h` | LCD-side UI only |
| Main ESP32 sensor/motor logic | Main ESP32 firmware | Separate firmware/project |
| Upload port | PlatformIO / local machine | May change between computers |

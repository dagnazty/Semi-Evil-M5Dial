<div align="center">

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg) ![GitHub issues](https://img.shields.io/github/issues/dagnazty/Semi-Evil-M5Dial) ![GitHub stars](https://img.shields.io/github/stars/dagnazty/Semi-Evil-M5Dial?style=social) ![GitHub forks](https://img.shields.io/github/forks/dagnazty/Semi-Evil-M5Dial?style=social)

![Project Banner](logo.png)

# Semi-Evil M5Dial

</div>

## Introduction 🎉

<div align="center">

Welcome to **Semi-Evil M5Dial**!

Unlock the full potential of your M5Dial device with our feature-rich firmware. Whether you're a developer, security enthusiast, or hobbyist, Semi-Evil-M5Dial empowers you with advanced tools such as WiFi captive portal management, SSID handling, Karma attacks, and BadUSB script execution. Dive in and transform your M5Dial into a versatile powerhouse!

</div>

## Table of Contents 📚

- [Introduction](#introduction)
- [Features](#features)
- [Setup & Installation 🚀🔧](#setup--installation-🚀🔧)
- [Usage 🛠️](#usage-🛠️)
- [Web-Based File Uploader](#web-based-file-uploader)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)
- [Contributing 🤝](#contributing-🤝)
- [License 📄](#license-📄)
- [Credits](#credits)
- [Support & Contact 📬](#support--contact-📬)

## Features

- 🔒 **WiFi Captive Portal:** Create a customizable captive portal for network interactions.
- 📡 **SSID Management:** Save, select, and manage multiple SSIDs with ease.
- 🕵️‍♂️ **Karma Attack Mode:** Perform Karma attacks by spoofing SSIDs and capturing probe requests.
- 💻 **BadUSB Integration:** Execute pre-defined USB HID scripts to automate keyboard inputs.
- ⚙️ **Settings Menu:** Adjust screen brightness, toggle modes, enable verbose debugging, and more.
- 🌐 **Web-Based File Uploader:** Easily upload and delete files on the device’s SPIFFS from a browser interface.

## Setup & Installation 🚀🔧

Follow these steps to get **Semi-Evil M5Dial** up and running on your M5Dial device using **PlatformIO**.

### Prerequisites

- **PlatformIO IDE:** It's recommended to use [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO extension](https://platformio.org/install/ide?install=vscode) installed.
- **USB Cable:** A reliable USB cable to connect your M5Dial device to your computer.
- **Git:** To clone the repository. Install [Git](https://git-scm.com/downloads) if you haven't already.

### Step 1: Clone the Repository

Open your terminal or command prompt and execute:

```bash
git clone https://github.com/dagnazty/Semi-Evil-M5Dial.git
```

### Step 2: Open the Project in PlatformIO

1. **Launch Visual Studio Code (VS Code):**
   - Ensure you have the [PlatformIO extension](https://platformio.org/install/ide?install=vscode) installed.

2. **Open the Project:**
   - Click on **File > Open Folder...** and select the cloned `Semi-Evil-M5Dial` directory.

3. **Wait for PlatformIO to Initialize:**
   - PlatformIO will automatically detect the project configuration and initialize the environment.

### Step 3: Install Dependencies

PlatformIO manages dependencies through the `platformio.ini` file. Ensure all required libraries are specified. The provided `platformio.ini` includes all necessary dependencies.

### Step 4: Prepare the Filesystem (SPIFFS)

The firmware utilizes the SPIFFS filesystem to store configurations, SSIDs, logs, and scripts.

1. **Locate the `data/` Folder:**
   - Ensure the `data/` folder is located at the **root of the project**, alongside the `src/` folder.

   ```
   Semi-Evil-M5Dial/
   ├── include/
   ├── lib/
   ├── src/
   │   └── main.cpp
   ├── data/
   │   ├── index.html
   │   ├── doge.html
   │   ├── SSID.json
   │   ├── log.txt
   │   ├── logo.bmp
   │   └── test.txt
   ├── platformio.ini
   └── README.md
   ```

2. **Add Required Files:**
   - **Web Files:**
     - `index.html`: The main page for the captive portal.
     - `doge.html`: An additional HTML page.
   - **Configuration Files:**
     - `SSID.json`: Stores the list of saved SSIDs.
     - `log.txt`: Stores logs collected via the captive portal.
   - **BadUSB Scripts:**
     - Place your `.txt` script files in the `data/` folder. These scripts define the USB HID actions.
   - **Images:**
     - `logo.bmp`: Displayed upon device startup.

3. **Upload SPIFFS to Device:**
   - In VS Code, open the **PlatformIO Terminal** (`View > Terminal`).
   - Execute the following command to upload the `data/` folder to the device's SPIFFS:

     ```bash
     pio run --target uploadfs
     ```

   - **Note:** Ensure your device is connected and recognized by your computer.

### Step 5: Upload Firmware to Device

After uploading the filesystem, proceed to flash the firmware.

1. **Ensure the Device is in Bootloader Mode:**
   - **Automatic Entry:** Most devices automatically enter bootloader mode when initiating a firmware upload.
   - **Manual Entry:**
     - Press and hold the **RESET** button.
     - While holding **RESET**, plug the device in.
     - Release the **RESET** button.

2. **Upload Firmware:**
   - In the **PlatformIO Toolbar** at the bottom, click the **Upload** button (right arrow icon) or press `Ctrl + Alt + U`.
   - PlatformIO will compile the project and flash the firmware to your device.
   - **Progress Indicators:**
     - Monitor the **Output** panel for upload progress and any potential errors.

3. **Verify Successful Upload:**
   - Upon completion, you should see a success message.
   - The device will automatically reboot if **Auto Reboot** is enabled in `platformio.ini`.

## Usage 🛠️

### Navigating the Menu

The M5Dial features a rotary encoder and a button (**BtnA**) for navigation:

- **Rotate Encoder:** Scroll through menu items.
- **Press BtnA:** Select the highlighted menu item.
- **Long Press BtnA (1 second):** Return to the main menu or perform specific actions depending on the context.

### Features Overview

Upon powering up, the device displays a logo (if available) and navigates to the main menu with the following options:

1. **Start Portal:**  
   Initiates a WiFi captive portal allowing users to connect and interact via the hosted web server.

2. **Saved SSID:**  
   View and select from a list of previously saved SSIDs to configure the device's AP.

3. **Start Karma:**  
   Engages Karma attack mode to spoof SSIDs and capture probe requests.

4. **BadUSB:**  
   Execute pre-defined USB HID scripts for automated keyboard inputs.

5. **About:**  
   Displays information about the firmware version and authors.

6. **Settings:**  
   Access and modify device settings such as screen brightness, toggle modes, and enable verbose debugging.

## Web-Based File Uploader

In addition to the captive portal, **Semi-Evil M5Dial** provides a **web-based uploader** for conveniently managing files on the device’s SPIFFS *without* needing to rebuild or re-upload via PlatformIO.

1. **Start the Portal** (from the main menu) and connect to the device’s AP (e.g. `192.168.4.1`).
2. **Navigate to `/upload`:**  
   For example, open `http://192.168.4.1/upload` in your browser.
3. **Use the “Device Control Panel”:**
   - **Upload Files**: Click **“Upload Files”** to add new scripts, images, or JSON files to SPIFFS.
   - **List & Delete**: See a list of uploaded files and delete them at the press of a button.

### Typical Workflow

- **Add** or **replace** your **BadUSB** scripts (`.txt` files), images (`.bmp`), or configuration (`.json`) files via `/upload`.
- After uploading, the device automatically updates the SPIFFS file list, so you can immediately use or select new resources in the menus.

**Note**: If your device is connected under a different IP or captive portal, adjust the URL accordingly (e.g., `http://<device-IP>/upload`).

## Configuration

### Settings Menu

Access the **Settings** menu from the main menu to perform various configurations:

1. **Power Off:**
   - Safely powers down the device.

2. **Screen Brightness:**
   - Adjust the display brightness using the encoder.

3. **Toggle BadUSB Mode:**
   - Switch between Normal (Debug) mode and BadUSB (HID) mode.

4. **Verbose Debug:**
   - Enable or disable verbose debugging for more detailed logs.

5. **Back:**
   - Return to the main menu.

### BadUSB Scripts

To utilize the BadUSB feature:

1. **Create Scripts:**
   - Write your BadUSB scripts in `.txt` files following the defined format:
     - `DELAY <milliseconds>`
     - `STRING <text>`
     - `ENTER`
     - `TAB`
     - `ESC`
     - `CTRL <key>`
     - `ALT <key>`

2. **Add Scripts to SPIFFS:**
   - Place your `.txt` script files in the `data/` folder before uploading SPIFFS.

3. **Execute Scripts:**
   - Navigate to **BadUSB** in the main menu.
   - Select the desired script to execute. In Debug mode, scripts can be read via Serial. In BadUSB mode, selected scripts execute upon device reset or power-on.

## Troubleshooting

- **SPIFFS Upload Issues:**
  - Ensure the `data/` folder is correctly structured at the project root.
  - Verify the **PlatformIO Core** is up-to-date and no duplicate installations exist.
  - Check file permissions to ensure PlatformIO can access the directory and its files.

- **Web Server Not Accessible:**
  - Confirm the device's AP is active and you're connected to the correct SSID.
  - Check firewall settings on your computer that might block access.

- **BadUSB Scripts Not Executing:**
  - Verify scripts are correctly formatted and placed in SPIFFS.
  - Ensure the device is in BadUSB mode.

- **Display Issues:**
  - Adjust screen brightness in the Settings menu.
  - Check for corrupted SPIFFS files.

- **Device Not Responding:**
  - Perform a reset by disconnecting and reconnecting the USB cable.
  - Re-upload the firmware if necessary.

- **PlatformIO Core Errors:**
  - Ensure only one PlatformIO Core installation exists.
  - Update PlatformIO Core to the latest version following the [PlatformIO Troubleshooting Guide](https://docs.platformio.org/en/latest/core/installation/troubleshooting.html).

## Contributing 🤝

We welcome contributions from the community! To ensure a smooth collaboration, please follow these guidelines:

1. **Fork the Repository:**  
   Click the **Fork** button at the top-right corner of this page.

2. **Clone Your Fork:**  
   ```bash
   git clone https://github.com/yourusername/Semi-Evil-M5Dial.git
   ```

3. **Create a Feature Branch:**  
   ```bash
   git checkout -b feature/your-feature-name
   ```

4. **Commit Your Changes:**  
   Ensure your code follows the existing style and includes appropriate documentation.

5. **Push to Your Fork:**  
   ```bash
   git push origin feature/your-feature-name
   ```

6. **Open a Pull Request:**  
   Navigate to the original repository and click **New Pull Request**.

## License 📄

This project is licensed under the [MIT License](LICENSE).  
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

## Credits

- **Developers:**
  - [7h30th3r0n3](https://github.com/7h30th3r0n3)
  - [dagnazty](https://github.com/dagnazty)

- **Libraries:**
  - [M5Dial](https://github.com/m5stack/M5Dial)
  - [ArduinoJson](https://arduinojson.org/)
  - [ESP32 Arduino](https://github.com/espressif/arduino-esp32)
  
- **Tools:**
  - [ESP32 Sketch Data Upload](https://github.com/esp8266/arduino-esp8266fs-plugin)

- **Original Project:**
  - [Evil-M5Core2](https://github.com/7h30th3r0n3/Evil-M5Core2) by [7h30th3r0n3](https://github.com/7h30th3r0n3)

## Support & Contact 📬

For any questions or support, please open an issue on the [GitHub repository](https://github.com/dagnazty/Semi-Evil-M5Dial/issues) or reach out to us directly:

- **GitHub:** [@dagnazty](https://github.com/dagnazty)
- **Twitter:** [@dagnazty](https://twitter.com/dagnazty)

Stay updated with the latest developments by starring the repository ⭐ and following our [project updates](https://github.com/dagnazty/Semi-Evil-M5Dial/releases).

---
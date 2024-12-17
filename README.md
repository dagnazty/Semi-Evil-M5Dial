<div align="center">

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg) ![GitHub issues](https://img.shields.io/github/issues/dagnazty/Semi-Evil-M5Dial) ![GitHub stars](https://img.shields.io/github/stars/dagnazty/Semi-Evil-M5Dial?style=social) ![GitHub forks](https://img.shields.io/github/forks/dagnazty/Semi-Evil-M5Dial?style=social)

![Project Banner](logo.png)

# Semi-Evil M5Dial

</div>

## Introduction

<div align="center">

Welcome to **Semi-Evil M5Dial**! 🎉

Unlock the full potential of your M5Dial device with our feature-rich firmware. Whether you're a developer, security enthusiast, or hobbyist, Semi-Evil-M5Dial empowers you with advanced tools such as WiFi captive portal management, SSID handling, Karma attacks, and BadUSB script execution. Dive in and transform your M5Dial into a versatile powerhouse!

</div>

## Table of Contents 📚

- [Introduction](#introduction)
- [Features](#features)
- [Getting Started](#getting-started-🚀)
- [Installation](#installation-🔧)
- [Usage](#usage-🛠️)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing-🤝)
- [License](#license-📄)
- [Credits](#credits)
- [Support & Contact](#support--contact-📬)

## Features

- 🔒 **WiFi Captive Portal:** Create a customizable captive portal for network interactions.
- 📡 **SSID Management:** Save, select, and manage multiple SSIDs with ease.
- 🕵️‍♂️ **Karma Attack Mode:** Perform Karma attacks by spoofing SSIDs and capturing probe requests.
- 💻 **BadUSB Integration:** Execute pre-defined USB HID scripts to automate keyboard inputs.
- ⚙️ **Settings Menu:** Adjust screen brightness, toggle modes, enable verbose debugging, and more.

## Getting Started 🚀

Follow these simple steps to get Semi-Evil M5Dial up and running on your M5Dial device:

1. **Clone the Repository:**  
   ```bash
   git clone https://github.com/dagnazty/Semi-Evil-M5Dial.git
   ```
2. **Install Dependencies:**  
   Open the project in Arduino IDE and install the required libraries via the Library Manager.
3. **Prepare SPIFFS Filesystem:**  
   Create the `data` folder and add necessary files as detailed in the [Installation](#installation-🔧) section.
4. **Upload Firmware:**  
   Connect your M5Dial device and upload the firmware using Arduino IDE.
5. **Configure and Enjoy:**  
   Navigate through the menu on your device to configure settings and explore features.

## Installation 🔧

### 1. Clone the Repository

Clone this repository to your local machine using Git:

```bash
git clone https://github.com/dagnazty/Semi-Evil-M5Dial.git
```

### 2. Install Dependencies

Open the `Semi-Evil-M5Dial` folder in the Arduino IDE. Navigate to **Sketch > Include Library > Manage Libraries** and install the following libraries:

- **M5Dial**
- **ArduinoJson**
- **USBHIDKeyboard**
- **Preferences**
- **SPIFFS**

### 3. Prepare SPIFFS Filesystem

The firmware utilizes the SPIFFS filesystem to store configurations, SSIDs, logs, and scripts. Follow these steps:

1. **Create SPIFFS Folder:**
   - In the root directory of the project, create or copy the folder named `data`.

2. **Add Required Files:**
   - **Web Files:**
     - `index.html`: The main page for the captive portal.
     - `doge.html`: An additional HTML page.
   - **Configuration Files:**
     - `SSID.json`: Stores the list of saved SSIDs.
     - `log.txt`: Stores logs collected via the captive portal.
   - **BadUSB Scripts:**
     - Place your `.txt` script files in the root of the `data` folder. These scripts define the USB HID actions.

3. **Sample File Structure:**

    ```
    Semi-Evil-M5Dial/
    ├── Semi-Evil-M5Dial-v1-1-0.ino
    └── data/
        ├── index.html
        ├── doge.html
        ├── SSID.json
        ├── log.txt
        ├── logo.bmp
        └── test.txt
    ```

4. **Upload SPIFFS:**
   - Install the **ESP32 Sketch Data Upload** tool from [here](https://github.com/esp8266/arduino-esp8266fs-plugin).
   - In Arduino IDE, navigate to **Tools > ESP32 Sketch Data Upload** to upload the `data` folder to the SPIFFS.

### 4. Upload Firmware

1. **Connect M5Dial:**
   - Connect your M5Dial device to your computer via USB.

2. **Select Board and Port:**
   - In Arduino IDE, go to **Tools > Board** and select the appropriate ESP32 board.
   - Select the correct **Port** under **Tools > Port**.

3. **Upload Code:**
   - Open `Semi-Evil-M5Dial-v1-1-0.ino`.
   - Click the **Upload** button to compile and upload the firmware to the M5Dial.

## Usage 🛠️

### Navigating the Menu

The M5Dial features a rotary encoder and a button (BtnA) for navigation:

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
   - Place your `.txt` script files in the `data` folder before uploading SPIFFS.

3. **Execute Scripts:**
   - Navigate to **BadUSB** in the main menu.
   - Select the desired script to execute. In Debug mode, scripts can be read via Serial. In BadUSB mode, selected scripts execute upon device reset or power-on.

## Troubleshooting

- **SPIFFS Upload Issues:**
  - Ensure the `data` folder is correctly structured.
  - Verify the **ESP32 Sketch Data Upload** tool is properly installed.

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
# 📟 PagerDeck — Token Notification System

**PagerDeck** is an IoT-enabled, cloud-connected queue tracking device housed in a portable aluminum mint tin. Powered by an ESP32 microcontroller, it fetches live queue status over Wi-Fi from Adafruit IO REST endpoints and alerts the user in real time via an OLED display and piezo buzzer.

---

## 🚀 Key Features
* **Cloud Sync:** Real-time HTTP/JSON synchronization with Adafruit IO feeds.
* **Non-Blocking Logic:** Asynchronous timing loops for seamless screen, buzzer, and button responsiveness.
* **Multi-State UI Engine:** Includes a Welcome Screen, Token Dashboard, Match Alert ("YOUR TURN"), and ID Reminder overlays.
* **Ultra-Compact Enclosure:** Direct USB-powered design built inside an insulated mint tin.

---

## 🛠️ Hardware Setup
* **Microcontroller:** ESP32-WROOM-32
* **Display:** 0.96" SSD1306 OLED (I2C: SDA=GPIO 21, SCL=GPIO 22)
* **Inputs:** 2x Tactile Buttons (GPIO 4: Start/Mute, GPIO 12: Toggle)
* **Audio:** Piezo Buzzer (GPIO 5)
* **Power:** Direct 5V Micro-USB / USB-C

---

## 📁 Repository Structure
* `/src`: ESP32 Firmware C++ source code.
* `/working`: Working of the project.

---

## 📄 License
This project is open-source under the MIT License.

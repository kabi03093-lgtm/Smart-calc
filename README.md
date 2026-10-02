# 📚 CheatBox V3 — ESP32-C3 Smart Formula & Notes Reader

> **A pocket-sized digital formula/reference box designed to reduce the need to carry and search through traditional formula books.**

CheatBox V3 is an **ESP32-C3 based portable file reader** for storing and quickly reading text-based formulas, notes, laws, equations, and reference material.

Instead of carrying multiple small reference books or repeatedly searching through handwritten notes, the device stores text files in **ESP32-C3 LittleFS** and lets the user browse and read them directly from a small OLED display.

It also includes a **Wi-Fi file management system**, allowing files to be uploaded or deleted from a phone or laptop through a browser.

---

## ✨ Project Idea

Students often need to carry formula sheets, important equations, laws, definitions, and short technical notes for different subjects.

The basic idea behind CheatBox is:

```text
Traditional Books / Notes
          ↓
 Search manually
          ↓
 Find the required formula
          ↓
 Read / Memorize
```

CheatBox changes this into:

```text
        📱 / 💻
           │
       Wi-Fi Upload
           │
           ▼
     ┌───────────────┐
     │   ESP32-C3    │
     │               │
     │   LittleFS     │
     │      ↓         │
     │  Text Files    │
     └───────┬───────┘
             │
             ▼
        128 × 32 OLED
             │
             ▼
      Browse & Read
       formulas/notes
```

The project is intentionally simple: **store useful information as lightweight `.txt` files and read it anywhere without an internet connection.**

---

## 🚀 Main Features

- 📖 Store formulas and study notes as text files
- 💾 File storage using **LittleFS**
- 📂 Browse stored files from the OLED
- 👀 Read text directly on the device
- ⬆️ Upload files through Wi-Fi
- 🗑️ Delete files through the web interface
- 📱 Works with a phone or laptop browser
- 🔌 Portable battery-powered concept
- 🎛️ Three-button navigation
- 🌐 ESP32-C3 creates its own Wi-Fi Access Point
- 🔒 Filename validation before file operations
- 💤 OLED display can be turned off using a long press
- ⚡ Lightweight embedded implementation

---

## 🧠 What Makes It "Smart"?

The goal is not to build another calculator with hundreds of buttons.

The useful part is the **digital reference system**:

```text
Subject
 ├── Physics
 │    ├── Coulomb_Law.txt
 │    ├── Newtons_Law.txt
 │    └── Kepler_Law.txt
 │
 ├── Mathematics
 │    ├── Algebraic_Equations.txt
 │    └── Pythagoras_Theorem.txt
 │
 └── Geography
      └── Rivers.txt
```

The same structure can be extended to:

```text
ECE/
├── Digital_Electronics.txt
├── Network_Theory.txt
├── Signals_and_Systems.txt
├── Communication_Systems.txt
└── Microprocessors.txt

Embedded/
├── UART.txt
├── SPI.txt
├── I2C.txt
├── GPIO.txt
└── Interrupts.txt

Programming/
├── C_Syntax.txt
├── Python.txt
└── Git_Commands.txt
```

This turns the ESP32 into a **portable personal knowledge-storage device**.

---

## 🔧 Hardware

### Core Components

| Component | Purpose |
|---|---|
| ESP32-C3 | Main microcontroller |
| 128×32 I2C OLED | Display |
| Push Button ×3 | Navigation |
| Li-Po Battery | Portable power |
| DC-DC / Boost-Converter Module | Power regulation |
| USB / charging circuit | Battery charging |
| Wires / PCB | Interconnection |

### OLED Configuration

The firmware is configured for:

- Resolution: **128 × 32**
- I2C address: **0x3C**
- SDA: **GPIO 6**
- SCL: **GPIO 7**

### Button Configuration

| Button | GPIO | Function |
|---|---:|---|
| UP | GPIO 8 | Move up / scroll up |
| DOWN | GPIO 9 | Move down / scroll down |
| ENTER | GPIO 10 | Select / open / go back |

The firmware configures the buttons using internal pull-ups.

---

## 🔌 Hardware Architecture

```text
                 ┌─────────────────────┐
                 │      Li-Po Battery  │
                 └──────────┬──────────┘
                            │
                            ▼
                   ┌─────────────────┐
                   │ Power Regulation│
                   │ / Boost Module  │
                   └────────┬────────┘
                            │
                            ▼
                   ┌─────────────────┐
                   │    ESP32-C3     │
                   │                 │
                   │   LittleFS      │
                   │      │          │
                   │      ▼          │
                   │  File Storage   │
                   └─────┬─────┬─────┘
                         │     │
                  I2C    │     │ GPIO
                         │     │
                ┌────────▼┐   ┌▼──────────────┐
                │ OLED    │   │ 3 Push Buttons│
                │128×32   │   │ UP/DOWN/ENTER │
                └─────────┘   └───────────────┘

                         Wi-Fi
                           │
                 ┌─────────▼─────────┐
                 │ Phone / Laptop    │
                 │ Web Browser       │
                 └───────────────────┘
```

> **Important:** The exact battery and power-module wiring depends on the hardware module used. Do not assume the pictured converter's terminals are identical to another module.

---

## 📂 Software Architecture

```text
CheatBox V3
│
├── ESP32-C3
│
├── Wi-Fi Access Point
│   ├── ESP_Reader_C3
│   └── 192.168.4.1
│
├── Web Server
│   ├── File Upload
│   ├── File Listing
│   └── File Delete
│
├── LittleFS
│   └── Text File Storage
│
├── OLED UI
│   ├── Main Menu
│   ├── File Browser
│   ├── File Viewer
│   └── Wi-Fi Upload Mode
│
└── Button Controller
    ├── UP
    ├── DOWN
    └── ENTER
```

---

## 🖥️ Device User Interface

The main menu contains two functions:

```text
> Read Files
  WiFi Upload
```

### Read Files

```text
> Formula.txt
  Physics.txt
  Maths.txt
```

Select a file with **ENTER**.

The contents are wrapped automatically to fit the OLED.

### File Viewer

The current implementation displays approximately:

```text
21 characters / line
4 visible lines
```

and allows scrolling using the UP/DOWN buttons.

### Wi-Fi Upload

Selecting Wi-Fi Upload starts the ESP32-C3 as an Access Point.

```text
SSID      : ESP_Reader_C3
Password  : 12345678
IP        : 192.168.4.1
```

Connect your phone/laptop to the ESP32 Wi-Fi network and open:

```text
http://192.168.4.1
```

The web interface provides:

- File upload
- File listing
- File deletion

---

## 🌐 Web File Manager

The browser interface is designed as a simple mobile-friendly file manager.

```text
┌─────────────────────────────┐
│          ESP32-C3           │
│                             │
│       File Upload           │
│  [ Choose File ]            │
│  [     UPLOAD FILE     ]    │
│                             │
│          Files              │
│  ┌───────────────────────┐  │
│  │ /formula.txt  DELETE  │  │
│  │ /notes.txt    DELETE  │  │
│  └───────────────────────┘  │
│                             │
│       192.168.4.1           │
└─────────────────────────────┘
```

The firmware itself generates the web interface and serves it from the ESP32-C3.

---

## 💾 File Storage

CheatBox uses **LittleFS** rather than browser local storage.

Example:

```text
LittleFS
│
├── Coulomb_Law.txt
├── Newtons_Law.txt
├── Kepler_Law.txt
├── Algebraic_Equation.txt
├── Pythagoras_Theorem.txt
└── Rivers.txt
```

This means the files are stored in the ESP32's flash filesystem and can be accessed by the firmware.

---

## 📝 Example Formula File

A simple `.txt` file could contain:

```text
COULOMB'S LAW

F = k(q1q2/r²)

F = Electrostatic force
q1 = First charge
q2 = Second charge
r = Distance
k = Coulomb constant

k = 8.99 × 10^9 N·m²/C²
```

Another file:

```text
OHM'S LAW

V = I × R

V = Voltage
I = Current
R = Resistance

Power:
P = V × I
P = I²R
P = V²/R
```

---

## 🧩 Firmware State Machine

The application is organized into four main states:

```text
             ┌──────────────┐
             │  MAIN MENU   │
             └──────┬───────┘
                    │
          ┌─────────┴──────────┐
          ▼                    ▼
 ┌────────────────┐    ┌────────────────┐
 │ FILE BROWSER   │    │ UPLOAD MODE    │
 └───────┬────────┘    └────────────────┘
         │
         ▼
 ┌────────────────┐
 │ FILE VIEWER    │
 └────────────────┘
```

Firmware states:

```cpp
MAIN_MENU
FILE_BROWSER
FILE_VIEWER
UPLOAD_MODE
```

---

## 🛠️ Software Stack

### Firmware

- Arduino framework
- C/C++
- ESP32-C3
- WiFi library
- WebServer
- LittleFS
- Wire / I2C
- Adafruit GFX
- Adafruit SSD1306

### Web Interface

- HTML
- CSS
- ESP32 WebServer

### Storage

- LittleFS
- Plain-text `.txt` files

---

## 📦 Required Arduino Libraries

Install the following libraries through Arduino IDE Library Manager:

```text
Adafruit GFX Library
Adafruit SSD1306
```

ESP32 support provides:

```text
WiFi
WebServer
LittleFS
Wire
```

---

## ⚙️ Getting Started

### 1. Install ESP32 Board Support

Install the ESP32 board package in Arduino IDE.

Select an ESP32-C3 compatible board.

### 2. Install Libraries

Install:

```text
Adafruit GFX Library
Adafruit SSD1306
```

### 3. Open Firmware

Open:

```text
Cheat_box_V3.ino
```

### 4. Check Pin Configuration

```cpp
#define BTN_UP      8
#define BTN_DOWN    9
#define BTN_ENTER   10

#define I2C_SDA     6
#define I2C_SCL     7
```

### 5. Upload

Compile and upload the firmware to the ESP32-C3.

### 6. Add Formula Files

Use the Wi-Fi upload mode to transfer `.txt` files into LittleFS.

### 7. Read

Use:

```text
UP
DOWN
ENTER
```

to browse and read the stored information.

---

## 🔐 Wi-Fi Configuration

Current firmware configuration:

```cpp
const char* AP_SSID = "ESP_Reader_C3";
const char* AP_PASS = "12345678";

IPAddress apIP(192, 168, 4, 1);
```

For a personal device, change the default password before distributing the project.

---

## 🔋 Power

The pictured hardware uses a rechargeable Li-Po battery and a power-conversion stage.

For a real portable version, the power section should include:

```text
Li-Po Battery
      │
      ├── Protection
      │
      ├── Charging
      │
      ▼
Voltage Regulation
      │
      ▼
ESP32-C3
```

**Do not connect a Li-Po battery directly to a pin unless the board/module is specifically designed for that battery input.** Verify the voltage range and charging/protection circuitry of your exact ESP32-C3 board and converter.

---

## 🚧 Current Limitations

This version is intentionally lightweight and has some limitations:

- OLED resolution limits how much text can be displayed at once.
- The current viewer is designed for text files rather than PDFs/images.
- There is a maximum in-memory file list of **20 files**.
- Wi-Fi is enabled only while the device is in upload mode.
- The web interface currently focuses on upload/list/delete operations.
- There is no search engine for formulas yet.
- There is no category/database system in the current firmware.
- There is no dedicated mathematical expression parser/calculator in the current version.

So the current project is best described as a **portable digital formula/reference reader**, not yet a full scientific calculator.

---

## 🔮 Future Development

### V4 — Smarter Formula System

Possible improvements:

- 🔎 Formula search
- 📚 Subject/category folders
- ⭐ Favourite formulas
- 🕘 Recently opened files
- 🏷️ Tags
- 🧮 Built-in scientific calculator
- 📐 Equation solver
- 🔢 Unit converter
- 📊 Quick reference tables
- 🔤 Better text navigation
- 📱 Improved mobile web dashboard
- 🔐 Wi-Fi authentication
- 💾 Better storage management

### AI-Assisted Future Version

A future version could allow natural-language queries:

```text
User:
"What is the formula for capacitive reactance?"

        ↓

ESP32 / Mobile Interface

        ↓

Formula Database

        ↓

XC = 1 / (2πfC)
```

For an ESP32-C3, the practical approach would be to keep the device responsible for **storage, display, navigation, and hardware control**, while a phone/web application handles heavier AI processing.

---

## 🎯 Project Goals

The long-term goal of CheatBox is to build a compact **personal engineering knowledge device**.

Instead of:

```text
Multiple books
     +
Loose notes
     +
Printed formula sheets
     +
Repeated searching
```

the target is:

```text
             ┌──────────────────┐
             │    CHEATBOX      │
             │                  │
             │  📚 Knowledge    │
             │  🧮 Calculator   │
             │  🔎 Search       │
             │  📐 Formulas     │
             │  📡 Wi-Fi        │
             │  💾 Storage      │
             └──────────────────┘
```

---

## 📸 Project Hardware

Add your hardware/wiring image to the repository, for example:

```text
docs/
└── hardware.jpg
```

Then display it here:

```markdown
![CheatBox V3 Hardware](docs/hardware.jpg)
```

---

## 📁 Repository Structure

Recommended repository structure:

```text
CheatBox-V3/
│
├── README.md
├── firmware/
│   └── Cheat_box_V3.ino
│
├── web/
│   └── dash.html
│
├── docs/
│   └── hardware.jpg
│
└── formulas/
    ├── Physics/
    ├── Mathematics/
    ├── Electronics/
    └── Embedded/
```

---

## 👨‍💻 Project

**CheatBox V3**  
Built around **ESP32-C3 + OLED + LittleFS + Wi-Fi**

Developed as an ECE/Embedded Systems project focused on creating a practical, low-cost portable reference device.

---

## ⭐ If You Like This Project

You can improve it, fork it, add your own formula database, design a custom PCB, or extend the firmware with a calculator and search system.

**From carrying information in books → to carrying it in a tiny embedded device.**

---

### 📜 License

Choose a license appropriate for your project before publishing. For example:

```text
MIT License
```

if you want others to freely reuse and modify the project.

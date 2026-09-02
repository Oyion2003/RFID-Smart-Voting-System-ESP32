# RFID-Based Smart Electronic Voting System Using ESP32

A smart electronic voting system built with an **ESP32 microcontroller**, an **RC522 RFID module**, push buttons, LEDs, and a buzzer. The system authenticates voters using RFID cards, prevents duplicate voting, supports vote undo, runs a timed voting session, and produces a final result with voter-level accountability tracking.

---

## 📌 Project Overview

This project allows users to:

- Authenticate using an RFID card
- Participate in an active voting session
- Vote **YES** or **NO**
- Prevent duplicate voting
- Undo a vote before the session ends
- Receive real-time **LED** and **buzzer** feedback
- Automatically close voting after a set session time
- View the final result (YES / NO / DRAW)
- Track voter UID and voting activity for accountability

The project combines **Embedded Systems**, **IoT**, **RFID Technology**, **Digital Logic**, and **Microcontroller Programming**.

---

## 🧰 Hardware Used

| Component | Purpose |
|---|---|
| ESP32 Dev Board (CP2102) | Main microcontroller |
| RC522 RFID Module | RFID card reading and authentication |
| RFID Cards/Tags | Voter identification |
| Push Buttons (x3) | YES, NO, and UNDO voting controls |
| LEDs | Visual system feedback |
| Buzzer | Audio feedback |
| Breadboard | Circuit prototyping |
| Jumper Wires | Component connections |
| Micro-USB Data Cable | Programming and powering the ESP32 |

**Board used:** ESP32 Development Board with **CP2102 USB-to-UART converter**, using a **Micro-USB** port (not USB-C).

---

## 🔌 Pin Configuration

| Component | ESP32 GPIO |
|---|---:|
| RFID SS/SDA | GPIO 5 |
| RFID RST | GPIO 22 |
| YES Button | GPIO 32 |
| NO Button | GPIO 33 |
| UNDO Button | GPIO 25 |
| Session Control | GPIO 16 |
| Authentication LED | GPIO 13 |
| Time LED | GPIO 14 |
| Enable LED | GPIO 2 |
| Duplicate LED | GPIO 17 |
| Event LED | GPIO 21 |
| YES LED | GPIO 26 |
| NO LED | GPIO 4 |
| UNDO LED | GPIO 27 |
| Buzzer | GPIO 15 |

### RFID (RC522) SPI Wiring

| RC522 Pin | ESP32 Pin |
|---|---|
| SDA/SS | GPIO 5 |
| SCK | GPIO 18 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| RST | GPIO 22 |
| VCC | 3.3V |
| GND | GND |

All buttons use `INPUT_PULLUP` (normal state = HIGH, pressed = LOW), which removes the need for external pull-up resistors.

---

## 🧠 How It Works

1. On boot, the ESP32 asks for the **voting question** and **session time** (in seconds) via the Serial Monitor.
2. Once entered, the voting session starts — the `SESSION`, `ENABLE_LED`, and `TIME_LED` pins go HIGH.
3. Voters scan their RFID card:
   - **New card** → registered as a new voter, authentication LED and beep confirm success.
   - **Existing card, not yet voted** → allowed to vote.
   - **Existing card, already voted** → rejected with a duplicate alert (LED + higher-pitch beep).
4. The authenticated voter presses **YES** or **NO** to cast a vote. The vote is locked to that voter's UID.
5. The **UNDO** button lets the currently active voter cancel their vote before the session ends.
6. When the session timer expires:
   - Voting is locked (`systemLocked = true`)
   - Final results are calculated and printed to Serial
   - The system shuts down its indicators until the ESP32 is reset

### Voting Logic (AND-gate style condition)

A vote is only accepted when **all** of the following are true:

```
RFID User Authenticated
        AND
Voting Session Active
        AND
System Not Locked
        AND
User Has Not Already Voted
        ↓
   Voting Enabled
```

### System Workflow

```
POWER ON ESP32
      ↓
Enter Voting Question
      ↓
Enter Session Time
      ↓
Voting Session Starts
      ↓
RFID Card Scan → New or Existing?
      ↓
Duplicate? ──Yes──> Reject + Alert
      │
      No
      ↓
YES / NO Selection → Store Vote → LED + Buzzer Feedback
      ↓
Wait for Session Timer
      ↓
Session Ends → Calculate Result → Show Winner & Stats
      ↓
System Shutdown (reset to start a new session)
```

---

## 🗂️ Voter Data & Accountability

Each voter is tracked using a `Voter` struct:

```cpp
struct Voter {
  String uid;
  bool voted;
  int vote;              // 1 = YES, 0 = NO, -1 = not voted
  unsigned long firstSeen;
  unsigned long voteTime;
};
```

The system stores up to **20 voters** (`MAX_VOTERS`) per session and records, for each voter:

- RFID UID
- Vote cast (YES / NO / none)
- Time first detected (`firstSeen`)
- Time the vote was cast (`voteTime`)
- Decision time (`voteTime - firstSeen`)

At the end of a session, results are printed in this format:

```
===== FINAL RESULT =====
QUESTION: <your question>
YES COUNT: <n>
NO COUNT: <n>
WINNER: YES WINS 🟢 / NO WINS 🔴 / DRAW

UID | VOTE | WIN_TIME(s) | LOSE_TIME(s) | DIFF(s)
```

---

## 💻 Full Source Code

The complete Arduino sketch (`voting_system.ino`) is included in this repository. Key libraries used:

```cpp
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
```

- **Arduino.h** — core Arduino functionality
- **SPI.h** — required for SPI communication with the RC522 module
- **MFRC522.h** — handles RFID card detection, UID reading, and communication with the RC522

> See [`voting_system.ino`](./voting_system.ino) in this repo for the full code.

---

## 🖥️ Simulation

A live simulation of the circuit and code is available on Wokwi:

**Simulation Link (Wokwi Project):**
[https://wokwi.com/projects/462842720344631297](https://wokwi.com/projects/462842720344631297)

Hardware and software video simulation: https://www.linkedin.com/posts/md-arif-oyion_esp32-rfid-embeddedsystems-activity-7471774511030247424-aBE6?utm_source=share&utm_medium=member_desktop&rcm=ACoAADI2ZkoBOPomn2vd2kZm5vHga4MbHUtKsKY
---

## ⚙️ Arduino IDE Setup Guide

### 1. Install Arduino IDE

Download and install the [Arduino IDE](https://www.arduino.cc/en/software).

### 2. Add ESP32 Board Support

Go to:
```
File → Preferences   (or "Arduino IDE → Settings" on some versions)
```

Under **Additional Board Manager URLs**, add:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

Click **OK**.

### 3. Install the ESP32 Board Package

Go to:
```
Tools → Board → Boards Manager
```

Search for **ESP32** and install **"ESP32 by Espressif Systems"**.

### 4. Connect the ESP32 and Select Port

Connect the ESP32 (CP2102) board using a **working Micro-USB data cable** (see the USB troubleshooting section below — this matters more than you'd expect!).

Then go to:
```
Tools → Port
```
and select the COM port that appears (e.g. `COM3`, `COM4`, etc.).

### 5. Select the Correct Board

Go to:
```
Tools → Board → ESP32 Arduino → ESP32 Dev Module
```

| Setting | Selection |
|---|---|
| Board | ESP32 Dev Module |
| USB Converter | CP2102 |
| USB Port Type | Micro-USB |
| Port | Your assigned COM port |

### 6. Install Required Libraries

Go to:
```
Sketch → Include Library → Manage Libraries
```

Search for and install:
```
MFRC522
```

(`SPI` and `Arduino` core libraries are included by default.)

### 7. Test the Board (Optional but Recommended)

Before wiring the full circuit, upload a simple blink sketch to confirm the board, cable, and COM port all work correctly:

```cpp
void setup() {
  pinMode(2, OUTPUT);
}

void loop() {
  digitalWrite(2, HIGH);
  delay(1000);
  digitalWrite(2, LOW);
  delay(1000);
}
```

GPIO 2 is also used as `ENABLE_LED` in the final project, so this doubles as a good sanity check.

### 8. Upload the Final Code

Open `voting_system.ino`, select the correct board and port, then click **Upload**. Open the **Serial Monitor** at **115200 baud** to interact with the system (enter the question and session time).

---

## 🔧 USB Cable Troubleshooting (Real Issue We Faced)

Our ESP32 board used an older-style **Micro-USB** port instead of USB-C, and it did **not** come with a USB cable in the box. This turned out to be one of the biggest early roadblocks.

**What happened:**

- We first tried old Micro-USB phone cables (our own and teammates').
- The ESP32 would receive **power** through these cables, but the Arduino IDE could **not** communicate with the board — no COM port, no upload.
- We borrowed a cable from a friend who was already working on an ESP32 project. Using his cable, the board connected and worked immediately.
- This confirmed the problem: our old cables were either **charging-only** or low-quality cables that didn't support proper **data transfer**.
- We then bought a new Micro-USB data cable, testing it in the store first to confirm the ESP32 was detected, a COM port appeared, and code uploaded successfully — before buying it.

**Lesson learned:** A USB cable can power a microcontroller while still failing to upload code, if it doesn't support data communication. If your ESP32 (or any Arduino-compatible board) isn't showing up as a COM port, **check your cable first** — it's one of the most common and overlooked issues.

---

## ✅ Final Outcome

- Reads RFID cards and registers voters per session
- Prevents duplicate voting
- Supports YES / NO voting with an UNDO option
- Enforces a fixed voting session duration
- Provides LED and buzzer feedback for every event
- Tracks per-voter accountability data (UID, vote, timing)
- Calculates and displays the final result and winner

---

## 📚 What We Learned

- Setting up Arduino IDE for ESP32 development
- Adding ESP32 board support and working with the CP2102 USB-UART chip
- Diagnosing "power-only" vs. true data USB cables
- SPI communication and RFID (RC522) integration
- Managing multiple digital inputs/outputs (buttons, LEDs, buzzer)
- Implementing duplicate-prevention and session-based logic
- Applying digital logic concepts (AND-gate style conditions) in embedded code
- Debugging real hardware end-to-end, from cable issues to final results

---

## 👥 Team

Developed by a team of five members for a Spring Semester 2026 embedded systems project 
.

Md Arif Mahmud Oyion,
Md Bony Amin, 
Md Rakibul Islam, 
Niloy Bormon,  
Mohona 
American International University- Bangladesh

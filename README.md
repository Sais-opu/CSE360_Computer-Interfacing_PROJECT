# 🐾 Pet Health Monitoring and Automated Feeding System

An **IoT-enabled Pet Health Monitoring and Automated Feeding System** developed for the **CSE360: Computer Interfacing** course at **BRAC University**.

The system uses an **Arduino Uno**, sensors, actuators, LCD, buzzer, and Bluetooth communication to automate pet feeding, monitor environmental conditions, and track pet activity.

---

## 🎥 Project Demonstration

### ▶️ [Watch Project Demonstration Video](https://github.com/user-attachments/assets/7ec8527b-071b-4719-8bf8-acbb99bfdff8)

The complete project demonstration video is also available in the repository:

**Path:** `Project-demonstrate-video/project_video.mp4`


---

## 📌 Project Overview

The **Pet Health Monitoring and Automated Feeding System** is designed to help pet owners monitor and maintain their pets' daily feeding and environmental conditions, especially when they cannot constantly supervise them.

### Key Features

- 🍽️ Automatic food dispensing
- 🌡️ Temperature and humidity monitoring
- 💨 Automatic cooling using a DC fan
- 🐾 Pet activity monitoring
- 🍖 Food intake monitoring
- 💧 Water-level monitoring
- 🔔 Audible alerts using a buzzer
- 📱 Bluetooth-based notifications
- 🖥️ LCD-based system status display
- 🛡️ Food dispenser overflow prevention
- ⏰ 24-hour inactivity alert

---

## 🎓 Academic Information

**Course:** CSE360 – Computer Interfacing  
**Project:** Lab Project  
**Title:** Pet Health Monitoring and Automated Feeding System  
**Group:** Group 4  
**Section:** 8  
**University:** BRAC University  
**Department:** Computer Science and Engineering  
**Date:** 30 August 2026

---

## 👥 Group Members

- **Rafi Uddin Auntik**
- **Naved Abrar Nibir**
- **Md Saidul Islam Apu**
- **Fariha Yasmin**

---

## 🎯 Objectives

The main objectives of this project are:

1. Automate the feeding process for domestic pets.
2. Monitor temperature and humidity around the pet's environment.
3. Automatically activate a cooling fan when environmental conditions become unfavorable.
4. Monitor pet activity around the feeding station.
5. Track food intake using IR sensors.
6. Monitor water levels using a water-level sensor.
7. Generate alerts when abnormal conditions are detected.
8. Send important information to the pet owner through Bluetooth.
9. Display system information locally using an LCD.
10. Prevent food dispenser overflow by monitoring the food container.

---

## 🏗️ System Architecture

The **Arduino Uno** acts as the central processing unit of the system.

### Inputs

- **DHT11 Sensor** – Measures temperature and humidity.
- **IR Sensors** – Detect pet presence and monitor food intake.
- **Water Level Sensor** – Monitors the water level.

### Processing

- **Arduino Uno** processes sensor readings and controls the connected actuators and alert systems.

### Outputs

- **Servo Motors** – Control the food dispenser.
- **DC Fan** – Provides automatic cooling.
- **LCD Display** – Shows system status.
- **Buzzer** – Generates local alerts.
- **Bluetooth Module** – Sends information and alerts to the owner's smartphone.

---

## 🔧 Components Used

| Component | Purpose |
|---|---|
| Arduino Uno | Main microcontroller |
| DHT11 Sensor | Temperature and humidity monitoring |
| IR Sensors | Pet activity and food-intake detection |
| Water Level Sensor | Water-level monitoring |
| Servo Motors | Automatic food dispenser control |
| DC Fan | Environmental cooling |
| LCD Display | Local system-status display |
| Buzzer | Audible alerts |
| Bluetooth Module | Remote communication |
| Food Container | Food storage and dispensing |
| Water Container | Pet water supply |

---

## ⚙️ How the System Works

### 1. Automatic Feeding

Servo motors are used to control the food dispenser. At the scheduled feeding time, the Arduino activates the servo motor to open the dispenser and provide food to the pet.

The system also monitors the food dispensing process to help prevent unnecessary dispensing and food overflow.

### 2. Temperature and Humidity Monitoring

The **DHT11 sensor** continuously measures the surrounding temperature and humidity.

If the environmental conditions become unfavorable, the Arduino automatically activates the **DC fan** to improve the surrounding environment.

### 3. Pet Activity Monitoring

IR sensors detect the presence and movement of the pet around the feeding station.

This information helps determine whether the pet is interacting with the feeding area.

### 4. Food Intake Monitoring

The IR sensors are used to monitor food-related activity.

If the pet does not interact with the food dispenser for an extended period, the system can generate an alert for the owner.

### 5. Water-Level Monitoring

A water-level sensor monitors the availability of water for the pet.

When the water level becomes low, the system can notify the owner through the alert system.

### 6. Alert System

A buzzer provides local audible alerts when important conditions are detected.

Possible alert conditions include:

- No food intake
- Low water level
- Unfavorable environmental conditions
- Extended pet inactivity
- Other predefined system conditions

### 7. Bluetooth Communication

A Bluetooth module allows the system to send important information and alerts to the owner's smartphone.

---

## 🔄 System Workflow

```text
                         ┌─────────────────┐
                         │   Start System  │
                         └────────┬────────┘
                                  │
                                  ▼
                         ┌─────────────────┐
                         │ Initialize      │
                         │ Arduino &       │
                         │ Components      │
                         └────────┬────────┘
                                  │
                                  ▼
                         ┌─────────────────┐
                         │ Read Sensors    │
                         └────────┬────────┘
                                  │
                ┌─────────────────┼─────────────────┐
                │                 │                 │
                ▼                 ▼                 ▼
        ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
        │ Temperature  │  │ Pet Activity │  │ Water Level  │
        │ & Humidity   │  │ / Food       │  │ Monitoring   │
        └──────┬───────┘  └──────┬───────┘  └──────┬───────┘
               │                 │                 │
               └─────────────────┼─────────────────┘
                                 │
                                 ▼
                       ┌────────────────────┐
                       │ Check Conditions   │
                       └─────────┬──────────┘
                                 │
               ┌─────────────────┼─────────────────┐
               │                 │                 │
               ▼                 ▼                 ▼
        ┌─────────────┐   ┌─────────────┐   ┌─────────────┐
        │ Feeding     │   │ High Temp / │   │ Low Water / │
        │ Required    │   │ Humidity    │   │ Inactivity  │
        └──────┬──────┘   └──────┬──────┘   └──────┬──────┘
               │                 │                 │
               ▼                 ▼                 ▼
        ┌─────────────┐   ┌─────────────┐   ┌─────────────┐
        │ Servo Motor │   │ DC Fan      │   │ Buzzer +    │
        │ Activated   │   │ Activated   │   │ Bluetooth   │
        └─────────────┘   └─────────────┘   └─────────────┘
                                 │
                                 ▼
                       ┌────────────────────┐
                       │ Update LCD Display │
                       └─────────┬──────────┘
                                 │
                                 ▼
                       ┌────────────────────┐
                       │ Continue Monitoring│
                       └─────────┬──────────┘
                                 │
                                 └──────────► LOOP



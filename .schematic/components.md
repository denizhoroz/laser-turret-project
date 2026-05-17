# Overview
This components.md file explains the components information of the laser turret, PIN layout, quantity, model etc.

# Component List 

## Name (Quantity)
1) Jetson Orin Nano (1)
2) Arduino MEGA (1)
3) 12V AC-DC Power Supply (1)
4) Motor Driver DRV8825 (2)
5) Stepper Motor NEMA 17HS4401 (2)
6) LED (3)
7) Limit Switch (4)
8) 5V 650nm Laser Module (1)
9) IMX179 USB Camera (1)
10) Ground Control System (1) 

# Component Schematic

## Jetson Schematic
```
Jetson Nano <---> Arduino MEGA
```

### Connection type
- Connected via USB connection

## Arduino MEGA Schematic
```
Arduino MEGA ----> Motor Driver 
               |
               |-> Motor Driver
               |
               |-> Laser
               |
               |-> LED
               |
               |-> LED
               |
               |-> LED
               |
               |-> Limit Switch
               |
               |-> Limit Switch
               |
               |-> Limit Switch
               |
               |-> Limit Switch
```

### Connection Type
- Everything here is connected via GPIO connection.
- Motor driver pins
  - Pitch motor
    - STEP -> **12**
    - DIR -> **11**
  - Yaw motor
    - STEP -> **10**
    - DIR -> **9**
- Laser pin
  - 5V -> **2**
- LED pins
  - Red LED -> **A0**
  - Yellow LED -> **A1**
  - Green LED -> **A2**
- Limit switch pins
  - Left Limit Switch
    - COMMON -> **A12**
    - NC -> **GND**
  - Right Limit Switch
    - COMMON -> **A13**
    - NC -> **GND**
  - Up Limit Switch
    - COMMON -> **A14**
    - NC -> **GND**
  - Down Limit Switch
    - COMMON -> **A15**
    - NC -> **GND**

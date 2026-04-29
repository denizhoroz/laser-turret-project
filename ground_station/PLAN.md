# Jetson Nano Ground Station Web App

## Overview
- Jetson Nano will connect to a FastAPI server on a Windows laptop via USB serial cable.
- Jetson will not run the FastAPI, it will just receive/send data through the serial connection.
- Jetson is going to make detections itself. Then broadcast the video and detection information through the serial connection.
- Jetson will also send various sensor information it reads from Arduino MEGA card.
- Jetson will receive start/stop signals and mission information to execute from the ground station.

## Data I/O Overview

### Input (To Ground Station, From Jetson Nano)
- 4 Limit Switch activity information. (Active, Not Active)
- Jetson Temperature (integer)
- Motor Y Dir (-1, 1, 0)
- Motor P Dir (-1, 1, 0)
- Laser activity (Active, Not Active)
- Current status (Online, Scanning, Targeting, Firing)

### Output (From Ground Station, To Jetson Nano)
- Start mission signal (Mission type)
- Stop mission

## Methadology & Technical Specifications

- Video Stream Choice: MJPEG
- Camera: USB Cam
- Turret Control: Ground station will not be controlling the turret. Ground station will only give the mission information. (1 and 2. mission)
- Single user
- No auth needed, serial connection
- Real time broadcast

## Front-end Design

- Overall the website will be a professional engineer GUI interface with minimalistic design prioritizing functionality over style.
- Website color is dark mode, color palette is (light black, lime green, gray, etc.), prefer pastel colors instead of high contrast colors.
- Do not use rounded elements, choose rectangular parts.
- Add a header title called "Turret Ground Station"
- Add two horizontal box sections (left and right).
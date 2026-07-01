<img width="1918" height="1079" alt="cover" src="https://github.com/user-attachments/assets/c1382bca-6485-4c5d-945e-121b319ea1b7" />


# 🎯 Laser Turret for Stationary and Moving Targets

An affordable, fully autonomous laser turret that **detects, tracks, and fires** at targets using AI-based image processing. Built as a Mechatronics Engineering graduation project at Istanbul Ticaret University.

![License](https://img.shields.io/badge/license-MIT-green)
![Python](https://img.shields.io/badge/python-3.12%2B-blue)

## 🌟 Highlights

- 🧠 **Real-time AI detection** — custom-trained YOLO model(s) runs on an NVIDIA Jetson Orin Nano
- 🔭 **2-DOF turret** — pitch + yaw driven by NEMA 17 steppers via DRV8825 drivers, closed-loop PID tracking
- 🛰️ **Live ground station** — browser dashboard streams the camera feed with detection overlays and dispatches missions
- 🛡️ **Safety-first** — mechanical limit switches, scan-limit recovery, and a supervisor that auto-restarts on failure

## ℹ️ Overview

The system splits into three cooperating nodes, one per top-level folder:

| Folder | Node | Role |
| --- | --- | --- |
| [`jetson_mission_codes/`](jetson_mission_codes/) | **Jetson Orin Nano** | Captures the camera feed, runs YOLO detection + target tracking, and sends aim/fire commands to the Arduino over USB serial. |
| [`arduino_codes/`](arduino_codes/) | **Arduino Mega 2560** | Turret firmware: drives the stepper motors (STEP/DIR), the laser, and status LEDs; reads limit switches for safety. Also contains the moving-target sketch (Arduino UNO). |
| [`ground_station/`](ground_station/) | **Ground Station** | FastAPI web app on the operator's laptop. Relays missions to the Jetson and streams telemetry + video to the browser. |

Each folder has its own README with setup and usage details.

### 🔄 System Architecture
<img width="1389" height="481" alt="systems" src="https://github.com/user-attachments/assets/65cd9db8-4eb3-447b-9d62-e99f91323562" />

### 🎮 Missions

The Ground Station sends a numeric mission type; the Jetson maps it to a mission module:

| Mission number | Report testbed |
| --- | --- |
| `1` | Stationary target testbed (5 shapes) |
| `2` | Non-stationary (moving) target testbed |

## 📄 License

Released under the [MIT License](LICENSE).

# Jetson Mission Code

The turret's brain. Runs on the **NVIDIA Jetson Orin Nano**: it captures the camera feed, runs YOLO detection and target tracking, drives the Arduino over serial, and reports to the Ground Station.

## 🌟 Highlights

- 👁️ **YOLO detection** on GPU (CUDA) via [Ultralytics](https://docs.ultralytics.com/), with a trained weight file per target set
- 🎯 **Target tracking** — confirmation frames, loss timeout, and re-acquisition logic for stable locks
- 🧵 **Resilient supervisor** — missions run in a worker thread; a crash can't take down the process, and sessions auto-reconnect
- 🔌 **Two links** — TCP to the Ground Station and USB serial to the Arduino (with a background reader + auto-reconnect)
- 🚀 **Deploy as a service** — ships with a `systemd` unit for hands-off boot start

## ℹ️ Overview

```
jetson_mission_codes/
├── requirements.txt            # full pinned desktop/dev environment
├── requirements-jetson.txt     # minimal on-device set (torch preinstalled by JetPack)
├── deploy/
│   └── laser-turret.service    # systemd unit
└── src/
    ├── main.py                 # supervisor: connect → run session → restart on drop
    ├── serial_test.py          # standalone Arduino serial check
    └── packages/
        ├── config.py           # ports, camera index, model paths, tracking constants
        ├── detector.py         # YOLO inference wrapper
        ├── tracker.py          # target selection / lock / loss handling
        ├── state.py            # SystemState: state machine + frame/data senders
        ├── link.py             # TCP client to the Ground Station
        ├── serial.py           # (ArduLink) USB serial to the Arduino
        ├── missions/           # m1 (stationary), m2 (moving), test_detect (td)
        └── models/             # model-target1/best.pt, model-target2/best.pt
```

### 🔄 How it works

`main.py` runs a supervisor loop:

1. Connect to the Ground Station (`Link`) and open the Arduino serial port (`ArduLink`).
2. Wait for `mission_start` / `mission_stop` messages and run the selected mission on a worker thread.
3. During a mission, stream JPEG frames + detection boxes to the Ground Station and push target offsets to the Arduino (rate-limited so the Arduino's 64-byte UART buffer never overflows).
4. If the link drops, tear down cleanly and reconnect after a short backoff.

| Mission | Module | Purpose |
| --- | --- | --- |
| `m1` | `missions/m1.py` | Stationary target testbed |
| `m2` | `missions/m2.py` | Moving target testbed |

### ⚙️ Environment variables

| Variable | Default | Purpose |
| --- | --- | --- |
| `GS_HOST` | `192.168.55.100` | Ground Station address (USB-gadget net) |
| `GS_PORT` | `9001` | Ground Station port |
| `ARDUINO_PORT` | auto-detected¹ | Arduino serial device |
| `ARDUINO_BAUDRATE` | `115200` | Must match the firmware |

¹ Auto-detect tries `COM8` on Windows and `/dev/ttyUSB*` / `/dev/ttyACM*` on Linux. Camera index (`CAMERA=1`), compute `DEVICE=cuda`, and model paths are set in [`src/packages/config.py`](src/packages/config.py).

## 🧷 Run at boot (systemd)

[`deploy/laser-turret.service`](deploy/laser-turret.service) starts the supervisor automatically. It expects the repo at `/home/jetson/laser-turret/`, a `venv/` there, and a `.env` file for the environment variables above.

```bash
sudo cp deploy/laser-turret.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now laser-turret.service
journalctl -u laser-turret -f          # follow logs
```

> The unit runs as user `jetson` with `dialout` (serial) and `video` (camera) groups. Adjust the paths/user if your layout differs.

## 🔗 Related

- [Arduino firmware](../arduino_codes/) — the serial peer
- [Ground Station](../ground_station/) — the TCP peer
- Project root [README](../README.md)

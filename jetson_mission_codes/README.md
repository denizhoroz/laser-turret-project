# 🧠 Jetson Mission Code

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
| `td` | `missions/test_detect.py` | Detection-only bring-up test |

## ⬇️ Installation

Requires **Python 3.12+**, a CUDA-capable device, and the Arduino connected over USB.

**On the Jetson (JetPack ships PyTorch/torchvision):**

```bash
cd jetson_mission_codes
python3 -m venv venv
source venv/bin/activate
pip install -r requirements-jetson.txt
```

**On an x86 dev machine (fully pinned, includes CUDA torch):**

```bash
pip install -r requirements.txt
```

> ⚠️ `requirements.txt` pins `torch==2.11.0+cu128` / `torchvision==0.26.0+cu128` (CUDA 12.8), intended for a desktop dev box. On the Jetson, use `requirements-jetson.txt` instead and rely on the JetPack-provided PyTorch build — mixing the two will break the CUDA setup.

## 🚀 Usage

Run from the `src/` directory so the `packages` import path resolves:

```bash
cd src
python main.py
```

The supervisor will connect to the Ground Station, then wait for a mission to be dispatched from the dashboard. Logs stream to the console and rotate into `jetson_mission_codes/logs/jetson.log`.

To sanity-check the Arduino link on its own:

```bash
python serial_test.py
```

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

## 🧠 Models

`src/packages/models/` holds a trained YOLO weight file (`best.pt`) per target set — `model-target1` for the stationary shapes, `model-target2` for the moving target. Swap in your own weights by replacing these files (see the training notebook in the project report, Appendix B).

## 🔗 Related

- [Arduino firmware](../arduino_codes/) — the serial peer
- [Ground Station](../ground_station/) — the TCP peer
- Project root [README](../README.md)

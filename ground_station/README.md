# 🛰️ Ground Station

The operator's control panel. A **FastAPI + Uvicorn** web app that relays missions to the Jetson and streams live telemetry and camera frames to a browser dashboard.

## 🌟 Highlights

- 🖥️ **Browser dashboard** — live video with detection overlays, mission controls, and a rolling log
- 🔌 **Two links in one process** — a TCP server for the Jetson and a WebSocket fan-out for browser clients
- ⚡ **Real-time** — Jetson frames and state changes are pushed to every connected browser instantly
- 🧷 **Fail-safe** — refuses to start a mission when the Jetson link is down and auto-stops missions if it drops

## ℹ️ Overview

```
ground_station/
├── requirements.txt
└── src/
    ├── main.py                 # FastAPI app + WebSocket manager + Jetson link wiring
    ├── backend/
    │   ├── link.py             # (JetsonLink) TCP transport to the Jetson
    │   └── routes/             # mission / status / telemetry / video route handlers
    └── frontend/
        ├── templates/          # Jinja2 dashboard (index.html + partials)
        └── static/             # CSS + JS modules (ws, mission, overlay, log, ...)
```

### 🔄 Data flow

```
[Browser] ──HTTP/WebSocket (:8000)──► [Ground Station] ──TCP JSON (:9001)──► [Jetson]
```

- The Jetson connects **in** to the Ground Station's TCP server (default `0.0.0.0:9001`).
- Browsers connect to the web server on port **8000** and subscribe to `/ws/telemetry`.
- Incoming Jetson messages (`state`, `frame`, `data`, `mission`) are broadcast to all browser clients; mission commands flow the other way.

## ⬇️ Installation

Requires **Python 3.12+**.

```bash
cd ground_station
python -m venv venv
source venv/bin/activate        # Windows (PowerShell): .\venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

Key dependencies: [FastAPI](https://fastapi.tiangolo.com/), [Uvicorn](https://www.uvicorn.org/), [Jinja2](https://jinja.palletsprojects.com/), and [`websockets`](https://websockets.readthedocs.io/).

## 🚀 Usage

From the `ground_station/` directory:

```bash
uvicorn src.main:app --reload --port 8000
```

Then open **http://127.0.0.1:8000**, choose a mission, and press **Start**.

### ⚙️ Environment variables

| Variable | Default | Purpose |
| --- | --- | --- |
| `JETSON_LINK_HOST` | `0.0.0.0` | Interface the Jetson TCP server binds to |
| `JETSON_LINK_PORT` | `9001` | Port the Jetson connects to |

> ⚠️ The Jetson reaches the Ground Station over the Orin Nano's USB-gadget network (`jetson 192.168.55.1`, `laptop 192.168.55.100`). Make sure the laptop holds that address so the Jetson's default `GS_HOST` resolves — or override `GS_HOST` on the Jetson side.

## 🔌 API surface

| Method | Path | Body / Notes |
| --- | --- | --- |
| `GET` | `/` | Dashboard (HTML) |
| `GET` | `/api/status` | Link + connected-client status |
| `POST` | `/api/mission/start` | `{"mission_type": 1 \| 2 \| 3}` → `m1` / `m2` / `td` |
| `POST` | `/api/mission/stop` | Stops the active mission |
| `WS` | `/ws/telemetry` | Push channel: `connection`, `telemetry`, `frame`, `log`, `jetson_data` |

## 🔗 Related

- [Jetson mission code](../jetson_mission_codes/) — the TCP client on the other end
- Project root [README](../README.md)

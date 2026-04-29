"""FastAPI server for the Turret Ground Station.

Hosts the frontend, exposes mission start/stop endpoints, and bridges to the
Jetson over a JSON-line link (TCP for dev, swap to pyserial for prod USB).

Run from ground_station/ :
    uvicorn src.backend.main:app --reload --port 8000
"""

from __future__ import annotations

import asyncio
import json
import logging
import os
import random
from contextlib import asynccontextmanager
from pathlib import Path
from typing import Set

from fastapi import FastAPI, Request, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from pydantic import BaseModel

from .link import JetsonLink

log = logging.getLogger("ground_station")
logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(name)s: %(message)s")

FRONTEND_DIR = Path(__file__).resolve().parents[1] / "frontend"
TEMPLATES_DIR = FRONTEND_DIR / "templates"
STATIC_DIR = FRONTEND_DIR / "static"

templates = Jinja2Templates(directory=str(TEMPLATES_DIR))

MISSION_TYPE_MAP = {1: "m1", 2: "m2"}


class WSManager:
    def __init__(self) -> None:
        self.clients: Set[WebSocket] = set()

    async def connect(self, ws: WebSocket) -> None:
        await ws.accept()
        self.clients.add(ws)

    def disconnect(self, ws: WebSocket) -> None:
        self.clients.discard(ws)

    async def broadcast(self, msg: dict) -> None:
        if not self.clients:
            return
        data = json.dumps(msg)
        dead = []
        for ws in self.clients:
            try:
                await ws.send_text(data)
            except Exception:
                dead.append(ws)
        for ws in dead:
            self.disconnect(ws)


ws_manager = WSManager()
jetson_link = JetsonLink(
    host=os.environ.get("JETSON_LINK_HOST", "127.0.0.1"),
    port=int(os.environ.get("JETSON_LINK_PORT", "9001")),
)


async def on_jetson_message(msg: dict) -> None:
    """Forward Jetson-originated frames to all WS clients."""
    mtype = msg.get("type")
    if mtype == "state":
        state = msg.get("state")
        await ws_manager.broadcast({
            "type": "telemetry",
            "data": {"status": str(state).capitalize() if state else "Unknown"},
        })
        await ws_manager.broadcast({"type": "log", "text": f"Jetson state -> {state}"})
    elif mtype == "mission":
        await ws_manager.broadcast({"type": "log", "text": f"Jetson mission -> {msg.get('mission')}"})
    elif mtype == "data":
        await ws_manager.broadcast({"type": "jetson_data", "key": msg.get("key"), "value": msg.get("value")})
    elif mtype == "frame":
        await ws_manager.broadcast({
            "type": "frame",
            "w": msg.get("w"),
            "h": msg.get("h"),
            "jpeg": msg.get("jpeg") or "",
            "boxes": msg.get("boxes", []),
        })
    elif mtype == "hello":
        await ws_manager.broadcast({"type": "log", "text": "Jetson said hello"})
    else:
        await ws_manager.broadcast({"type": "log", "text": f"Jetson: {json.dumps(msg)}"})


async def on_jetson_connect(connected: bool) -> None:
    await ws_manager.broadcast({"type": "connection", "connected": connected})
    await ws_manager.broadcast({
        "type": "log",
        "text": "Jetson link UP" if connected else "Jetson link DOWN",
    })


jetson_link.on_message = on_jetson_message
jetson_link.on_connect = on_jetson_connect


async def fake_telemetry_loop() -> None:
    """Synthetic telemetry while Jetson is not connected."""
    statuses = ["Online", "Scanning", "Targeting", "Firing"]
    tick = 0
    try:
        while True:
            await asyncio.sleep(0.5)
            if jetson_link.connected:
                continue  # real link in charge
            tick += 1
            await ws_manager.broadcast({
                "type": "telemetry",
                "data": {
                    "limit_switches": [bool(random.getrandbits(1)) for _ in range(4)],
                    "temp": 40 + random.randint(0, 25),
                    "yaw_dir": random.choice([-1, 0, 1]),
                    "pitch_dir": random.choice([-1, 0, 1]),
                    "laser": tick % 7 == 0,
                    "status": statuses[(tick // 4) % len(statuses)],
                },
            })
    except asyncio.CancelledError:
        raise


@asynccontextmanager
async def lifespan(app: FastAPI):
    await jetson_link.start()
    fake_task = asyncio.create_task(fake_telemetry_loop())
    log.info("Ground station ready")
    try:
        yield
    finally:
        fake_task.cancel()
        try:
            await fake_task
        except asyncio.CancelledError:
            pass
        await jetson_link.stop()


app = FastAPI(lifespan=lifespan)
app.mount("/static", StaticFiles(directory=str(STATIC_DIR)), name="static")


@app.middleware("http")
async def no_cache_static(request: Request, call_next):
    """Dev: prevent browsers from caching JS/CSS so edits take effect on reload."""
    response = await call_next(request)
    if request.url.path.startswith("/static/"):
        response.headers["Cache-Control"] = "no-store, no-cache, must-revalidate, max-age=0"
        response.headers["Pragma"] = "no-cache"
        response.headers["Expires"] = "0"
    return response


@app.get("/", response_class=HTMLResponse)
async def index(request: Request):
    return templates.TemplateResponse(request, "index.html")


@app.get("/video")
async def video():
    return JSONResponse({"detail": "video stream not connected"}, status_code=404)


@app.get("/api/status")
async def status():
    return {
        "connected": jetson_link.connected,
        "ws_clients": len(ws_manager.clients),
    }


class MissionStart(BaseModel):
    mission_type: int


@app.post("/api/mission/start")
async def mission_start(body: MissionStart):
    mission = MISSION_TYPE_MAP.get(body.mission_type)
    if mission is None:
        return JSONResponse({"ok": False, "error": "unknown mission_type"}, status_code=400)

    sent = await jetson_link.send({"type": "mission_start", "mission": mission})
    log.info("MISSION_START mission_type=%s -> jetson sent=%s", body.mission_type, sent)
    await ws_manager.broadcast({
        "type": "log",
        "text": f"GS dispatched MISSION_START({mission}) {'OK' if sent else 'NO_LINK'}",
    })
    return {"ok": sent, "mission": mission, "link_connected": jetson_link.connected}


@app.post("/api/mission/stop")
async def mission_stop():
    sent = await jetson_link.send({"type": "mission_stop"})
    log.info("MISSION_STOP -> jetson sent=%s", sent)
    await ws_manager.broadcast({
        "type": "log",
        "text": f"GS dispatched MISSION_STOP {'OK' if sent else 'NO_LINK'}",
    })
    return {"ok": sent, "link_connected": jetson_link.connected}


@app.websocket("/ws/telemetry")
async def ws_telemetry(ws: WebSocket):
    await ws_manager.connect(ws)
    log.info("WS client connected (%d total)", len(ws_manager.clients))
    try:
        await ws.send_text(json.dumps({"type": "connection", "connected": jetson_link.connected}))
        while True:
            await ws.receive_text()  # ignore inbound
    except WebSocketDisconnect:
        pass
    finally:
        ws_manager.disconnect(ws)
        log.info("WS client disconnected (%d remain)", len(ws_manager.clients))


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="127.0.0.1", port=8000, log_level="info")

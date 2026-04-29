"""Async link to Jetson.

Dev: TCP server on 127.0.0.1:9001 (line-delimited JSON).
Prod: swap to pyserial wrapper with same line-JSON framing.

Single-client (only one Jetson). Reconnect supported by replacing the writer.
"""
from __future__ import annotations

import asyncio
import json
import logging
from typing import Awaitable, Callable, Optional

log = logging.getLogger("ground_station.link")

OnMessage = Callable[[dict], Awaitable[None]]


class JetsonLink:
    def __init__(self, host: str = "127.0.0.1", port: int = 9001) -> None:
        self.host = host
        self.port = port
        self._writer: Optional[asyncio.StreamWriter] = None
        self._server: Optional[asyncio.base_events.Server] = None
        self.on_message: Optional[OnMessage] = None
        self.on_connect: Optional[Callable[[bool], Awaitable[None]]] = None

    @property
    def connected(self) -> bool:
        return self._writer is not None and not self._writer.is_closing()

    async def start(self) -> None:
        # 4 MiB line limit so base64 JPEG frames fit comfortably.
        self._server = await asyncio.start_server(
            self._handle_client, self.host, self.port, limit=4 * 1024 * 1024
        )
        log.info("Jetson link listening on %s:%d", self.host, self.port)

    async def stop(self) -> None:
        if self._writer and not self._writer.is_closing():
            self._writer.close()
        if self._server:
            self._server.close()
            await self._server.wait_closed()

    async def _handle_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        peer = writer.get_extra_info("peername")
        log.info("Jetson connected from %s", peer)
        if self._writer is not None and not self._writer.is_closing():
            log.warning("Replacing existing Jetson connection")
            try:
                self._writer.close()
            except Exception:
                pass
        self._writer = writer
        if self.on_connect:
            await self.on_connect(True)
        try:
            while True:
                line = await reader.readline()
                if not line:
                    break
                try:
                    msg = json.loads(line.decode().strip())
                except (json.JSONDecodeError, UnicodeDecodeError):
                    log.warning("malformed line from Jetson: %r", line)
                    continue
                if self.on_message:
                    try:
                        await self.on_message(msg)
                    except Exception:
                        log.exception("on_message handler failed")
        finally:
            log.info("Jetson disconnected (%s)", peer)
            if self._writer is writer:
                self._writer = None
                if self.on_connect:
                    await self.on_connect(False)
            try:
                writer.close()
                await writer.wait_closed()
            except Exception:
                pass

    async def send(self, msg: dict) -> bool:
        if not self.connected or self._writer is None:
            return False
        data = (json.dumps(msg) + "\n").encode()
        try:
            self._writer.write(data)
            await self._writer.drain()
            return True
        except Exception as e:
            log.warning("send failed: %s", e)
            return False

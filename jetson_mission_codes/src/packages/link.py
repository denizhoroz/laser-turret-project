"""Transport link to ground station.

TCP client to GS (line-delimited JSON over the Jetson Nano USB-gadget
network: jetson 192.168.55.1, laptop 192.168.55.100).
"""
from __future__ import annotations

import json
import socket
import threading
import time
from queue import Queue, Empty


class Link:
    def __init__(self, host: str = "127.0.0.1", port: int = 9001):
        self.host = host
        self.port = port
        self.sock: socket.socket | None = None
        self.rx_queue: Queue = Queue()
        self._tx_lock = threading.Lock()
        self._stop = False
        # True between successful connect and reader exit / send failure.
        # Supervisor loop polls this to decide whether to recycle the session.
        self.connected: bool = False

    def connect(self, retry: bool = True) -> None:
        while not self._stop:
            try:
                self.sock = socket.create_connection((self.host, self.port), timeout=5)
                self.sock.settimeout(None)
                break
            except OSError as e:
                if not retry:
                    raise
                print(f"[link] connect failed ({e}), retrying in 2s...")
                time.sleep(2)
        self.connected = True
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self) -> None:
        buf = b""
        while not self._stop and self.sock:
            try:
                chunk = self.sock.recv(4096)
            except OSError:
                break
            if not chunk:
                break
            buf += chunk
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                line = line.strip()
                if not line:
                    continue
                try:
                    self.rx_queue.put(json.loads(line.decode()))
                except (json.JSONDecodeError, UnicodeDecodeError):
                    continue
        self.connected = False
        print("[link] reader stopped (disconnected)")

    def send(self, msg: dict) -> None:
        if self.sock is None or not self.connected:
            return
        data = (json.dumps(msg) + "\n").encode()
        with self._tx_lock:
            try:
                self.sock.sendall(data)
            except OSError as e:
                print(f"[link] send failed: {e}")
                self.connected = False

    def recv(self, timeout: float | None = None) -> dict | None:
        try:
            return self.rx_queue.get(timeout=timeout)
        except Empty:
            return None

    def close(self) -> None:
        self._stop = True
        self.connected = False
        if self.sock:
            try:
                self.sock.close()
            except OSError:
                pass
        self.sock = None

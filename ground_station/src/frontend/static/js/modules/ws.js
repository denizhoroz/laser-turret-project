// WebSocket client with exponential backoff reconnect.
// Dispatches typed messages to handlers passed in.

import { logLine } from './log.js';
import { setConnected } from './connection.js';
import { renderTelemetry } from './telemetry.js';
import { setDetections } from './overlay.js';
import { setStarted } from './mission.js';
import { els } from './dom.js';

const WS_URL = `ws://${location.host}/ws/telemetry`;
const BACKOFF_MIN = 500;
const BACKOFF_MAX = 8000;

let ws = null;
let backoff = BACKOFF_MIN;
let lastFrameURL = null;

function paintFrame(b64) {
  // base64 -> Uint8Array -> Blob -> object URL. Cheaper than data: URLs and frees old refs.
  const bin = atob(b64);
  const bytes = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
  const url = URL.createObjectURL(new Blob([bytes], { type: 'image/jpeg' }));
  if (lastFrameURL) URL.revokeObjectURL(lastFrameURL);
  lastFrameURL = url;
  els.video.src = url;
  els.videoFallback.classList.add('hidden');
  els.videoFallback.classList.remove('flex');
}

function handleMessage(msg) {
  switch (msg.type) {
    case 'connection':
      setConnected(!!msg.connected);
      logLine(msg.connected ? 'ok' : 'warn', `Jetson ${msg.connected ? 'connected' : 'disconnected'}`);
      if (!msg.connected) setStarted(false);
      break;
    case 'telemetry':
      renderTelemetry(msg.data || msg);
      break;
    case 'detections':
      setDetections(msg.data || [], msg.frame_w, msg.frame_h);
      break;
    case 'frame':
      if (msg.jpeg) paintFrame(msg.jpeg);
      setDetections(msg.boxes || [], msg.w, msg.h);
      break;
    case 'log':
      logLine('jetson', msg.text || '');
      break;
    default:
      if (msg.telemetry)  renderTelemetry(msg.telemetry);
      if (msg.detections) setDetections(msg.detections);
  }
}

function scheduleReconnect() {
  setTimeout(connect, backoff);
  backoff = Math.min(BACKOFF_MAX, backoff * 2);
}

function connect() {
  logLine('info', `WS connecting → ${WS_URL}`);
  try {
    ws = new WebSocket(WS_URL);
  } catch (e) {
    logLine('err', `WS construct failed: ${e.message}`);
    scheduleReconnect();
    return;
  }

  ws.addEventListener('open', () => {
    backoff = BACKOFF_MIN;
    logLine('ok', 'WS open');
  });

  ws.addEventListener('message', (ev) => {
    let msg;
    try { msg = JSON.parse(ev.data); }
    catch { logLine('warn', `WS non-JSON: ${ev.data}`); return; }
    handleMessage(msg);
  });

  ws.addEventListener('close', () => {
    logLine('warn', 'WS closed');
    setConnected(false);
    setStarted(false);
    scheduleReconnect();
  });

  ws.addEventListener('error', () => {
    logLine('err', 'WS error');
    try { ws.close(); } catch {}
  });
}

export function initWS() {
  connect();
}

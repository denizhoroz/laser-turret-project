// Mission start/stop button handlers. POST to /api/mission/*.
// Start/Stop are gated on Jetson connection state — disabled when link is down.

import { els } from './dom.js';
import { logLine } from './log.js';

let connected = false;
let started   = false;

async function postJSON(url, body) {
  const res = await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: body ? JSON.stringify(body) : undefined,
  });
  if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
  return res.json().catch(() => ({}));
}

function selectedMission() {
  const checked = document.querySelector('input[name="mission"]:checked');
  return checked ? Number(checked.value) : 1;
}

function refreshButtons() {
  // Start: disabled while running OR while link is down.
  const startDisabled = started || !connected;
  els.btnStart.disabled = startDisabled;
  els.btnStart.classList.toggle('opacity-50', startDisabled);
  els.btnStart.classList.toggle('cursor-not-allowed', startDisabled);
  const label = els.btnStart.querySelector('span');
  if (label) label.textContent = started ? 'Started' : 'Start';

  // Stop: useful only when link is up. Otherwise greyed.
  const stopDisabled = !connected;
  els.btnStop.disabled = stopDisabled;
  els.btnStop.classList.toggle('opacity-50', stopDisabled);
  els.btnStop.classList.toggle('cursor-not-allowed', stopDisabled);
}

export function setStarted(s) {
  started = s;
  refreshButtons();
}

// Called by ws.js whenever the Jetson connection state changes. Disconnect
// implicitly halts whatever mission was running, so we also clear `started`.
export function setMissionConnection(c) {
  connected = c;
  if (!c) started = false;
  refreshButtons();
}

export function initMission() {
  refreshButtons();   // initial: disconnected, not started → both disabled

  els.btnStart.addEventListener('click', async () => {
    if (!connected) {
      logLine('warn', 'Start ignored — Jetson disconnected');
      return;
    }
    const mission = selectedMission();
    logLine('info', `Mission ${mission} → start`);
    setStarted(true);
    try {
      const res = await postJSON('/api/mission/start', { mission_type: mission });
      if (res && res.ok === false) {
        logLine('err', `Start refused: ${res.error || 'unknown'}`);
        setStarted(false);
      } else {
        logLine('ok', `Mission ${mission} dispatched`);
      }
    } catch (e) {
      logLine('err', `Start failed: ${e.message}`);
      setStarted(false);
    }
  });

  els.btnStop.addEventListener('click', async () => {
    if (!connected) {
      logLine('warn', 'Stop ignored — Jetson disconnected');
      return;
    }
    logLine('info', 'Mission → stop');
    els.btnStop.disabled = true;
    try {
      await postJSON('/api/mission/stop');
      logLine('ok', 'Stop dispatched');
    } catch (e) {
      logLine('err', `Stop failed: ${e.message}`);
    } finally {
      setStarted(false);
    }
  });
}

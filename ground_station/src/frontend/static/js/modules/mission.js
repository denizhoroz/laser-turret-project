// Mission start/stop button handlers. POST to /api/mission/*.

import { els } from './dom.js';
import { logLine } from './log.js';

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

async function withBusy(btn, fn) {
  btn.disabled = true;
  try { await fn(); }
  finally { btn.disabled = false; }
}

export function initMission() {
  els.btnStart.addEventListener('click', () => withBusy(els.btnStart, async () => {
    const mission = selectedMission();
    logLine('info', `Mission ${mission} → start`);
    try {
      await postJSON('/api/mission/start', { mission_type: mission });
      logLine('ok', `Mission ${mission} dispatched`);
    } catch (e) {
      logLine('err', `Start failed: ${e.message}`);
    }
  }));

  els.btnStop.addEventListener('click', () => withBusy(els.btnStop, async () => {
    logLine('info', 'Mission → stop');
    try {
      await postJSON('/api/mission/stop');
      logLine('ok', 'Stop dispatched');
    } catch (e) {
      logLine('err', `Stop failed: ${e.message}`);
    }
  }));
}

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

export function setStarted(started) {
  const startBtn = els.btnStart;
  const label = startBtn.querySelector('span');
  startBtn.disabled = started;
  startBtn.classList.toggle('opacity-50', started);
  startBtn.classList.toggle('cursor-not-allowed', started);
  if (label) label.textContent = started ? 'Started' : 'Start';
}

export function initMission() {
  els.btnStart.addEventListener('click', async () => {
    const mission = selectedMission();
    logLine('info', `Mission ${mission} → start`);
    setStarted(true);
    try {
      await postJSON('/api/mission/start', { mission_type: mission });
      logLine('ok', `Mission ${mission} dispatched`);
    } catch (e) {
      logLine('err', `Start failed: ${e.message}`);
      setStarted(false); // re-enable on failure
    }
  });

  els.btnStop.addEventListener('click', async () => {
    logLine('info', 'Mission → stop');
    els.btnStop.disabled = true;
    try {
      await postJSON('/api/mission/stop');
      logLine('ok', 'Stop dispatched');
    } catch (e) {
      logLine('err', `Stop failed: ${e.message}`);
    } finally {
      els.btnStop.disabled = false;
      setStarted(false);
    }
  });
}

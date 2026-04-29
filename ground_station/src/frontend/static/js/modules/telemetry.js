// Telemetry panel renderer: limit switches, temp, motor dirs, laser, status.

import { els } from './dom.js';
import { setStatus } from './connection.js';

const dirArrow = (d) =>
  d === 1 ? '↑ +1' :
  d === -1 ? '↓ −1' :
  d === 0 ? '· 0' : '—';

export function renderTelemetry(t) {
  if (!t) return;

  if (Array.isArray(t.limit_switches)) {
    els.limitSwitches.forEach((node, i) => {
      const led = node.querySelector('[data-led]');
      const active = !!t.limit_switches[i];
      led.className = `w-2 h-2 mt-1 ${active ? 'bg-lime-400' : 'bg-zinc-700'}`;
    });
  }

  if (t.temp != null) els.temp.textContent = `${t.temp} °C`;
  if (t.yaw_dir   !== undefined) els.yaw.textContent   = dirArrow(t.yaw_dir);
  if (t.pitch_dir !== undefined) els.pitch.textContent = dirArrow(t.pitch_dir);

  if (t.laser !== undefined) {
    const on = !!t.laser;
    els.laserDot.className = `w-2 h-2 ${on ? 'bg-lime-400' : 'bg-zinc-700'}`;
    els.laserText.textContent = on ? 'Active' : 'Inactive';
  }

  if (t.status) setStatus(t.status);
}

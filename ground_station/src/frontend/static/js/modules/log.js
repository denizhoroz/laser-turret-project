// Log console: append timestamped lines, cap buffer, clear button.

import { els } from './dom.js';

const MAX_LINES = 200;
const COLOR = {
  info:   'text-zinc-400',
  warn:   'text-zinc-300',
  err:    'text-zinc-100',
  ok:     'text-lime-400',
  jetson: 'text-lime-300',
};

export function logLine(level, msg) {
  const li = document.createElement('li');
  const ts = new Date().toLocaleTimeString();
  li.className = COLOR[level] || COLOR.info;
  li.textContent = `[${ts}] ${msg}`;
  els.log.appendChild(li);
  while (els.log.children.length > MAX_LINES) els.log.firstChild.remove();
  els.log.scrollTop = els.log.scrollHeight;
}

export function initLog() {
  els.btnClearLog.addEventListener('click', (e) => {
    e.preventDefault();
    els.log.innerHTML = '';
  });
}

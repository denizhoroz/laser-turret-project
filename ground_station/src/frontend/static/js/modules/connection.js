// Connection pill + status pill rendering.

import { els } from './dom.js';

const STATUS_STYLE = {
  Online:    'border-zinc-700 bg-zinc-900 text-zinc-200',
  Scanning:  'border-lime-400/30 bg-lime-400/5 text-lime-300',
  Targeting: 'border-lime-400/50 bg-lime-400/10 text-lime-300',
  Firing:    'border-lime-400 bg-lime-400/20 text-lime-200',
  Offline:   'border-zinc-800 bg-zinc-950 text-zinc-500',
};

export function setConnected(connected) {
  if (connected) {
    els.connDot.className = 'w-2 h-2 bg-lime-400';
    els.connText.textContent = 'Connected';
    els.connPill.className = 'inline-flex items-center gap-2 px-3 py-1 text-xs font-medium border border-lime-400/40 bg-lime-400/10 text-lime-300';
  } else {
    els.connDot.className = 'w-2 h-2 bg-zinc-600';
    els.connText.textContent = 'Disconnected';
    els.connPill.className = 'inline-flex items-center gap-2 px-3 py-1 text-xs font-medium border border-zinc-800 bg-zinc-950 text-zinc-500';
    setStatus('Offline');
  }
}

export function setStatus(s) {
  els.statusText.textContent = s || '—';
  const cls = STATUS_STYLE[s] || 'border-zinc-800 bg-zinc-950 text-zinc-400';
  els.statusPill.className = `inline-flex items-center px-3 py-1 text-xs font-medium border ${cls}`;
}

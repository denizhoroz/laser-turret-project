// Bbox canvas overlay synced with video <img>. DPR-aware, resizes with viewport.

import { els } from './dom.js';

const STROKE = '#a3e635';      // lime-400
const FILL   = 'rgba(163, 230, 53, 0.9)';
const TEXT   = '#000000';

let detections = [];
let sourceW = 640;
let sourceH = 480;

export function setDetections(list, frameW, frameH) {
  detections = list || [];
  if (frameW) sourceW = frameW;
  if (frameH) sourceH = frameH;
  draw();
}

function computeImageRect() {
  // Mirrors object-contain: image is centered, scaled to fit while preserving aspect.
  const cw = els.overlay.width;
  const ch = els.overlay.height;
  const srcAspect = sourceW / sourceH;
  const dstAspect = cw / ch;
  let iw, ih, ox, oy;
  if (dstAspect > srcAspect) {
    ih = ch;
    iw = ch * srcAspect;
    ox = (cw - iw) / 2;
    oy = 0;
  } else {
    iw = cw;
    ih = cw / srcAspect;
    ox = 0;
    oy = (ch - ih) / 2;
  }
  return { ox, oy, iw, ih };
}

function draw() {
  const ctx = els.overlay.getContext('2d');
  ctx.clearRect(0, 0, els.overlay.width, els.overlay.height);
  if (!detections.length) return;

  const dpr = window.devicePixelRatio || 1;
  const { ox, oy, iw, ih } = computeImageRect();
  const sx = iw / sourceW;
  const sy = ih / sourceH;
  ctx.lineWidth = Math.max(1, 2 * dpr);
  ctx.font = `${12 * dpr}px ui-monospace, monospace`;
  ctx.textBaseline = 'top';

  for (const d of detections) {
    if (!d.bbox || d.bbox.length < 4) continue;
    const [x, y, w, h] = d.bbox;
    const X = ox + x * sx, Y = oy + y * sy, W = w * sx, H = h * sy;

    ctx.strokeStyle = STROKE;
    ctx.strokeRect(X, Y, W, H);

    const tag = d.name ?? (d.cls != null ? String(d.cls) : '');
    const label = d.conf != null ? `${tag} ${(d.conf * 100).toFixed(0)}%` : tag;
    if (label.trim()) {
      const pad = 4 * dpr;
      const tw = ctx.measureText(label).width + pad * 2;
      const th = 16 * dpr;
      ctx.fillStyle = FILL;
      ctx.fillRect(X, Y - th, tw, th);
      ctx.fillStyle = TEXT;
      ctx.fillText(label, X + pad, Y - th + pad / 2);
    }
  }
}

function resize() {
  const r = els.video.getBoundingClientRect();
  const dpr = window.devicePixelRatio || 1;
  els.overlay.width  = Math.max(1, Math.round(r.width * dpr));
  els.overlay.height = Math.max(1, Math.round(r.height * dpr));
  els.overlay.style.width  = r.width + 'px';
  els.overlay.style.height = r.height + 'px';
  draw();
}

export function initOverlay() {
  window.addEventListener('resize', resize);

  els.video.addEventListener('error', () => {
    els.videoFallback.classList.remove('hidden');
    els.videoFallback.classList.add('flex');
  });

  els.video.addEventListener('load', () => {
    els.videoFallback.classList.add('hidden');
    els.videoFallback.classList.remove('flex');
    if (els.video.naturalWidth)  sourceW = els.video.naturalWidth;
    if (els.video.naturalHeight) sourceH = els.video.naturalHeight;
    resize();
  });

  resize();
}

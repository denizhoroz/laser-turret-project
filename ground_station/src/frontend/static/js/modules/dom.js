// Centralized DOM element references. Single source of truth for selectors.

export const els = {
  connDot:        document.getElementById('conn-dot'),
  connText:       document.getElementById('conn-text'),
  connPill:       document.getElementById('conn-pill'),
  statusText:     document.getElementById('status-text'),
  statusPill:     document.getElementById('status-pill'),
  video:          document.getElementById('video'),
  videoFallback:  document.getElementById('video-fallback'),
  overlay:        document.getElementById('overlay'),
  limitSwitches:  document.querySelectorAll('#limit-switches [data-sw]'),
  temp:           document.getElementById('t-temp'),
  yaw:            document.getElementById('t-yaw'),
  pitch:          document.getElementById('t-pitch'),
  laserDot:       document.getElementById('t-laser-dot'),
  laserText:      document.getElementById('t-laser-text'),
  state:          document.getElementById('t-state'),
  btnStart:       document.getElementById('btn-start'),
  btnStop:        document.getElementById('btn-stop'),
  btnClearLog:    document.getElementById('btn-clear-log'),
  log:            document.getElementById('log'),
};

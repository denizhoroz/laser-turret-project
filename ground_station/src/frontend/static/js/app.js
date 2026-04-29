// Entry point: wire modules, set initial state, open WS.

import { initLog } from './modules/log.js';
import { setConnected, setStatus } from './modules/connection.js';
import { initOverlay } from './modules/overlay.js';
import { initMission } from './modules/mission.js';
import { initWS } from './modules/ws.js';

initLog();
setConnected(false);
setStatus('Offline');
initOverlay();
initMission();
initWS();

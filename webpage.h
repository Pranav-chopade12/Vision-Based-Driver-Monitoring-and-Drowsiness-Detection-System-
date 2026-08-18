/*
 *  webpage.h  —  Dashboard HTML stored in ESP32 flash (PROGMEM)
 *  DriverGuard Pro v1.0
 *
 *  NOTE: Do NOT add )rawliteral" anywhere inside the HTML string.
 */

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>DriverGuard Pro — Real-Time Monitoring</title>
<meta name="description" content="Professional automotive driver monitoring dashboard powered by ESP32">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;500;700;900&family=Rajdhani:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>
*,*::before,*::after{margin:0;padding:0;box-sizing:border-box}
:root{
  --bg:#04060e;
  --card:rgba(255,255,255,.028);
  --card-h:rgba(255,255,255,.055);
  --border:rgba(0,212,255,.1);
  --border-h:rgba(0,212,255,.32);
  --cyan:#00d4ff;
  --green:#00ff88;
  --amber:#ffaa00;
  --red:#ff3355;
  --txt:#dde8ff;
  --muted:rgba(221,232,255,.42);
  --glow-c:0 0 28px rgba(0,212,255,.3);
  --glow-g:0 0 22px rgba(0,255,136,.45);
  --glow-a:0 0 22px rgba(255,170,0,.45);
  --glow-r:0 0 22px rgba(255,51,85,.45);
}
html,body{height:100%}
body{
  background:var(--bg);color:var(--txt);font-family:'Rajdhani',sans-serif;
  min-height:100vh;overflow-x:hidden;
  background-image:
    radial-gradient(ellipse at 15% 12%,rgba(0,212,255,.06) 0,transparent 52%),
    radial-gradient(ellipse at 85% 88%,rgba(0,60,200,.06) 0,transparent 52%);
}
.grid-bg{
  position:fixed;inset:0;pointer-events:none;z-index:0;
  background-image:
    linear-gradient(rgba(0,212,255,.023) 1px,transparent 1px),
    linear-gradient(90deg,rgba(0,212,255,.023) 1px,transparent 1px);
  background-size:60px 60px;
}
.scan{
  position:fixed;inset:0;pointer-events:none;z-index:0;
  background:repeating-linear-gradient(0deg,transparent,transparent 2px,rgba(0,0,0,.055) 2px,rgba(0,0,0,.055) 4px);
}
.alert-overlay{
  position:fixed;inset:0;pointer-events:none;z-index:99;
  opacity:0;transition:opacity .3s;border:3px solid transparent;
}
.alert-overlay.on{opacity:1;border-color:rgba(255,51,85,.45);animation:brd 1s infinite}
@keyframes brd{0%,100%{border-color:rgba(255,51,85,.3)}50%{border-color:rgba(255,51,85,.85)}}
.wrap{position:relative;z-index:1;max-width:1440px;margin:0 auto;padding:14px 20px}

/* ── HEADER ── */
.hdr{
  display:flex;justify-content:space-between;align-items:center;
  background:rgba(0,212,255,.035);border:1px solid var(--border);
  border-radius:16px;padding:13px 22px;margin-bottom:16px;
  backdrop-filter:blur(12px);
}
.hdr-logo{display:flex;align-items:center;gap:14px}
.hdr-icon{
  width:44px;height:44px;border-radius:12px;display:flex;align-items:center;
  justify-content:center;font-size:1.4rem;flex-shrink:0;
  background:linear-gradient(135deg,var(--cyan),#0055ff);box-shadow:var(--glow-c);
}
.hdr-title{font-family:'Orbitron',sans-serif;font-size:1.25rem;font-weight:900;letter-spacing:3px;color:var(--cyan);text-shadow:var(--glow-c)}
.hdr-title span{color:#fff}
.hdr-sub{font-family:'Orbitron',sans-serif;font-size:.52rem;letter-spacing:4px;color:var(--muted);margin-top:2px}
.hdr-mid{display:flex;gap:28px}
.hs{text-align:center}
.hs-l{font-family:'Orbitron',sans-serif;font-size:.48rem;letter-spacing:2px;color:var(--muted)}
.hs-v{font-family:'Orbitron',sans-serif;font-size:.82rem;color:var(--cyan)}
.conn{display:flex;align-items:center;gap:10px;font-family:'Orbitron',sans-serif;font-size:.6rem;letter-spacing:2px}
.dot{width:12px;height:12px;border-radius:50%;background:var(--red);box-shadow:0 0 12px var(--red);transition:all .5s}
.dot.on{background:var(--green);box-shadow:0 0 12px var(--green);animation:breathe 2s infinite}
@keyframes breathe{0%,100%{transform:scale(1);opacity:1}50%{transform:scale(1.3);opacity:.55}}

/* ── MAIN 3-COL ── */
.main-grid{display:grid;grid-template-columns:255px 1fr 255px;gap:14px;margin-bottom:14px}

/* ── SAFETY GAUGE ── */
.safety-p{
  background:var(--card);border:1px solid var(--border);border-radius:20px;
  padding:22px 16px;display:flex;flex-direction:column;align-items:center;gap:13px;
}
.p-lbl{font-family:'Orbitron',sans-serif;font-size:.56rem;letter-spacing:3px;color:var(--muted)}
.g-wrap{position:relative;width:148px;height:148px}
.g-svg{width:100%;height:100%;transform:rotate(-90deg)}
.g-bg{fill:none;stroke:rgba(255,255,255,.07);stroke-width:9}
.g-fill{fill:none;stroke-width:9;stroke-linecap:round;stroke-dasharray:408;stroke-dashoffset:0;transition:stroke-dashoffset 1.2s ease,stroke .5s}
.g-in{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);text-align:center}
.g-num{font-family:'Orbitron',sans-serif;font-size:2.2rem;font-weight:900;line-height:1;transition:color .5s}
.g-unit{font-family:'Orbitron',sans-serif;font-size:.52rem;letter-spacing:2px;color:var(--muted)}
.g-grade{font-family:'Orbitron',sans-serif;font-size:.88rem;font-weight:700;letter-spacing:3px;transition:color .5s}

/* ── TIME ── */
.time-p{
  background:var(--card);border:1px solid var(--border);border-radius:20px;
  padding:28px;display:flex;flex-direction:column;align-items:center;
  justify-content:center;position:relative;overflow:hidden;min-height:258px;
}
.time-p::before{
  content:'';position:absolute;top:-50%;left:-50%;width:200%;height:200%;
  background:radial-gradient(ellipse at center,rgba(0,212,255,.045) 0,transparent 62%);
  animation:rot 22s linear infinite;pointer-events:none;
}
@keyframes rot{from{transform:rotate(0)}to{transform:rotate(360deg)}}
.t-time{
  font-family:'Orbitron',sans-serif;font-size:4.5rem;font-weight:900;
  color:var(--cyan);text-shadow:0 0 40px rgba(0,212,255,.5);
  letter-spacing:5px;position:relative;z-index:1;
}
.t-colon{animation:blink 1s infinite}
@keyframes blink{0%,49%{opacity:1}50%,100%{opacity:.12}}
.t-date{font-family:'Orbitron',sans-serif;font-size:.95rem;color:var(--muted);letter-spacing:4px;margin-top:9px;position:relative;z-index:1}
.t-day{font-family:'Orbitron',sans-serif;font-size:.58rem;color:rgba(0,212,255,.5);letter-spacing:6px;margin-top:6px;position:relative;z-index:1}

/* ── STATE PANEL ── */
.state-p{
  background:var(--card);border:1px solid var(--border);border-radius:20px;
  padding:22px 16px;display:flex;flex-direction:column;align-items:center;gap:11px;
  transition:border-color .5s,background .5s;
}
.state-p.s-w{border-color:rgba(0,255,136,.25);background:rgba(0,255,136,.025)}
.state-p.s-d{border-color:rgba(255,170,0,.25);background:rgba(255,170,0,.022)}
.state-p.s-a{border-color:rgba(255,51,85,.25);background:rgba(255,51,85,.025)}
.s-ring{position:relative;width:148px;height:148px}
.s-svg{width:100%;height:100%;transform:rotate(-90deg)}
.s-track{fill:none;stroke:rgba(255,255,255,.07);stroke-width:6}
.s-prog{fill:none;stroke-width:6;stroke-linecap:round;stroke-dasharray:408;stroke-dashoffset:200;transition:stroke .5s,filter .5s}
.s-w .s-prog{stroke:var(--green);filter:drop-shadow(0 0 7px var(--green))}
.s-d .s-prog{stroke:var(--amber);filter:drop-shadow(0 0 7px var(--amber))}
.s-a .s-prog{stroke:var(--red);filter:drop-shadow(0 0 7px var(--red))}
.s-in{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);text-align:center}
.s-letter{font-family:'Orbitron',sans-serif;font-size:3.1rem;font-weight:900;line-height:1;transition:color .5s,text-shadow .5s}
.s-w .s-letter{color:var(--green);text-shadow:var(--glow-g)}
.s-d .s-letter{color:var(--amber);text-shadow:var(--glow-a)}
.s-a .s-letter{color:var(--red);text-shadow:var(--glow-r);animation:apulse .8s infinite}
@keyframes apulse{0%,100%{transform:scale(1);opacity:1}50%{transform:scale(1.1);opacity:.65}}
.s-code{font-family:'Orbitron',sans-serif;font-size:.48rem;letter-spacing:2px;color:var(--muted);margin-top:4px}
.s-name{font-family:'Orbitron',sans-serif;font-size:.68rem;letter-spacing:2px;transition:color .5s}
.s-w .s-name{color:var(--green)}
.s-d .s-name{color:var(--amber)}
.s-a .s-name{color:var(--red)}

/* ── INFO ROW (4 cards) ── */
.info-row{display:grid;grid-template-columns:repeat(4,1fr);gap:13px;margin-bottom:13px}
.ic{
  background:var(--card);border:1px solid var(--border);border-radius:15px;
  padding:17px;position:relative;overflow:hidden;transition:.28s;cursor:default;
}
.ic::after{content:'';position:absolute;top:0;left:0;right:0;height:2px;background:var(--ic-top,linear-gradient(90deg,var(--cyan),transparent))}
.ic:hover{transform:translateY(-3px);border-color:var(--border-h);background:var(--card-h)}
.ic-l{font-family:'Orbitron',sans-serif;font-size:.52rem;letter-spacing:3px;color:var(--muted);margin-bottom:8px}
.ic-v{font-family:'Orbitron',sans-serif;font-size:1.45rem;font-weight:700;transition:color .4s}
.ic-s{font-size:.8rem;color:var(--muted);margin-top:4px}

/* ── COUNT CARDS ── */
.counts{display:grid;grid-template-columns:repeat(3,1fr);gap:13px;margin-bottom:13px}
.cc{
  background:var(--card);border:1px solid var(--border);border-radius:15px;
  padding:20px;display:flex;align-items:center;gap:16px;
  position:relative;overflow:hidden;transition:.28s;
}
.cc:hover{transform:translateY(-3px)}
.cc:hover.cc-w{border-color:rgba(0,255,136,.32)}
.cc:hover.cc-d{border-color:rgba(255,170,0,.32)}
.cc:hover.cc-a{border-color:rgba(255,51,85,.32)}
.cc-icon{
  width:54px;height:54px;border-radius:13px;display:flex;
  align-items:center;justify-content:center;font-size:1.45rem;flex-shrink:0;
}
.cc-w .cc-icon{background:rgba(0,255,136,.1);border:1px solid rgba(0,255,136,.2)}
.cc-d .cc-icon{background:rgba(255,170,0,.1);border:1px solid rgba(255,170,0,.2)}
.cc-a .cc-icon{background:rgba(255,51,85,.1);border:1px solid rgba(255,51,85,.2)}
.cc-num{font-family:'Orbitron',sans-serif;font-size:2.45rem;font-weight:900;line-height:1}
.cc-w .cc-num{color:var(--green)}
.cc-d .cc-num{color:var(--amber)}
.cc-a .cc-num{color:var(--red)}
.cc-lbl{font-family:'Orbitron',sans-serif;font-size:.56rem;letter-spacing:2px;color:var(--muted);margin-top:5px}
.cc-bar{position:absolute;bottom:0;left:0;height:3px;width:0;transition:width 1.1s ease}
.cc-w .cc-bar{background:var(--green);box-shadow:0 0 8px var(--green)}
.cc-d .cc-bar{background:var(--amber);box-shadow:0 0 8px var(--amber)}
.cc-a .cc-bar{background:var(--red);box-shadow:0 0 8px var(--red)}
@keyframes npop{0%{transform:scale(1)}50%{transform:scale(1.22)}100%{transform:scale(1)}}
.pop{animation:npop .35s ease}

/* ── LOG ── */
.log-p{background:var(--card);border:1px solid var(--border);border-radius:15px;padding:17px 20px;margin-bottom:13px}
.log-hdr{display:flex;justify-content:space-between;align-items:center;margin-bottom:13px;padding-bottom:11px;border-bottom:1px solid var(--border)}
.log-title{font-family:'Orbitron',sans-serif;font-size:.6rem;letter-spacing:4px;color:var(--cyan)}
.log-badge{font-family:'Orbitron',sans-serif;font-size:.54rem;letter-spacing:2px;color:var(--muted);background:rgba(0,212,255,.08);padding:3px 10px;border-radius:20px;border:1px solid rgba(0,212,255,.15)}
.log-list{max-height:195px;overflow-y:auto;scrollbar-width:thin;scrollbar-color:rgba(0,212,255,.3) transparent}
.log-list::-webkit-scrollbar{width:3px}
.log-list::-webkit-scrollbar-thumb{background:rgba(0,212,255,.3);border-radius:2px}
.log-row{
  display:grid;grid-template-columns:100px 14px 1fr 92px 78px;
  gap:12px;align-items:center;padding:9px 0;
  border-bottom:1px solid rgba(255,255,255,.045);
  animation:slin .38s ease;
}
.log-row:last-child{border-bottom:none}
@keyframes slin{from{opacity:0;transform:translateX(-18px)}to{opacity:1;transform:translateX(0)}}
.l-time{font-family:'Orbitron',monospace;font-size:.65rem;color:var(--muted)}
.l-dot{width:10px;height:10px;border-radius:50%}
.l-dot-W{background:var(--green);box-shadow:0 0 6px var(--green)}
.l-dot-D{background:var(--amber);box-shadow:0 0 6px var(--amber)}
.l-dot-A{background:var(--red);box-shadow:0 0 6px var(--red)}
.l-name{font-weight:600;font-size:.92rem}
.l-nW{color:var(--green)}.l-nD{color:var(--amber)}.l-nA{color:var(--red)}
.l-tag{font-family:'Orbitron',sans-serif;font-size:.5rem;letter-spacing:1px;padding:3px 8px;border-radius:20px;text-align:center}
.l-tW{background:rgba(0,255,136,.1);color:var(--green);border:1px solid rgba(0,255,136,.22)}
.l-tD{background:rgba(255,170,0,.1);color:var(--amber);border:1px solid rgba(255,170,0,.22)}
.l-tA{background:rgba(255,51,85,.1);color:var(--red);border:1px solid rgba(255,51,85,.22)}
.l-evt{font-family:'Orbitron',sans-serif;font-size:.62rem;color:var(--muted);text-align:right}
.log-empty{text-align:center;color:var(--muted);padding:28px;font-family:'Orbitron',sans-serif;font-size:.62rem;letter-spacing:3px}

/* ── FOOTER ── */
.ftr{display:flex;justify-content:space-between;align-items:center;padding:11px 0;border-top:1px solid rgba(0,212,255,.1);font-family:'Orbitron',sans-serif;font-size:.5rem;letter-spacing:2px;color:var(--muted)}

/* ── RESPONSIVE ── */
@media(max-width:1100px){
  .main-grid{grid-template-columns:1fr 1fr}
  .safety-p{grid-column:1/-1;flex-direction:row;justify-content:space-around}
  .info-row{grid-template-columns:repeat(2,1fr)}
}
@media(max-width:680px){
  .main-grid{grid-template-columns:1fr}
  .t-time{font-size:3rem}
  .info-row,.counts{grid-template-columns:1fr}
  .hdr-mid{display:none}
}
</style>
</head>
<body>
<div class="grid-bg"></div>
<div class="scan"></div>
<div class="alert-overlay" id="aOverlay"></div>
<div class="wrap">

<!-- HEADER -->
<header class="hdr" id="mainHdr">
  <div class="hdr-logo">
    <div class="hdr-icon">&#128737;</div>
    <div>
      <div class="hdr-title">DRIVER<span>GUARD</span></div>
      <div class="hdr-sub">REAL-TIME DRIVER MONITORING SYSTEM</div>
    </div>
  </div>
  <div class="hdr-mid">
    <div class="hs"><div class="hs-l">SESSION</div><div class="hs-v" id="hSes">00:00:00</div></div>
    <div class="hs"><div class="hs-l">DEVICE</div><div class="hs-v">ESP32</div></div>
    <div class="hs"><div class="hs-l">PROTOCOL</div><div class="hs-v">UART2</div></div>
    <div class="hs"><div class="hs-l">BAUD RATE</div><div class="hs-v">115200</div></div>
  </div>
  <div class="conn">
    <div class="dot" id="dot"></div>
    <span id="connTxt">CONNECTING</span>
  </div>
</header>

<!-- MAIN 3-COL -->
<div class="main-grid">

  <!-- Safety Gauge -->
  <div class="safety-p">
    <div class="p-lbl">SAFETY SCORE</div>
    <div class="g-wrap">
      <svg class="g-svg" viewBox="0 0 150 150">
        <circle class="g-bg" cx="75" cy="75" r="65"/>
        <circle class="g-fill" id="gCircle" cx="75" cy="75" r="65"/>
      </svg>
      <div class="g-in">
        <div class="g-num" id="gNum">100</div>
        <div class="g-unit">/ 100</div>
      </div>
    </div>
    <div class="g-grade" id="gGrade" style="color:var(--green)">EXCELLENT</div>
  </div>

  <!-- Clock -->
  <div class="time-p">
    <div class="t-time">
      <span id="tH">00</span><span class="t-colon">:</span><span id="tM">00</span><span class="t-colon">:</span><span id="tS">00</span>
    </div>
    <div class="t-date" id="tDate">-- --- ----</div>
    <div class="t-day" id="tDay">---------</div>
  </div>

  <!-- State Ring -->
  <div class="state-p s-w" id="stateP">
    <div class="p-lbl">DRIVER STATE</div>
    <div class="s-ring">
      <svg class="s-svg" viewBox="0 0 150 150">
        <circle class="s-track" cx="75" cy="75" r="65"/>
        <circle class="s-prog" id="sProg" cx="75" cy="75" r="65"/>
      </svg>
      <div class="s-in">
        <div class="s-letter" id="sLetter">W</div>
        <div class="s-code" id="sCode">UART2 READY</div>
      </div>
    </div>
    <div class="s-name" id="sName">&#9679; WIDE AWAKE</div>
  </div>
</div>

<!-- INFO ROW -->
<div class="info-row">
  <div class="ic">
    <div class="ic-l">ALERT TYPE</div>
    <div class="ic-v" id="iAlert" style="color:var(--green);font-size:1.1rem">WIDE AWAKE</div>
    <div class="ic-s">Driver Classification</div>
  </div>
  <div class="ic">
    <div class="ic-l">STATE DURATION</div>
    <div class="ic-v" id="iDur" style="color:var(--cyan)">00:00:00</div>
    <div class="ic-s">Active since last change</div>
  </div>
  <div class="ic">
    <div class="ic-l">TOTAL EVENTS</div>
    <div class="ic-v" id="iTotal" style="color:var(--cyan)">0</div>
    <div class="ic-s">State transitions logged</div>
  </div>
  <div class="ic" style="--ic-top:linear-gradient(90deg,var(--red),transparent)">
    <div class="ic-l">RISK LEVEL</div>
    <div class="ic-v" id="iRisk" style="color:var(--green);font-size:1.1rem">LOW</div>
    <div class="ic-s">Current risk assessment</div>
  </div>
</div>

<!-- COUNTS -->
<div class="counts">
  <div class="cc cc-w">
    <div class="cc-icon">&#129001;</div>
    <div>
      <div class="cc-num" id="cW">0</div>
      <div class="cc-lbl">WIDE AWAKE EVENTS</div>
    </div>
    <div class="cc-bar" id="bW"></div>
  </div>
  <div class="cc cc-d">
    <div class="cc-icon">&#128993;</div>
    <div>
      <div class="cc-num" id="cD">0</div>
      <div class="cc-lbl">DROWSY DETECTIONS</div>
    </div>
    <div class="cc-bar" id="bD"></div>
  </div>
  <div class="cc cc-a">
    <div class="cc-icon">&#128308;</div>
    <div>
      <div class="cc-num" id="cA">0</div>
      <div class="cc-lbl">CRITICAL ALERTS</div>
    </div>
    <div class="cc-bar" id="bA"></div>
  </div>
</div>

<!-- ACTIVITY LOG -->
<div class="log-p">
  <div class="log-hdr">
    <div class="log-title">&#11041; ACTIVITY LOG</div>
    <div class="log-badge" id="logBadge">0 EVENTS</div>
  </div>
  <div class="log-list" id="logList">
    <div class="log-empty">AWAITING DATA FROM ESP32...</div>
  </div>
</div>

<!-- FOOTER -->
<footer class="ftr">
  <span>DRIVERGUARD PRO v1.0 &mdash; AUTOMOTIVE MONITORING SYSTEM</span>
  <span id="fTime">--:--:--</span>
  <span>ESP32 WROOM32 &bull; UART2 &bull; 115200 BAUD</span>
</footer>

</div><!-- /wrap -->

<script>
'use strict';

/* ── Config ── */
const WS_PORT = 81;
const MAX_LOG = 50;
const DAYS    = ['SUNDAY','MONDAY','TUESDAY','WEDNESDAY','THURSDAY','FRIDAY','SATURDAY'];
const MONTHS  = ['JAN','FEB','MAR','APR','MAY','JUN','JUL','AUG','SEP','OCT','NOV','DEC'];

/* ── State ── */
let state   = 'W';
let wakeC   = 0, drowsyC = 0, alertC = 0;
let dur     = 0;
let logs    = [];
let sessT   = Date.now();
let ws      = null;
let recon   = 0;

/* ── Helpers ── */
const $  = id => document.getElementById(id);
const p2 = n  => String(n).padStart(2,'0');
const fmtTime = s => p2(Math.floor(s/3600))+':'+p2(Math.floor((s%3600)/60))+':'+p2(s%60);

/* ── WebSocket ── */
function connect() {
  const host = window.location.hostname || '192.168.1.100';
  try {
    ws = new WebSocket('ws://'+host+':'+WS_PORT);
    ws.onopen    = ()  => { setConn(true);  recon = 0; };
    ws.onmessage = e   => { try{ onData(JSON.parse(e.data)); }catch(_){} };
    ws.onclose   = ws.onerror = () => {
      setConn(false);
      recon++;
      setTimeout(connect, Math.min(recon*1000, 6000));
    };
  } catch(_){ setTimeout(connect, 3000); }
}

function setConn(on) {
  $('dot').className = 'dot'+(on?' on':'');
  $('connTxt').textContent = on ? 'CONNECTED' : 'RECONNECTING';
}

/* ── Data handler ── */
function onData(d) {
  const prev = state;
  if(d.state       !== undefined) state   = d.state;
  if(d.wakeCount   !== undefined) wakeC   = d.wakeCount;
  if(d.drowsyCount !== undefined) drowsyC = d.drowsyCount;
  if(d.alertCount  !== undefined) alertC  = d.alertCount;
  if(d.duration    !== undefined) dur     = d.duration;

  if(prev !== state){ addLog(state); animatePanel(); }
  drawState();
  drawCounts();
  drawSafety();
  $('iTotal').textContent = wakeC + drowsyC + alertC;
}

/* ── State Map ── */
const SM = {
  W:{ cls:'s-w', letter:'W', code:'WIDE AWAKE', name:'\u25CF WIDE AWAKE', alert:'WIDE AWAKE',     risk:'LOW',    rc:'var(--green)', ac:'var(--green)' },
  D:{ cls:'s-d', letter:'D', code:'DROWSY',     name:'\u26A0 WARNING',    alert:'DROWSY DETECTED', risk:'MEDIUM', rc:'var(--amber)', ac:'var(--amber)' },
  A:{ cls:'s-a', letter:'A', code:'ALERT',      name:'\uD83D\uDD34 CRITICAL', alert:'CRITICAL ALERT', risk:'HIGH', rc:'var(--red)', ac:'var(--red)' }
};

function drawState() {
  const m = SM[state] || SM.W;
  $('stateP').className = 'state-p '+m.cls;
  $('sLetter').textContent = m.letter;
  $('sCode').textContent   = m.code;
  $('sName').textContent   = m.name;
  $('iAlert').textContent  = m.alert;  $('iAlert').style.color = m.ac;
  $('iRisk').textContent   = m.risk;   $('iRisk').style.color  = m.rc;
  $('aOverlay').className  = 'alert-overlay'+(state==='A'?' on':'');
}

function animatePanel() {
  const p = $('stateP');
  p.style.transition = 'transform .24s ease,border-color .5s,background .5s';
  p.style.transform  = 'scale(.92)';
  setTimeout(()=>{ p.style.transform='scale(1)'; }, 250);
}

/* ── Counts ── */
function drawCounts() {
  setNum('cW', wakeC);
  setNum('cD', drowsyC);
  setNum('cA', alertC);
  const tot = wakeC + drowsyC + alertC;
  if(tot > 0) {
    $('bW').style.width = (wakeC/tot*100)+'%';
    $('bD').style.width = (drowsyC/tot*100)+'%';
    $('bA').style.width = (alertC/tot*100)+'%';
  }
}

function setNum(id, val) {
  const el = $(id);
  if(el.textContent != val) {
    el.textContent = val;
    el.classList.remove('pop');
    void el.offsetWidth;
    el.classList.add('pop');
  }
}

/* ── Safety Score ── */
function drawSafety() {
  const tot = wakeC + drowsyC + alertC;
  let sc = 100;
  if(tot > 0) sc = Math.max(0, Math.min(100, Math.round(((wakeC + drowsyC*0.4)/tot)*100)));

  const circ = 408; /* 2*PI*65 */
  $('gCircle').style.strokeDasharray  = circ;
  $('gCircle').style.strokeDashoffset = circ - (sc/100)*circ;
  $('gNum').textContent = sc;

  let col, grade;
  if     (sc >= 90){ col='var(--green)'; grade='EXCELLENT'; }
  else if(sc >= 70){ col='#88ff44';      grade='GOOD'; }
  else if(sc >= 50){ col='var(--amber)'; grade='FAIR'; }
  else              { col='var(--red)';   grade='POOR'; }

  $('gCircle').style.stroke = col;
  $('gNum').style.color     = col;
  $('gGrade').style.color   = col;
  $('gGrade').textContent   = grade;
}

/* ── Log ── */
const SN = { W:'WIDE AWAKE', D:'DROWSY DETECTED', A:'CRITICAL ALERT' };
const ST = { W:'NORMAL',     D:'WARNING',          A:'CRITICAL' };

function addLog(s) {
  const now = new Date();
  const t   = p2(now.getHours())+':'+p2(now.getMinutes())+':'+p2(now.getSeconds());
  logs.unshift({ t, s, name: SN[s]||s, tag: ST[s]||s });
  if(logs.length > MAX_LOG) logs.pop();
  drawLog();
}

function drawLog() {
  const el = $('logList');
  $('logBadge').textContent = logs.length+' EVENTS';
  if(!logs.length){
    el.innerHTML = '<div class="log-empty">AWAITING DATA FROM ESP32...</div>';
    return;
  }
  el.innerHTML = logs.map((e,i)=>`
    <div class="log-row">
      <span class="l-time">${e.t}</span>
      <div class="l-dot l-dot-${e.s}"></div>
      <span class="l-name l-n${e.s}">${e.name}</span>
      <span class="l-tag l-t${e.s}">${e.tag}</span>
      <span class="l-evt">EVT #${logs.length-i}</span>
    </div>`).join('');
}

/* ── Clock ── */
function tick() {
  const now = new Date();
  const h=p2(now.getHours()), m=p2(now.getMinutes()), s=p2(now.getSeconds());
  $('tH').textContent = h; $('tM').textContent = m; $('tS').textContent = s;
  $('fTime').textContent  = h+':'+m+':'+s;
  $('tDate').textContent  = p2(now.getDate())+' '+MONTHS[now.getMonth()]+' '+now.getFullYear();
  $('tDay').textContent   = DAYS[now.getDay()];
  $('hSes').textContent   = fmtTime(Math.floor((Date.now()-sessT)/1000));
  $('iDur').textContent   = fmtTime(dur);
}

/* ── Init ── */
tick();
setInterval(tick, 1000);
drawLog();
drawState();
drawSafety();
connect();
</script>
</body>
</html>
)rawliteral";

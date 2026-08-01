/* ==========================================================
   SMART AGRICULTURE IoT DASHBOARD — script.js
   ThingSpeak: field1 = suhu, field2 = kelembapan udara,
               field3 = kelembapan tanah  (field4/field5 TIDAK dipakai)
   ESP32 API  : /data, /pompa/on|off, /kipas/on|off,
                /pompa/manual|auto, /kipas/manual|auto
   ========================================================== */

const CHANNEL_ID = "3441844";
const READ_API_KEY = "OO4CB81Z1IA7Z50M";
const TS_API = `https://api.thingspeak.com/channels/${CHANNEL_ID}/feeds.json?api_key=${READ_API_KEY}&results=30`;

const TS_INTERVAL = 15000;   // ms, batas free ThingSpeak ~15s
const ESP_INTERVAL = 2000;   // ms

const DEVICES = {
  pump: { key: 'pompa' },
  fan:  { key: 'kipas' }
};

let espOnline = false;

// ============================ ELEMENTS ============================

const el = {
  temp: document.getElementById('temp'),
  hum: document.getElementById('hum'),
  soil: document.getElementById('soil'),
  tempStatus: document.getElementById('tempStatus'),
  humStatus: document.getElementById('humStatus'),
  soilStatus: document.getElementById('soilStatus'),
  lastUpdate: document.getElementById('lastUpdate'),
  clock: document.getElementById('clock'),
  tsStatus: document.getElementById('tsStatus'),
  espStatus: document.getElementById('espStatus'),
  pumpModeToggle: document.getElementById('pumpModeToggle'),
  pumpRelayToggle: document.getElementById('pumpRelayToggle'),
  pumpModeText: document.getElementById('pumpModeText'),
  pumpStateText: document.getElementById('pumpStateText'),
  pumpLed: document.getElementById('pumpLed'),
  fanModeToggle: document.getElementById('fanModeToggle'),
  fanRelayToggle: document.getElementById('fanRelayToggle'),
  fanModeText: document.getElementById('fanModeText'),
  fanStateText: document.getElementById('fanStateText'),
  fanLed: document.getElementById('fanLed'),
};

// ============================ CHARTS ============================

const chartColors = {
  temp: '#f59e0b',
  hum: '#38bdf8',
  soil: '#22c55e',
};

const charts = {
  temp: null,
  hum: null,
  soil: null,
};

function createChart(id, label, color) {
  return new Chart(document.getElementById(id), {
    type: 'line',
    data: {
      labels: [],
      datasets: [{
        label,
        data: [],
        borderColor: color,
        backgroundColor: color + '26',
        fill: true,
        tension: 0.35,
        borderWidth: 2.5,
        pointRadius: 3,
        pointBackgroundColor: color,
      }],
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      animation: { duration: 400 },
      plugins: {
        legend: { display: false },
        tooltip: {
          mode: 'index',
          intersect: false,
          backgroundColor: 'rgba(17,24,39,0.95)',
          borderColor: 'rgba(255,255,255,0.12)',
          borderWidth: 1,
        },
      },
      scales: {
        x: {
          grid: { color: 'rgba(255,255,255,0.05)' },
          ticks: { color: '#8b9bb4', font: { size: 10 } },
        },
        y: {
          beginAtZero: true,
          grid: { color: 'rgba(255,255,255,0.05)' },
          ticks: { color: '#8b9bb4', font: { size: 10 } },
        },
      },
    },
  });
}

function initCharts() {
  charts.temp = createChart('tempChart', 'Suhu (°C)', chartColors.temp);
  charts.hum = createChart('humChart', 'Kelembapan Udara (%)', chartColors.hum);
  charts.soil = createChart('soilChart', 'Kelembapan Tanah (%)', chartColors.soil);
}

// ============================ UTIL ============================

function setToggle(input, checked) {
  if (input.checked !== checked) input.checked = checked;
}

function fmtTime(d) {
  return d.toLocaleTimeString('id-ID', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
}

function updateClock() {
  el.clock.textContent = fmtTime(new Date());
}

function setBadge(badge, state) {
  badge.classList.remove('online', 'offline');
  badge.classList.add(state);
}

function setModeUI(device, mode) {
  const cfg = device === 'pump'
    ? { toggle: el.pumpModeToggle, text: el.pumpModeText, relay: el.pumpRelayToggle }
    : { toggle: el.fanModeToggle, text: el.fanModeText, relay: el.fanRelayToggle };

  const isAuto = mode === 'auto' || mode === true;
  setToggle(cfg.toggle, isAuto);
  cfg.text.textContent = isAuto ? 'AUTO' : 'MANUAL';
  cfg.text.className = 'state-value badge-mode ' + (isAuto ? 'auto' : 'manual');

  const labels = cfg.toggle.closest('.toggle-group').querySelectorAll('.toggle-label');
  labels[0].classList.toggle('active-left', !isAuto);
  labels[1].classList.toggle('active-right', isAuto);

  cfg.relay.disabled = isAuto;
}

function setRelayUI(device, on) {
  const cfg = device === 'pump'
    ? { relay: el.pumpRelayToggle, stateText: el.pumpStateText, led: el.pumpLed }
    : { relay: el.fanRelayToggle, stateText: el.fanStateText, led: el.fanLed };

  const isOn = !!on;
  setToggle(cfg.relay, isOn);
  cfg.stateText.textContent = isOn ? 'ON' : 'OFF';
  cfg.stateText.style.color = isOn ? 'var(--green)' : 'var(--muted)';
  cfg.led.className = 'led ' + (isOn ? 'led-on' : 'led-dim');
}

// ============================ THINGSPEAK ============================

async function loadThingSpeak() {
  try {
    const res = await fetch(TS_API, { cache: 'no-store' });
    if (!res.ok) throw new Error('HTTP ' + res.status);
    const json = await res.json();
    const feeds = (json.feeds || []).filter(f => f.field1 || f.field2 || f.field3);
    if (!feeds.length) return;

    const last = feeds[feeds.length - 1];
    const suhu = parseFloat(last.field1) || 0;
    const hum = parseFloat(last.field2) || 0;
    const soil = parseFloat(last.field3) || 0;

    updateCharts(feeds);

    if (!espOnline) {
      updateSensorCards(suhu, hum, soil);
    }

    el.lastUpdate.textContent = 'Terakhir diperbarui: ' + fmtTime(new Date());
    setBadge(el.tsStatus, 'online');
  } catch (err) {
    console.warn('ThingSpeak:', err);
    setBadge(el.tsStatus, 'offline');
  }
}

function updateSensorCards(suhu, hum, soil) {
  el.temp.textContent = suhu.toFixed(1) + ' °C';
  el.hum.textContent = hum.toFixed(1) + ' %';
  el.soil.textContent = soil.toFixed(1) + ' %';

  el.tempStatus.textContent = suhu > 30 ? 'Kipas ON' : 'Suhu Normal';
  el.tempStatus.className = 'mini-status ' + (suhu > 30 ? 'warn' : 'good');

  el.humStatus.textContent = hum < 30 ? 'Kering' : hum > 80 ? 'Lembab' : 'Normal';
  el.humStatus.className = 'mini-status ' + (hum < 30 ? 'warn' : hum > 80 ? 'info' : 'good');

  el.soilStatus.textContent = soil < 40 ? 'Pompa ON' : 'Tanah Cukup';
  el.soilStatus.className = 'mini-status ' + (soil < 40 ? 'danger' : 'good');
}

function updateCharts(feeds) {
  const labels = [];
  const temp = [];
  const hum = [];
  const soil = [];

  feeds.forEach(f => {
    labels.push(new Date(f.created_at).toLocaleTimeString('id-ID', { hour: '2-digit', minute: '2-digit' }));
    temp.push(parseFloat(f.field1) || 0);
    hum.push(parseFloat(f.field2) || 0);
    soil.push(parseFloat(f.field3) || 0);
  });

  charts.temp.data.labels = labels;
  charts.temp.data.datasets[0].data = temp;
  charts.temp.update('none');

  charts.hum.data.labels = labels;
  charts.hum.data.datasets[0].data = hum;
  charts.hum.update('none');

  charts.soil.data.labels = labels;
  charts.soil.data.datasets[0].data = soil;
  charts.soil.update('none');
}

// ============================ ESP32 ============================

async function loadESP() {
  try {
    const res = await fetch('/data', { cache: 'no-store' });
    if (!res.ok) throw new Error('HTTP ' + res.status);
    const d = await res.json();

    espOnline = true;
    setBadge(el.espStatus, 'online');

    // data sensor realtime dari ESP32 (lebih cepat dari ThingSpeak)
    if (typeof d.suhu === 'number' || typeof d.suhu === 'string') {
      const suhu = parseFloat(d.suhu) || 0;
      const hum = parseFloat(d.humidity) || 0;
      const soil = parseFloat(d.soil) || 0;
      updateSensorCards(suhu, hum, soil);
    }

    // status relay
    setRelayUI('pump', d.pompa === true || d.pompa === 1 || d.pompa === '1' || d.pompa === 'true');
    setRelayUI('fan', d.kipas === true || d.kipas === 1 || d.kipas === '1' || d.kipas === 'true');

    // mode sistem
    setModeUI('pump', d.modePompa);
    setModeUI('fan', d.modeKipas);
  } catch (err) {
    console.warn('ESP32:', err);
    espOnline = false;
    setBadge(el.espStatus, 'offline');
  }
}

async function sendCommand(endpoint) {
  try {
    const res = await fetch(endpoint, { cache: 'no-store' });
    const text = await res.text();
    console.log('CMD', endpoint, '->', text);
  } catch (err) {
    console.warn('CMD', endpoint, err);
  }
  // tunggu ESP32 memproses, lalu resync status asli
  setTimeout(loadESP, 250);
}

// ============================ EVENT ============================

function bindEvents() {
  el.pumpModeToggle.addEventListener('change', () => {
    const mode = el.pumpModeToggle.checked ? 'auto' : 'manual';
    setModeUI('pump', mode);
    sendCommand('/pompa/' + mode);
  });

  el.fanModeToggle.addEventListener('change', () => {
    const mode = el.fanModeToggle.checked ? 'auto' : 'manual';
    setModeUI('fan', mode);
    sendCommand('/kipas/' + mode);
  });

  el.pumpRelayToggle.addEventListener('change', () => {
    if (el.pumpModeToggle.checked) { loadESP(); return; } // dikunci saat AUTO
    sendCommand('/pompa/' + (el.pumpRelayToggle.checked ? 'on' : 'off'));
  });

  el.fanRelayToggle.addEventListener('change', () => {
    if (el.fanModeToggle.checked) { loadESP(); return; } // dikunci saat AUTO
    sendCommand('/kipas/' + (el.fanRelayToggle.checked ? 'on' : 'off'));
  });
}

// ============================ START ============================

function init() {
  initCharts();
  bindEvents();
  updateClock();
  setInterval(updateClock, 1000);
  loadThingSpeak();
  loadESP();
  setInterval(loadThingSpeak, TS_INTERVAL);
  setInterval(loadESP, ESP_INTERVAL);
}

document.addEventListener('DOMContentLoaded', init);

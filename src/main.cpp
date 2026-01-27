/**
 * Mawaqit Prayer Times Display v3.1
 * ESP32-S3 + JC4827W543 + GT911 Touch
 * 
 * ============================================
 * DOKUMENTATION DER FIXES UND FEATURES
 * ============================================
 * 
 * DISPLAY ROTATION FIX:
 * - Problem: NV3041A + Canvas zeigt Pixelfehler bei Rotation im Konstruktor
 * - Lösung: Display mit Rotation 0 init, dann setRotation() nach begin()
 * - Werte: 0 = Normal Landscape, 2 = 180° gedreht
 * 
 * TOUCH SYSTEM:
 * - Einfaches Touch-System: Überall tippen = nächster Modus
 * - Display Modi: 0=Gebetsliste, 1=Analog-Uhr, 2=Countdown, 3=Settings
 * - Debounce: 500ms zwischen Touches
 * 
 * ADHAN SYSTEM:
 * - Audio über I2S DAC (Pin 17)
 * - LittleFS für MP3 Speicherung
 * - Pro-Gebet aktivierbar/deaktivierbar
 * - Touch während Adhan = Stop
 * 
 * WEB UI:
 * - Responsive Design für Mobile
 * - Live Analog-Uhr mit SVG
 * - Audio-Player mit Stop-Funktion
 * - WiFi-Scanner und Moschee-Suche
 */

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <LittleFS.h>

#include "config.h"
#include "display_manager.h"
#include "touch_manager.h"

// Global objects
DisplayManager display;
TouchManager touch;
DNSServer dnsServer;
WebServer server(80);
Preferences prefs;

// State
String savedSSID = "";
String savedPassword = "";
String mosqueId = "";
String mosqueName = "";
PrayerTimes currentTimes;
bool timesLoaded = false;
bool apMode = true;

// Adhan settings
bool adhanFajr = true;
bool adhanShuruk = false;  // Sunrise alarm - OFF by default
bool adhanDhuhr = true;
bool adhanAsr = true;
bool adhanMaghrib = true;
bool adhanIsha = true;
bool adhanPlaying = false;  // Track if adhan is currently playing
int settingsSelection = -1;  // Which toggle is selected in settings (-1 = none)

// Theme and Language settings
int currentLanguage = 0;  // 0=Deutsch, 1=English, 2=Français, 3=Türkçe, 4=العربية
int currentTheme = 0;     // 0=Green, 1=Blue, 2=Purple, 3=Dark
bool autoNightMode = true; // Auto switch to dark theme at night

// Battery monitoring - DISABLED (uncomment if you have battery connected)
// #define BATTERY_PIN 4  // GPIO4 for battery voltage divider
// float batteryVoltage = 0.0;
// int batteryPercent = 100;

// Display state  
int displayMode = 0;  // 0=List, 1=Clock, 2=NextPrayer, 3=Settings
unsigned long lastTimeUpdate = 0;

// HTML Page with Tabs - Enhanced UI v2
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>Mawaqit</title>
    <style>
        :root { --green: #1a5f2a; --light: #90EE90; --dark: #0d3d17; --gold: #FFD700; --accent: #4CAF50; }
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(160deg, #1e5631 0%, #0a2e14 100%);
            min-height: 100vh; color: white; padding: 8px; max-width: 500px; margin: 0 auto;
        }
        .header { display: flex; align-items: center; justify-content: center; gap: 8px; margin-bottom: 10px; }
        .header h1 { font-size: 18px; }
        .tabs { display: flex; gap: 3px; margin-bottom: 10px; background: rgba(0,0,0,0.2); padding: 4px; border-radius: 10px; }
        .tab {
            flex: 1; padding: 10px 6px; text-align: center;
            background: transparent; cursor: pointer;
            border: none; color: rgba(255,255,255,0.7); font-size: 13px; border-radius: 8px;
            transition: all 0.2s;
        }
        .tab.active { background: var(--accent); color: white; font-weight: 600; box-shadow: 0 2px 8px rgba(0,0,0,0.3); }
        .panel { display: none; animation: fadeIn 0.3s; }
        .panel.active { display: block; }
        @keyframes fadeIn { from { opacity: 0; transform: translateY(5px); } to { opacity: 1; transform: translateY(0); } }
        .card { background: rgba(255,255,255,0.1); border-radius: 12px; padding: 14px; margin-bottom: 10px; backdrop-filter: blur(10px); }
        .card h2 { margin-bottom: 10px; font-size: 13px; color: var(--light); text-transform: uppercase; letter-spacing: 0.5px; }
        input, select { width: 100%; padding: 12px; border: none; border-radius: 8px; margin-bottom: 8px; font-size: 15px; background: rgba(255,255,255,0.95); color: #333; }
        button { width: 100%; padding: 12px; background: var(--accent); color: white; border: none; border-radius: 8px; font-size: 14px; cursor: pointer; font-weight: 500; transition: transform 0.1s, background 0.2s; }
        button:active { transform: scale(0.98); background: #388E3C; }
        .btn-sm { padding: 10px 14px; font-size: 13px; }
        .btn-outline { background: transparent; border: 1px solid rgba(255,255,255,0.3); }
        .btn-danger { background: #e53935; }
        .status { padding: 8px; border-radius: 8px; margin-top: 8px; text-align: center; font-size: 13px; }
        .success { background: var(--accent); }
        .error { background: #e53935; }
        .list { max-height: 160px; overflow-y: auto; }
        .item { padding: 10px 12px; background: rgba(255,255,255,0.08); border-radius: 8px; margin-bottom: 6px; cursor: pointer; display: flex; justify-content: space-between; align-items: center; transition: background 0.2s; }
        .item:hover { background: rgba(255,255,255,0.15); }
        .item-name { font-weight: 600; font-size: 14px; }
        .item-sub { font-size: 11px; color: #aaa; }
        
        /* Clock */
        .clock-box { display: flex; align-items: center; justify-content: center; gap: 20px; padding: 16px; background: rgba(0,0,0,0.3); border-radius: 14px; margin-bottom: 12px; }
        .analog-clock { width: 110px; height: 110px; }
        .analog-clock svg { width: 100%; height: 100%; }
        .clock-face { fill: rgba(255,255,255,0.08); stroke: var(--light); stroke-width: 2; }
        .clock-center { fill: var(--gold); }
        .hour-hand { stroke: white; stroke-width: 3.5; stroke-linecap: round; }
        .minute-hand { stroke: var(--light); stroke-width: 2.5; stroke-linecap: round; }
        .second-hand { stroke: var(--gold); stroke-width: 1.5; stroke-linecap: round; }
        .digital { text-align: center; }
        .digital-time { font-size: 38px; font-weight: 700; font-family: 'SF Mono', monospace; letter-spacing: -1px; }
        .digital-date { font-size: 13px; color: #aaa; margin-top: 4px; }
        
        /* Next Prayer */
        .next-prayer { text-align: center; padding: 16px; background: linear-gradient(135deg, rgba(76,175,80,0.35) 0%, rgba(46,125,50,0.2) 100%); border-radius: 14px; margin-bottom: 12px; border: 1px solid rgba(76,175,80,0.4); }
        .next-label { font-size: 11px; color: #bbb; text-transform: uppercase; letter-spacing: 1px; }
        .next-name { font-size: 28px; font-weight: 700; color: var(--gold); margin: 4px 0; text-shadow: 0 2px 10px rgba(0,0,0,0.3); }
        .next-time { font-size: 15px; color: #ddd; }
        .countdown { font-size: 32px; font-weight: 700; color: white; margin-top: 6px; font-family: 'SF Mono', monospace; }
        
        /* Prayer rows */
        .prayer-grid { display: grid; gap: 6px; }
        .prayer-row { display: flex; justify-content: space-between; align-items: center; padding: 10px 12px; background: rgba(255,255,255,0.06); border-radius: 8px; }
        .prayer-row.sunrise { background: linear-gradient(90deg, rgba(255,152,0,0.15), transparent); border-left: 3px solid #FF9800; }
        .prayer-row.current { background: linear-gradient(90deg, rgba(76,175,80,0.25), transparent); border-left: 3px solid var(--light); }
        .prayer-row.passed { opacity: 0.4; }
        .prayer-name { font-weight: 600; font-size: 14px; }
        .prayer-time { font-size: 16px; font-family: 'SF Mono', monospace; font-weight: 500; }
        
        /* Toggle */
        .toggle-row { display: flex; justify-content: space-between; align-items: center; padding: 10px 0; border-bottom: 1px solid rgba(255,255,255,0.1); }
        .toggle-row:last-child { border-bottom: none; }
        .toggle { width: 46px; height: 26px; background: #555; border-radius: 13px; position: relative; cursor: pointer; transition: background 0.2s; }
        .toggle.on { background: var(--accent); }
        .toggle::after { content: ''; position: absolute; width: 22px; height: 22px; background: white; border-radius: 50%; top: 2px; left: 2px; transition: 0.2s; box-shadow: 0 2px 4px rgba(0,0,0,0.2); }
        .toggle.on::after { left: 22px; }
        
        .flex-row { display: flex; gap: 8px; }
        .flex-row > * { flex: 1; }
        
        /* Audio player */
        .audio-box { background: rgba(0,0,0,0.2); border-radius: 10px; padding: 12px; margin-top: 10px; }
        .audio-box audio { width: 100%; margin-top: 8px; }
        
        /* Info box */
        .info-box { background: rgba(255,193,7,0.15); border: 1px solid rgba(255,193,7,0.3); border-radius: 10px; padding: 12px; margin-bottom: 12px; }
        .info-box h3 { color: var(--gold); font-size: 13px; margin-bottom: 6px; }
        .info-box p { font-size: 12px; line-height: 1.5; color: #ddd; }
    </style>
</head>
<body>
    <div class="header">
        <span style="font-size:24px">☪️</span>
        <h1>Mawaqit Gebetszeiten</h1>
    </div>
    
    <div class="tabs">
        <button class="tab active" onclick="showTab('home',this)">🏠 Start</button>
        <button class="tab" onclick="showTab('wifi',this)">📶 WLAN</button>
        <button class="tab" onclick="showTab('mosque',this)">🕌 Moschee</button>
        <button class="tab" onclick="showTab('settings',this)">⚙️ Einst.</button>
    </div>
    
    <!-- HOME -->
    <div id="home" class="panel active">
        <div class="clock-box">
            <div class="analog-clock">
                <svg viewBox="0 0 100 100">
                    <circle class="clock-face" cx="50" cy="50" r="47"/>
                    <g id="clockMarks"></g>
                    <line id="hourHand" class="hour-hand" x1="50" y1="50" x2="50" y2="26"/>
                    <line id="minuteHand" class="minute-hand" x1="50" y1="50" x2="50" y2="16"/>
                    <line id="secondHand" class="second-hand" x1="50" y1="50" x2="50" y2="12"/>
                    <circle class="clock-center" cx="50" cy="50" r="4"/>
                </svg>
            </div>
            <div class="digital">
                <div class="digital-time" id="clock">--:--:--</div>
                <div class="digital-date" id="date">--</div>
            </div>
        </div>
        
        <div class="next-prayer">
            <div class="next-label">Nächstes Gebet</div>
            <div class="next-name" id="nextName">--</div>
            <div class="next-time">um <span id="nextTime">--:--</span> Uhr</div>
            <div class="countdown" id="countdown">--:--:--</div>
        </div>
        
        <div class="card">
            <h2 id="mosqueLbl">Gebetszeiten</h2>
            <div class="prayer-grid" id="times"><p style="text-align:center;padding:20px">Laden...</p></div>
        </div>
    </div>
    
    <!-- WIFI -->
    <div id="wifi" class="panel">
        <div class="card">
            <h2>📡 Verfügbare Netzwerke</h2>
            <button onclick="scanWifi()">🔄 Netzwerke scannen</button>
            <div class="list" id="wifiList" style="margin-top:10px"></div>
        </div>
        <div class="card">
            <h2>🔐 Verbindung</h2>
            <input type="text" id="ssid" placeholder="WLAN Name (SSID)">
            <input type="password" id="pass" placeholder="Passwort">
            <button onclick="connectWifi()">📶 Verbinden</button>
            <div id="wifiStatus"></div>
        </div>
        <div class="card">
            <h2>📊 Status</h2>
            <div id="wifiInfo" style="font-size:13px;line-height:1.6">--</div>
        </div>
    </div>
    
    <!-- MOSQUE -->
    <div id="mosque" class="panel">
        <div class="card">
            <h2>🔍 Moschee suchen</h2>
            <input type="text" id="searchQ" placeholder="Stadt oder Moschee Name">
            <button onclick="searchMosque()">🔍 Suchen</button>
            <div id="searchStatus"></div>
            <div class="list" id="mosqueList" style="margin-top:10px"></div>
        </div>
        <div class="card">
            <h2>✅ Aktuelle Moschee</h2>
            <div id="curMosque" style="font-size:15px;font-weight:600">Keine ausgewählt</div>
        </div>
    </div>
    
    <!-- SETTINGS -->
    <div id="settings" class="panel">
        <div class="info-box">
            <h3>💡 Touch-Steuerung am Display</h3>
            <p>• <b>Oben tippen</b> → Nächster Anzeigemodus<br>• <b>Links tippen</b> → Vorheriger Modus<br>• <b>Rechts tippen</b> → Nächster Modus</p>
        </div>
        
        <div class="card">
            <h2>🔔 Adhan für Gebete</h2>
            <div class="toggle-row"><span>Fajr</span><div class="toggle on" id="tFajr" onclick="toggleAdhan('fajr',this)"></div></div>
            <div class="toggle-row"><span>Dhuhr</span><div class="toggle on" id="tDhuhr" onclick="toggleAdhan('dhuhr',this)"></div></div>
            <div class="toggle-row"><span>Asr</span><div class="toggle on" id="tAsr" onclick="toggleAdhan('asr',this)"></div></div>
            <div class="toggle-row"><span>Maghrib</span><div class="toggle on" id="tMaghrib" onclick="toggleAdhan('maghrib',this)"></div></div>
            <div class="toggle-row"><span>Isha</span><div class="toggle on" id="tIsha" onclick="toggleAdhan('isha',this)"></div></div>
        </div>
        
        <div class="card">
            <h2>🎵 Adhan Audio</h2>
            <div class="audio-box">
                <p style="font-size:12px;color:#aaa;margin-bottom:8px">Adhan-Vorschau (Browser):</p>
                <audio id="adhanAudio" controls style="width:100%">
                    <source src="/adhan.mp3" type="audio/mpeg">
                </audio>
            </div>
            <div class="flex-row" style="margin-top:10px">
                <button class="btn-sm" onclick="testAdhan()">▶️ Am Gerät testen</button>
                <button class="btn-sm btn-danger" onclick="stopAdhan()">⏹️ Stop</button>
            </div>
            <div style="margin-top:12px">
                <input type="file" id="mp3File" accept=".mp3">
                <button onclick="uploadMp3()" style="margin-top:6px">📤 Eigene MP3 hochladen</button>
            </div>
            <div id="uploadStatus"></div>
        </div>
        
        <div class="card">
            <h2>🛠️ System</h2>
            <div class="flex-row">
                <button onclick="rotateDisp()">🔄 Display drehen</button>
            </div>
            <div class="flex-row" style="margin-top:8px">
                <button class="btn-danger" onclick="if(confirm('Gerät neustarten?')){fetch('/api/reboot');setTimeout(()=>location.reload(),4000)}">🔁 Neustart</button>
            </div>
        </div>
    </div>

<script>
let prayerData=[], nextPrayerMins=0;

function showTab(id,btn){
    document.querySelectorAll('.panel').forEach(p=>p.classList.remove('active'));
    document.querySelectorAll('.tab').forEach(t=>t.classList.remove('active'));
    document.getElementById(id).classList.add('active');
    btn.classList.add('active');
}

function initAnalogClock(){
    const marks = document.getElementById('clockMarks');
    for(let i=0; i<60; i++){
        const angle = i * 6 - 90;
        const rad = angle * Math.PI / 180;
        const inner = i % 5 === 0 ? 38 : 42;
        const outer = 46;
        const x1 = 50 + inner * Math.cos(rad);
        const y1 = 50 + inner * Math.sin(rad);
        const x2 = 50 + outer * Math.cos(rad);
        const y2 = 50 + outer * Math.sin(rad);
        const line = document.createElementNS('http://www.w3.org/2000/svg','line');
        line.setAttribute('x1',x1);
        line.setAttribute('y1',y1);
        line.setAttribute('x2',x2);
        line.setAttribute('y2',y2);
        line.setAttribute('class', i % 5 === 0 ? 'clock-mark-hour' : 'clock-mark');
        marks.appendChild(line);
    }
}

function updateClock(){
    const now = new Date();
    const h = now.getHours(), m = now.getMinutes(), s = now.getSeconds();
    
    // Digital clock
    document.getElementById('clock').textContent = 
        String(h).padStart(2,'0') + ':' + String(m).padStart(2,'0') + ':' + String(s).padStart(2,'0');
    document.getElementById('date').textContent = 
        now.toLocaleDateString('de-DE',{weekday:'short',day:'numeric',month:'short'});
    
    // Analog clock hands
    const hourAngle = (h % 12 + m/60) * 30 - 90;
    const minAngle = m * 6 - 90;
    const secAngle = s * 6 - 90;
    
    setHand('hourHand', hourAngle, 22);
    setHand('minuteHand', minAngle, 32);
    setHand('secondHand', secAngle, 36);
    
    // Update countdown
    updateCountdown(h, m, s);
}

function setHand(id, angle, length){
    const rad = angle * Math.PI / 180;
    const x = 50 + length * Math.cos(rad);
    const y = 50 + length * Math.sin(rad);
    const hand = document.getElementById(id);
    hand.setAttribute('x2', x);
    hand.setAttribute('y2', y);
}

function updateCountdown(h, m, s){
    if(nextPrayerMins <= 0) return;
    const nowMins = h * 60 + m;
    let diff = nextPrayerMins - nowMins;
    if(diff < 0) diff += 24 * 60; // Next day
    
    const diffSecs = diff * 60 - s;
    const hours = Math.floor(diffSecs / 3600);
    const mins = Math.floor((diffSecs % 3600) / 60);
    const secs = diffSecs % 60;
    
    document.getElementById('countdown').textContent = 
        String(hours).padStart(2,'0') + ':' + String(mins).padStart(2,'0') + ':' + String(secs).padStart(2,'0');
}

initAnalogClock();
setInterval(updateClock, 1000);
updateClock();

window.onload=function(){
    loadStatus();
    loadTimes();
    loadSettings();
};

function loadStatus(){
    fetch('/api/status').then(r=>r.json()).then(d=>{
        document.getElementById('wifiInfo').innerHTML='<b>SSID:</b> '+(d.ssid||'--')+'<br><b>IP:</b> '+(d.ip||'--')+'<br><b>Signal:</b> '+(d.rssi||'--')+' dBm';
        document.getElementById('curMosque').innerHTML='<b>'+(d.mosqueName||'Keine')+'</b>';
        if(d.mosqueName) document.getElementById('mosqueLbl').textContent=d.mosqueName;
    }).catch(()=>{});
}

function loadTimes(){
    fetch('/api/times').then(r=>r.json()).then(d=>{
        if(!d.valid){document.getElementById('times').innerHTML='<p>Keine Zeiten</p>';return;}
        prayerData=[{n:'Fajr',t:d.fajr},{n:'Shuruk',t:d.sunrise,s:1},{n:'Dhuhr',t:d.dhuhr},{n:'Asr',t:d.asr},{n:'Maghrib',t:d.maghrib},{n:'Isha',t:d.isha}];
        
        const now=new Date(), nowMins=now.getHours()*60+now.getMinutes();
        let html='', nextSet=false, currentIdx=-1;
        
        // Find current prayer (last one that passed)
        for(let i=prayerData.length-1; i>=0; i--){
            if(prayerData[i].s) continue;
            const[hh,mm]=prayerData[i].t.split(':').map(Number);
            if(hh*60+mm <= nowMins){ currentIdx=i; break; }
        }
        
        prayerData.forEach((x,i)=>{
            const[hh,mm]=x.t.split(':').map(Number);
            const pMins=hh*60+mm;
            let cls=x.s?'sunrise':'';
            if(i===currentIdx) cls+=' current';
            else if(pMins<nowMins && !x.s) cls+=' passed';
            
            html+='<div class="prayer-row '+cls+'"><span class="prayer-name">'+x.n+'</span><span class="prayer-time">'+x.t+'</span></div>';
            
            if(!nextSet && !x.s && pMins>nowMins){
                document.getElementById('nextName').textContent=x.n;
                document.getElementById('nextTime').textContent=x.t;
                nextPrayerMins=pMins;
                nextSet=true;
            }
        });
        
        // If no next prayer today, next is Fajr tomorrow
        if(!nextSet){
            document.getElementById('nextName').textContent='Fajr';
            document.getElementById('nextTime').textContent=prayerData[0].t;
            const[hh,mm]=prayerData[0].t.split(':').map(Number);
            nextPrayerMins=hh*60+mm+24*60;
        }
        
        document.getElementById('times').innerHTML=html;
    }).catch(()=>document.getElementById('times').innerHTML='<p>Fehler</p>');
}

function loadSettings(){
    fetch('/api/settings').then(r=>r.json()).then(d=>{
        ['fajr','dhuhr','asr','maghrib','isha'].forEach(p=>{
            const el=document.getElementById('t'+p.charAt(0).toUpperCase()+p.slice(1));
            if(el)el.classList.toggle('on',d['adhan_'+p]!==false);
        });
    }).catch(()=>{});
}

function scanWifi(){
    document.getElementById('wifiList').innerHTML='<p>Scanne...</p>';
    fetch('/api/scan').then(r=>r.json()).then(d=>{
        if(!d.length){document.getElementById('wifiList').innerHTML='<p>Keine gefunden</p>';return;}
        let h='';
        d.forEach(n=>h+='<div class="item" onclick="selWifi(\''+n.ssid.replace(/'/g,"\\'")+'\')">'
            +'<span class="item-name">'+n.ssid+'</span><span class="item-sub">'+n.rssi+' dBm</span></div>');
        document.getElementById('wifiList').innerHTML=h;
    }).catch(()=>document.getElementById('wifiList').innerHTML='<p>Fehler</p>');
}

function selWifi(s){document.getElementById('ssid').value=s;document.getElementById('pass').focus();}

function connectWifi(){
    const s=document.getElementById('ssid').value,p=document.getElementById('pass').value;
    if(!s){showSt('wifiStatus','SSID fehlt','error');return;}
    showSt('wifiStatus','Verbinde...','');
    fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'ssid='+encodeURIComponent(s)+'&pass='+encodeURIComponent(p)})
        .then(()=>{showSt('wifiStatus','Verbinde... Seite neu laden','success');setTimeout(()=>location.reload(),5000);})
        .catch(()=>showSt('wifiStatus','Fehler','error'));
}

function searchMosque(){
    const q=document.getElementById('searchQ').value;
    if(q.length<2){showSt('searchStatus','Min 2 Zeichen','error');return;}
    document.getElementById('mosqueList').innerHTML='<p>Suche...</p>';
    fetch('/api/mosque/search?q='+encodeURIComponent(q)).then(r=>r.json()).then(d=>{
        if(!d.length){document.getElementById('mosqueList').innerHTML='<p>Keine</p>';return;}
        let h='';
        d.forEach(m=>h+='<div class="item" onclick="selMosque(\''+m.uuid+'\',\''+m.name.replace(/'/g,"\\'")+'\')">'
            +'<div><div class="item-name">'+m.name+'</div><div class="item-sub">'+(m.city||'')+'</div></div></div>');
        document.getElementById('mosqueList').innerHTML=h;
    }).catch(()=>document.getElementById('mosqueList').innerHTML='<p>Fehler</p>');
}

function selMosque(id,name){
    fetch('/api/mosque',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'id='+encodeURIComponent(id)+'&name='+encodeURIComponent(name)})
        .then(()=>{showSt('searchStatus','Gespeichert!','success');loadStatus();loadTimes();})
        .catch(()=>showSt('searchStatus','Fehler','error'));
}

function toggleAdhan(p,el){
    const on=!el.classList.contains('on');
    el.classList.toggle('on',on);
    fetch('/api/adhan',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'prayer='+p+'&enabled='+on});
}

function testAdhan(){
    showSt('uploadStatus','Adhan wird abgespielt...','success');
    fetch('/api/adhan/play').then(()=>{
        setTimeout(()=>showSt('uploadStatus','',''),3000);
    });
}

function stopAdhan(){
    // Stop browser audio
    const audio = document.getElementById('adhanAudio');
    if(audio) { audio.pause(); audio.currentTime = 0; }
    // Stop device audio
    fetch('/api/adhan/stop');
    showSt('uploadStatus','Gestoppt','');
    setTimeout(()=>showSt('uploadStatus','',''),1500);
}

function uploadMp3(){
    const f=document.getElementById('mp3File').files[0];
    if(!f){showSt('uploadStatus','Bitte MP3 Datei auswählen','error');return;}
    if(f.size>2*1024*1024){showSt('uploadStatus','Datei zu gro\u00df (max 2MB)','error');return;}
    showSt('uploadStatus','Hochladen... ('+Math.round(f.size/1024)+'KB','');
    const fd=new FormData();fd.append('file',f);
    fetch('/api/upload',{method:'POST',body:fd})
        .then(r=>r.json()).then(d=>{
            showSt('uploadStatus',d.success?'✓ Hochgeladen!':'Fehler: '+d.error,d.success?'success':'error');
            if(d.success) document.getElementById('adhanAudio').load();
        })
        .catch(()=>showSt('uploadStatus','Upload fehlgeschlagen','error'));
}

function rotateDisp(){
    showSt('uploadStatus','Display wird gedreht...','success');
    fetch('/api/rotate').then(()=>{
        showSt('uploadStatus','Gerät startet neu...','success');
    });
}

function showSt(id,msg,type){
    const el=document.getElementById(id);
    if(!el)return;
    el.textContent=msg;
    el.className='status'+(type?' '+type:'');
    if(!msg)el.style.display='none';
    else el.style.display='block';
}
</script>
</body>
</html>
)rawliteral";

// Display rotation state
int displayRotation = 2;  // Start with 180°

// Forward declarations
void loadSettings();
void saveSettings();
void setupWebServer();
void startAPMode();
void connectWiFi();
void fetchPrayerTimes();

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n====================================");
    Serial.println("  Mawaqit Prayer Times v3.0");
    Serial.println("====================================\n");
    
    // Initialize LittleFS for audio files
    if (!LittleFS.begin(true)) {
        Serial.println("[FS] LittleFS mount failed!");
    } else {
        Serial.println("[FS] LittleFS mounted");
    }
    
    // EINMALIG: Einstellungen löschen für Neustart (auskommentiert)
    // Preferences p;
    // p.begin("mawaqit", false);
    // p.clear();
    // p.end();
    // Serial.println("Settings cleared!");
    
    display.begin();
    display.showSplashScreen();
    delay(1500);
    
    touch.begin(&display);
    loadSettings();
    setupWebServer();
    
    if (savedSSID.length() > 0) {
        connectWiFi();
    } else {
        startAPMode();
    }
}

void loadSettings() {
    prefs.begin("mawaqit", true);
    savedSSID = prefs.getString("wifi_ssid", "");
    savedPassword = prefs.getString("wifi_pass", "");
    mosqueId = prefs.getString("mosque_id", "");
    mosqueName = prefs.getString("mosque_name", "");
    adhanFajr = prefs.getBool("adhan_fajr", true);
    adhanDhuhr = prefs.getBool("adhan_dhuhr", true);
    adhanAsr = prefs.getBool("adhan_asr", true);
    adhanMaghrib = prefs.getBool("adhan_maghrib", true);
    adhanIsha = prefs.getBool("adhan_isha", true);
    adhanShuruk = prefs.getBool("adhan_shuruk", false);
    currentLanguage = prefs.getInt("language", 0);
    currentTheme = prefs.getInt("theme", 0);
    autoNightMode = prefs.getBool("auto_night", true);
    prefs.end();
}

void saveSettings() {
    prefs.begin("mawaqit", false);
    prefs.putString("wifi_ssid", savedSSID);
    prefs.putString("wifi_pass", savedPassword);
    prefs.putString("mosque_id", mosqueId);
    prefs.putString("mosque_name", mosqueName);
    prefs.putBool("adhan_fajr", adhanFajr);
    prefs.putBool("adhan_dhuhr", adhanDhuhr);
    prefs.putBool("adhan_asr", adhanAsr);
    prefs.putBool("adhan_maghrib", adhanMaghrib);
    prefs.putBool("adhan_isha", adhanIsha);
    prefs.putBool("adhan_shuruk", adhanShuruk);
    prefs.putInt("language", currentLanguage);
    prefs.putInt("theme", currentTheme);
    prefs.putBool("auto_night", autoNightMode);
    prefs.end();
}

void setupWebServer() {
    // Main page
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", INDEX_HTML);
    });
    
    // Serve adhan.mp3 from LittleFS
    server.on("/adhan.mp3", HTTP_GET, []() {
        if (LittleFS.exists("/adhan.mp3")) {
            File file = LittleFS.open("/adhan.mp3", "r");
            server.streamFile(file, "audio/mpeg");
            file.close();
        } else {
            server.send(404, "text/plain", "No adhan.mp3 uploaded");
        }
    });
    
    // Upload MP3 file
    server.on("/api/upload", HTTP_POST, []() {
        server.send(200, "application/json", "{\"success\":true}");
    }, []() {
        HTTPUpload& upload = server.upload();
        static File uploadFile;
        
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("[UPLOAD] Start: %s\n", upload.filename.c_str());
            uploadFile = LittleFS.open("/adhan.mp3", "w");
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (uploadFile) {
                uploadFile.write(upload.buf, upload.currentSize);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (uploadFile) {
                uploadFile.close();
                Serial.printf("[UPLOAD] Done: %d bytes\n", upload.totalSize);
            }
        }
    });
    
    // Status API
    server.on("/api/status", HTTP_GET, []() {
        JsonDocument doc;
        doc["ssid"] = WiFi.SSID();
        doc["ip"] = WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
        doc["mosqueId"] = mosqueId;
        doc["mosqueName"] = mosqueName;
        doc["apMode"] = apMode;
        String json;
        serializeJson(doc, json);
        server.send(200, "application/json", json);
    });
    
    // Prayer times API
    server.on("/api/times", HTTP_GET, []() {
        JsonDocument doc;
        doc["valid"] = currentTimes.valid;
        doc["fajr"] = currentTimes.fajr;
        doc["sunrise"] = currentTimes.sunrise;
        doc["dhuhr"] = currentTimes.dhuhr;
        doc["asr"] = currentTimes.asr;
        doc["maghrib"] = currentTimes.maghrib;
        doc["isha"] = currentTimes.isha;
        String json;
        serializeJson(doc, json);
        server.send(200, "application/json", json);
    });
    
    // Settings API
    server.on("/api/settings", HTTP_GET, []() {
        JsonDocument doc;
        doc["adhan_fajr"] = adhanFajr;
        doc["adhan_dhuhr"] = adhanDhuhr;
        doc["adhan_asr"] = adhanAsr;
        doc["adhan_maghrib"] = adhanMaghrib;
        doc["adhan_isha"] = adhanIsha;
        String json;
        serializeJson(doc, json);
        server.send(200, "application/json", json);
    });
    
    // WiFi scan
    server.on("/api/scan", HTTP_GET, []() {
        Serial.println("[API] Scanning WiFi...");
        int n = WiFi.scanNetworks();
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < n && i < 15; i++) {
            JsonObject net = arr.add<JsonObject>();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
        }
        String json;
        serializeJson(doc, json);
        server.send(200, "application/json", json);
        Serial.printf("[API] Found %d networks\n", n);
    });
    
    // WiFi connect
    server.on("/api/wifi", HTTP_POST, []() {
        savedSSID = server.arg("ssid");
        savedPassword = server.arg("pass");
        saveSettings();
        server.send(200, "text/plain", "OK");
        Serial.printf("[API] WiFi saved: %s\n", savedSSID.c_str());
        delay(500);
        connectWiFi();
    });
    
    // Mosque search - PROXY to avoid CORS
    server.on("/api/mosque/search", HTTP_GET, []() {
        String query = server.arg("q");
        Serial.printf("[API] Mosque search: %s\n", query.c_str());
        
        if (query.length() < 2) {
            server.send(400, "application/json", "[]");
            return;
        }
        
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure();
        
        String url = "https://mawaqit.net/api/2.0/mosque/search?word=" + query;
        http.begin(client, url);
        int code = http.GET();
        
        if (code == 200) {
            String response = http.getString();
            server.send(200, "application/json", response);
            Serial.println("[API] Mosque search OK");
        } else {
            server.send(500, "application/json", "[]");
            Serial.printf("[API] Mosque search failed: %d\n", code);
        }
        http.end();
    });
    
    // Save mosque
    server.on("/api/mosque", HTTP_POST, []() {
        mosqueId = server.arg("id");
        mosqueName = server.arg("name");
        saveSettings();
        server.send(200, "text/plain", "OK");
        Serial.printf("[API] Mosque saved: %s\n", mosqueName.c_str());
        fetchPrayerTimes();
    });
    
    // Adhan toggle
    server.on("/api/adhan", HTTP_POST, []() {
        String prayer = server.arg("prayer");
        bool enabled = server.arg("enabled") == "true";
        
        if (prayer == "fajr") adhanFajr = enabled;
        else if (prayer == "dhuhr") adhanDhuhr = enabled;
        else if (prayer == "asr") adhanAsr = enabled;
        else if (prayer == "maghrib") adhanMaghrib = enabled;
        else if (prayer == "isha") adhanIsha = enabled;
        
        saveSettings();
        server.send(200, "text/plain", "OK");
        Serial.printf("[API] Adhan %s: %s\n", prayer.c_str(), enabled ? "on" : "off");
    });
    
    // Adhan play
    server.on("/api/adhan/play", HTTP_GET, []() {
        Serial.println("[API] Adhan play requested");
        adhanPlaying = true;
        display.showAdhanScreen("Test", true);
        // TODO: Implement actual audio playback
        server.send(200, "application/json", "{\"playing\":true}");
    });
    
    // Adhan stop
    server.on("/api/adhan/stop", HTTP_GET, []() {
        Serial.println("[API] Adhan stop requested");
        adhanPlaying = false;
        // Return to current display mode
        if (timesLoaded) {
            switch (displayMode) {
                case 0: display.showPrayerList(currentTimes, -1); break;
                case 1: display.showClockScreen(currentTimes); break;
                case 2: display.showNextPrayer(currentTimes); break;
            }
        }
        server.send(200, "application/json", "{\"stopped\":true}");
    });
    
    // Reboot
    server.on("/api/reboot", HTTP_GET, []() {
        server.send(200, "text/plain", "Rebooting...");
        delay(500);
        ESP.restart();
    });
    
    // Rotate display - save to preferences and reboot
    server.on("/api/rotate", HTTP_GET, []() {
        Preferences p;
        p.begin("mawaqit", false);
        int rot = p.getInt("rotation", 0);
        // Toggle between 0 and 2 (180° flip)
        rot = (rot == 0) ? 2 : 0;
        p.putInt("rotation", rot);
        p.end();
        Serial.printf("[API] Rotation saved: %d, rebooting...\n", rot);
        server.send(200, "application/json", "{\"rotation\":" + String(rot) + ",\"rebooting\":true}");
        delay(500);
        ESP.restart();
    });
    
    // Reboot
    server.on("/api/reboot", HTTP_GET, []() {
        server.send(200, "text/plain", "OK");
        delay(500);
        ESP.restart();
    });
    
    // Captive portal redirect
    server.onNotFound([]() {
        server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
        server.send(302);
    });
}

void startAPMode() {
    Serial.println("[WIFI] Starting AP...");
    apMode = true;
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);
    delay(500);
    
    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("[WIFI] AP: %s\n", apIP.toString().c_str());
    
    dnsServer.start(53, "*", apIP);
    server.begin();
    
    display.showSetupScreen(WIFI_AP_NAME, WIFI_AP_PASSWORD, apIP.toString());
}

void connectWiFi() {
    Serial.printf("[WIFI] Connecting to %s...\n", savedSSID.c_str());
    display.showConnecting("Verbinde mit " + savedSSID + "...");
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        apMode = false;
        Serial.printf("[WIFI] Connected: %s\n", WiFi.localIP().toString().c_str());
        
        configTime(3600, 3600, "pool.ntp.org");
        
        // Wait for NTP sync and WiFi to stabilize
        Serial.println("[WIFI] Waiting for connection to stabilize...");
        delay(2000);
        
        server.begin();
        
        if (mosqueId.length() > 0) {
            // Small delay before first API call
            delay(1000);
            fetchPrayerTimes();
        } else {
            display.showMosqueSetupScreen(WiFi.localIP().toString());
        }
    } else {
        Serial.println("[WIFI] Failed");
        startAPMode();
    }
}

void fetchPrayerTimes() {
    if (mosqueId.length() == 0) {
        Serial.println("[API] No mosque ID");
        return;
    }
    
    Serial.printf("[API] Fetching prayer times for: %s\n", mosqueId.c_str());
    Serial.printf("[API] Free heap before: %d\n", ESP.getFreeHeap());
    
    // Check if enough heap available
    if (ESP.getFreeHeap() < 50000) {
        Serial.println("[API] Not enough heap, skipping API call");
        display.showError("Speicher knapp");
        return;
    }
    
    display.showConnecting("Lade Gebetszeiten...");
    
    // Single attempt with longer timeout
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(20000);
    
    // Mawaqit API - search returns times directly!
    String url = "https://mawaqit.net/api/2.0/mosque/search?word=" + mosqueName.substring(0, 10);
    Serial.printf("[API] URL: %s\n", url.c_str());
    
    http.begin(client, url);
    http.setTimeout(20000);
    
    int code = http.GET();
    Serial.printf("[API] Response code: %d, heap after: %d\n", code, ESP.getFreeHeap());
    
    if (code == 200) {
        String payload = http.getString();
        http.end();
        client.stop();  // Explicitly close connection
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);
        
        if (err) {
            Serial.printf("[API] JSON error: %s\n", err.c_str());
            display.showError("JSON Fehler");
            return;
        }
        
        // Process response...
        JsonArray mosques = doc.as<JsonArray>();
        bool found = false;
        
        for (JsonObject mosque : mosques) {
            String uuid = mosque["uuid"].as<String>();
            if (uuid == mosqueId) {
                JsonArray times = mosque["times"].as<JsonArray>();
                if (times.size() >= 5) {
                    currentTimes.fajr = times[0].as<String>();
                    currentTimes.sunrise = times[1].as<String>();
                    currentTimes.dhuhr = times[2].as<String>();
                    currentTimes.asr = times[3].as<String>();
                        currentTimes.maghrib = times[4].as<String>();
                        currentTimes.isha = times.size() > 5 ? times[5].as<String>() : "00:00";
                        currentTimes.mosqueName = mosqueName;
                        currentTimes.valid = true;
                        timesLoaded = true;
                        found = true;
                        
                        Serial.printf("[API] OK! Fajr=%s Dhuhr=%s Asr=%s Maghrib=%s Isha=%s\n", 
                            currentTimes.fajr.c_str(), currentTimes.dhuhr.c_str(),
                            currentTimes.asr.c_str(), currentTimes.maghrib.c_str(),
                            currentTimes.isha.c_str());
                        
                        display.showPrayerList(currentTimes, -1);
                    }
                    break;
                }
            }
            
            if (!found && mosques.size() > 0) {
                JsonObject mosque = mosques[0];
                JsonArray times = mosque["times"].as<JsonArray>();
                if (times.size() >= 5) {
                    currentTimes.fajr = times[0].as<String>();
                    currentTimes.sunrise = times[1].as<String>();
                    currentTimes.dhuhr = times[2].as<String>();
                    currentTimes.asr = times[3].as<String>();
                    currentTimes.maghrib = times[4].as<String>();
                    currentTimes.isha = times.size() > 5 ? times[5].as<String>() : "00:00";
                    currentTimes.mosqueName = mosqueName;
                    currentTimes.valid = true;
                    timesLoaded = true;
                    display.showPrayerList(currentTimes, -1);
                }
            }
            
            if (!timesLoaded) {
                display.showError("Keine Zeiten");
            }
        } else {
            http.end();
            client.stop();
            Serial.printf("[API] HTTP error: %d\n", code);
            display.showError("API Fehler: " + String(code));
        }
}

void loop() {
    static unsigned long lastClockUpdate = 0;
    static unsigned long touchStartTime = 0;
    static bool waitingForRelease = false;
    static bool actionTaken = false;
    
    if (apMode) {
        dnsServer.processNextRequest();
    }
    
    server.handleClient();
    
    // Update touch state
    touch.update();
    
    // New touch started
    if (touch.justPressed()) {
        touchStartTime = millis();
        waitingForRelease = true;
        actionTaken = false;
    }
    
    // While holding - check for long press
    if (waitingForRelease && touch.isTouched() && !actionTaken) {
        unsigned long holdTime = millis() - touchStartTime;
        
        if (holdTime > 1500) {  // 1.5 seconds = long press
            actionTaken = true;
            
            if (displayMode == 3) {
                // In Settings - long press = go back to main
                displayMode = 0;
                if (timesLoaded) {
                    display.showPrayerList(currentTimes, -1);
                }
            } else if (timesLoaded) {
                // Not in settings - long press = open settings
                AdhanSettings settings;
                settings.fajr = adhanFajr;
                settings.shuruk = adhanShuruk;
                settings.dhuhr = adhanDhuhr;
                settings.asr = adhanAsr;
                settings.maghrib = adhanMaghrib;
                settings.isha = adhanIsha;
                display.showSettingsScreen(settings, -1);
                displayMode = 3;
            }
        }
    }
    
    // Finger released
    if (touch.justReleased()) {
        unsigned long holdTime = millis() - touchStartTime;
        
        // Only do short tap action if long press wasn't triggered
        if (!actionTaken && holdTime < 1500) {
            int x = touch.getX();
            int y = touch.getY();
            
            if (adhanPlaying) {
                adhanPlaying = false;
                switch (displayMode) {
                    case 0: display.showPrayerList(currentTimes, -1); break;
                    case 1: display.showClockScreen(currentTimes); break;
                    case 2: display.showNextPrayer(currentTimes); break;
                }
            } else if (displayMode == 3) {
                // In Settings - screen is 480x272
                // Left side (X < 235) = Adhan toggles
                // Right side (X >= 235) = Language, Theme, Night Mode, or back
                
                bool settingsChanged = false;
                
                if (x < 235 && y >= 55 && y < 230) {
                    // Left side - Adhan toggles (rows start at Y=55, each 28px)
                    int row = (y - 55) / 28;
                    if (row >= 0 && row <= 5) {
                        switch(row) {
                            case 0: adhanFajr = !adhanFajr; break;
                            case 1: adhanShuruk = !adhanShuruk; break;
                            case 2: adhanDhuhr = !adhanDhuhr; break;
                            case 3: adhanAsr = !adhanAsr; break;
                            case 4: adhanMaghrib = !adhanMaghrib; break;
                            case 5: adhanIsha = !adhanIsha; break;
                        }
                        settingsChanged = true;
                    }
                } else if (x >= 235) {
                    // Right side - check Y position for which setting
                    if (y >= 54 && y < 86) {
                        // Language selector (arrow area at right)
                        if (x >= 350) {
                            currentLanguage = (currentLanguage + 1) % 5;
                            settingsChanged = true;
                        }
                    } else if (y >= 86 && y < 118) {
                        // Theme selector (4 color boxes)
                        int themeIdx = (x - 240) / 30;
                        if (themeIdx >= 0 && themeIdx < 4) {
                            currentTheme = themeIdx;
                            settingsChanged = true;
                        }
                    } else if (y >= 118 && y < 150) {
                        // Auto Night Mode toggle
                        autoNightMode = !autoNightMode;
                        settingsChanged = true;
                    } else if (y >= 250) {
                        // Footer - back to main
                        displayMode = 0;
                        display.showPrayerList(currentTimes, -1);
                    }
                }
                
                if (settingsChanged) {
                    AdhanSettings settings;
                    settings.fajr = adhanFajr;
                    settings.shuruk = adhanShuruk;
                    settings.dhuhr = adhanDhuhr;
                    settings.asr = adhanAsr;
                    settings.maghrib = adhanMaghrib;
                    settings.isha = adhanIsha;
                    display.showSettingsScreen(settings, -1);
                    saveSettings();
                }
            } else if (timesLoaded) {
                displayMode = (displayMode + 1) % 3;
                switch (displayMode) {
                    case 0: display.showPrayerList(currentTimes, -1); break;
                    case 1: display.showClockScreen(currentTimes); break;
                    case 2: display.showNextPrayer(currentTimes); break;
                }
            }
        }
        
        waitingForRelease = false;
        actionTaken = false;
    }
    
    // Update clock display every 5 seconds
    if ((displayMode == 1 || displayMode == 2) && timesLoaded && (millis() - lastClockUpdate > 5000)) {
        lastClockUpdate = millis();
        if (displayMode == 1) {
            display.showClockScreen(currentTimes);
        } else if (displayMode == 2) {
            display.showNextPrayer(currentTimes);
        }
    }
    
    delay(50);
}

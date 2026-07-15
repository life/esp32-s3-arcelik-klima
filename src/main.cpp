// ================================================================
//  LG AC Remote — full climate control over WiFi (web UI)
//  Board: ESP32-S3 Mini   |   IR LED on GPIO 4
//  Protocol: LG (confirmed working on this unit)
//
//  A real AC remote transmits the ENTIRE state on every button
//  press, so we keep the full state here and re-send it each time.
// ================================================================

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRutils.h>

#define IR_LED_PIN 4

const char* WIFI_SSID     = "WIFI-SSID";
const char* WIFI_PASSWORD = "WIFI-PASSWORD";

IRac           irac(IR_LED_PIN);
AsyncWebServer server(80);

// ---- Current AC state (what the remote "remembers") ----
struct AcState {
  bool               power    = true;
  stdAc::opmode_t    mode     = stdAc::opmode_t::kCool;
  int                degrees  = 22;   // 16..30
  stdAc::fanspeed_t  fan      = stdAc::fanspeed_t::kAuto;
  stdAc::swingv_t    swingv   = stdAc::swingv_t::kOff;
  bool               light    = true;
} ac;

String lastMsg = "hazir";

// ---- Helpers: string <-> enum ----
stdAc::opmode_t modeFromStr(const String& s) {
  if (s == "cool") return stdAc::opmode_t::kCool;
  if (s == "heat") return stdAc::opmode_t::kHeat;
  if (s == "dry")  return stdAc::opmode_t::kDry;
  if (s == "fan")  return stdAc::opmode_t::kFan;
  if (s == "auto") return stdAc::opmode_t::kAuto;
  return ac.mode;
}
const char* modeToStr(stdAc::opmode_t m) {
  switch (m) {
    case stdAc::opmode_t::kCool: return "cool";
    case stdAc::opmode_t::kHeat: return "heat";
    case stdAc::opmode_t::kDry:  return "dry";
    case stdAc::opmode_t::kFan:  return "fan";
    case stdAc::opmode_t::kAuto: return "auto";
    default: return "cool";
  }
}
stdAc::fanspeed_t fanFromStr(const String& s) {
  if (s == "auto")   return stdAc::fanspeed_t::kAuto;
  if (s == "low")    return stdAc::fanspeed_t::kLow;
  if (s == "medium") return stdAc::fanspeed_t::kMedium;
  if (s == "high")   return stdAc::fanspeed_t::kHigh;
  return ac.fan;
}
const char* fanToStr(stdAc::fanspeed_t f) {
  switch (f) {
    case stdAc::fanspeed_t::kAuto:   return "auto";
    case stdAc::fanspeed_t::kLow:    return "low";
    case stdAc::fanspeed_t::kMedium: return "medium";
    case stdAc::fanspeed_t::kHigh:   return "high";
    default: return "auto";
  }
}

// ---- Transmit the whole current state to the AC ----
void sendState() {
  irac.next.protocol = decode_type_t::LG;
  irac.next.model    = 1;
  irac.next.celsius  = true;
  irac.next.power    = ac.power;
  irac.next.mode     = ac.mode;
  irac.next.degrees  = ac.degrees;
  irac.next.fanspeed = ac.fan;
  irac.next.swingv   = ac.swingv;
  irac.next.swingh   = stdAc::swingh_t::kOff;
  irac.next.light    = ac.light;
  irac.next.beep     = true;
  irac.next.econo    = false;
  irac.next.turbo    = false;
  irac.next.quiet    = false;
  irac.next.filter   = false;
  irac.next.clean    = false;
  irac.next.sleep    = -1;
  irac.next.clock    = -1;
  irac.sendAc();

  lastMsg = String(ac.power ? "ACIK" : "KAPALI") + " | " +
            modeToStr(ac.mode) + " | " + String(ac.degrees) + "C | fan:" +
            fanToStr(ac.fan) + " | salinim:" +
            (ac.swingv == stdAc::swingv_t::kOff ? "off" : "on");
  Serial.println("[TX] " + lastMsg);
}

// ---- JSON snapshot of state for the UI ----
String stateJson() {
  String s = "{";
  s += "\"power\":"  + String(ac.power ? "true" : "false");
  s += ",\"mode\":\"" + String(modeToStr(ac.mode)) + "\"";
  s += ",\"temp\":"  + String(ac.degrees);
  s += ",\"fan\":\"" + String(fanToStr(ac.fan)) + "\"";
  s += ",\"swing\":" + String(ac.swingv == stdAc::swingv_t::kOff ? "false" : "true");
  s += ",\"light\":" + String(ac.light ? "true" : "false");
  s += ",\"msg\":\"" + lastMsg + "\"";
  s += "}";
  return s;
}

const char PAGE[] PROGMEM = R"raw(
<!DOCTYPE html><html lang="tr"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>LG Klima Kumanda</title><style>
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
body{font-family:-apple-system,BlinkMacSystemFont,sans-serif;background:#0d0d12;
     color:#e2e2e2;padding:16px;max-width:460px;margin:0 auto}
h1{font-size:18px;font-weight:600;color:#fff;margin-bottom:16px;text-align:center}
.card{background:#16161f;border:1px solid #2a2a3a;border-radius:14px;padding:16px;margin-bottom:12px}
.lbl{font-size:11px;text-transform:uppercase;letter-spacing:.07em;color:#666;margin-bottom:10px}
.pwr{width:100%;padding:16px;border:none;border-radius:12px;font-size:16px;font-weight:700;color:#fff;cursor:pointer}
.pwr.on{background:#16a34a}.pwr.off{background:#dc2626}
.temp{display:flex;align-items:center;justify-content:space-between;gap:12px}
.temp button{width:64px;height:64px;border:none;border-radius:50%;background:#2a2a3a;color:#fff;font-size:28px;cursor:pointer}
.temp button:active{opacity:.6}
.tval{font-size:44px;font-weight:700;color:#fff}
.tval small{font-size:20px;color:#888}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(72px,1fr));gap:8px}
.seg{padding:12px 6px;border:1px solid #2a2a3a;border-radius:10px;background:#1e1e2e;color:#bbb;
     font-size:13px;font-weight:600;cursor:pointer;text-align:center}
.seg.sel{background:#2563eb;border-color:#2563eb;color:#fff}
.toggle{display:flex;gap:8px}
.toggle .seg{flex:1}
.st{font-size:12px;color:#4ade80;text-align:center;margin-top:6px;min-height:16px}
.dim{opacity:.4;pointer-events:none}
</style></head><body>
<h1>LG Klima Kumanda</h1>

<div class="card">
  <button id="pwr" class="pwr" onclick="togglePower()">GUC</button>
</div>

<div id="controls">
<div class="card">
  <div class="lbl">Sicaklik</div>
  <div class="temp">
    <button onclick="chgTemp(-1)">&minus;</button>
    <div class="tval"><span id="temp">22</span><small>&deg;C</small></div>
    <button onclick="chgTemp(1)">+</button>
  </div>
</div>

<div class="card">
  <div class="lbl">Mod</div>
  <div class="grid" id="mode">
    <div class="seg" data-v="cool"   onclick="setMode('cool')">Sogutma</div>
    <div class="seg" data-v="heat"   onclick="setMode('heat')">Isitma</div>
    <div class="seg" data-v="dry"    onclick="setMode('dry')">Nem</div>
    <div class="seg" data-v="fan"    onclick="setMode('fan')">Fan</div>
    <div class="seg" data-v="auto"   onclick="setMode('auto')">Oto</div>
  </div>
</div>

<div class="card">
  <div class="lbl">Fan Hizi</div>
  <div class="grid" id="fan">
    <div class="seg" data-v="auto"   onclick="setFan('auto')">Oto</div>
    <div class="seg" data-v="low"    onclick="setFan('low')">Dusuk</div>
    <div class="seg" data-v="medium" onclick="setFan('medium')">Orta</div>
    <div class="seg" data-v="high"   onclick="setFan('high')">Yuksek</div>
  </div>
</div>

<div class="card">
  <div class="lbl">Salinim</div>
  <div class="toggle" id="swing">
    <div class="seg" data-v="0" onclick="setSwing(0)">Kapali</div>
    <div class="seg" data-v="1" onclick="setSwing(1)">Acik</div>
  </div>
</div>
</div>

<div class="st" id="st">baglaniyor...</div>

<script>
let S={power:true,mode:'cool',temp:22,fan:'auto',swing:false,light:true};

function apply(){
  const p=document.getElementById('pwr');
  p.textContent = S.power ? 'ACIK' : 'KAPALI';
  p.className = 'pwr ' + (S.power?'on':'off');
  document.getElementById('controls').className = S.power ? '' : 'dim';
  document.getElementById('temp').textContent = S.temp;
  sel('mode', S.mode); sel('fan', S.fan);
  sel('swing', S.swing ? '1':'0');
}
function sel(group,val){
  document.querySelectorAll('#'+group+' .seg').forEach(e=>{
    e.classList.toggle('sel', e.dataset.v==String(val));
  });
}
function push(){
  const q='power='+(S.power?1:0)+'&mode='+S.mode+'&temp='+S.temp+
          '&fan='+S.fan+'&swing='+(S.swing?1:0);
  fetch('/set?'+q).then(r=>r.json()).then(d=>{S=d;apply();
    document.getElementById('st').textContent=d.msg;});
}
function togglePower(){S.power=!S.power;push();}
function chgTemp(d){S.temp=Math.max(16,Math.min(30,S.temp+d));apply();push();}
function setMode(m){S.mode=m;apply();push();}
function setFan(f){S.fan=f;apply();push();}
function setSwing(v){S.swing=!!v;apply();push();}

fetch('/state').then(r=>r.json()).then(d=>{S=d;apply();
  document.getElementById('st').textContent=d.msg;});
</script>
</body></html>
)raw";

// ---- Read params, update state, transmit ----
void handleSet(AsyncWebServerRequest* r) {
  if (r->hasParam("power")) ac.power = (r->getParam("power")->value() == "1");
  if (r->hasParam("mode"))  ac.mode  = modeFromStr(r->getParam("mode")->value());
  if (r->hasParam("temp")) {
    int t = r->getParam("temp")->value().toInt();
    if (t < 16) t = 16;
    if (t > 30) t = 30;
    ac.degrees = t;
  }
  if (r->hasParam("fan"))   ac.fan = fanFromStr(r->getParam("fan")->value());
  if (r->hasParam("swing")) {
    ac.swingv = (r->getParam("swing")->value() == "1")
                  ? stdAc::swingv_t::kAuto : stdAc::swingv_t::kOff;
  }
  sendState();
  r->send(200, "application/json", stateJson());
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n[BOOT] LG AC Remote");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 40) { delay(500); Serial.print("."); t++; }
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("\n[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
  else
    Serial.println("\n[WiFi] FAILED");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* r){
    r->send(200, "text/html", FPSTR(PAGE));
  });

  server.on("/state", HTTP_GET, [](AsyncWebServerRequest* r){
    r->send(200, "application/json", stateJson());
  });

  server.on("/set", HTTP_GET, [](AsyncWebServerRequest* r){
    handleSet(r);
  });

  server.begin();
  Serial.printf("[Server] http://%s\n", WiFi.localIP().toString().c_str());
}

void loop() {}

// Alpha Node firmware
#include <WiFi.h>
#include <WebServer.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <nvs_flash.h>

#define PIN_SS   5
#define PIN_RST  14
#define PIN_DIO0 2
#define LORA_FREQ 433E6

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ADDR 0x3C

const char* txSsid = "Romeo";
const char* txPassword = "Sierra";

WebServer txServer(80);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

struct Message {
  String text;
  bool ackReceived;
  bool isReceived;
};
Message messages[10];
int msgIndex = 0;
int lastRssi = 0;
bool oledAvailable = false;

void updateOLED() {
  if (!oledAvailable) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("ALPHA // TWO-WAY");
  display.drawLine(0,10,128,10,WHITE);
  display.setCursor(0,15);
  display.printf("Total: %d | RSSI: %d\n", msgIndex, lastRssi);
  display.drawLine(0,25,128,25,WHITE);
  
  int y = 30;
  for (int i = max(0, msgIndex - 3); i < msgIndex; i++) {
    display.setCursor(0, y);
    char prefix = messages[i].isReceived ? 'R' : 'S';
    char status = messages[i].ackReceived ? 'OK' : '..';
    display.printf("[%c,%c] %s\n", prefix, status, messages[i].text.substring(0, 12).c_str());
    y += 10;
  }
  display.display();
}

// ═══════════════════════════════════════════════════════════
// TWO-WAY MOBILE-OPTIMIZED UI
// ═══════════════════════════════════════════════════════════
const char WEB_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
<title>ALPHA // TWO-WAY</title>
<style>
  :root {
    --bg: #060305;
    --surface: #150a11;
    --surface-border: #2c1623;
    --crimson: #ff2a54;
    --crimson-glow: rgba(255, 42, 84, 0.4);
    --orange: #ff7300;
    --text-main: #fdf5f7;
    --text-dim: #9e7f89;
  }
  
  * { box-sizing: border-box; margin: 0; padding: 0; }
  
  body {
    background-color: var(--bg);
    color: var(--text-main);
    font-family: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    display: flex;
    flex-direction: column;
    height: 100vh;
    height: -webkit-fill-available;
    overflow: hidden;
  }

  .header {
    background: linear-gradient(180deg, #1b0a13 0%, var(--bg) 100%);
    border-bottom: 1px solid var(--surface-border);
    padding: 14px 16px;
    flex-shrink: 0;
  }

  .title {
    font-size: 20px; font-weight: 900; color: var(--crimson);
    letter-spacing: 1.5px; text-shadow: 0 0 10px var(--crimson-glow);
  }
  .subtitle { font-size: 10px; color: var(--text-dim); margin-top: 2px; }

  .metrics {
    display: flex; gap: 8px; padding: 10px 16px;
    background: var(--surface); border-bottom: 1px solid var(--surface-border);
    flex-shrink: 0;
  }
  .metric-card {
    flex: 1; background: rgba(5, 2, 4, 0.6);
    border: 1px solid var(--surface-border); border-radius: 6px;
    padding: 6px 10px;
  }
  .metric-lbl { font-size: 9px; color: var(--text-dim); font-weight: 700; }
  .metric-val { font-size: 14px; font-weight: 800; color: #fff; margin-top: 2px; }
  
  .status-dot {
    width: 6px; height: 6px; border-radius: 50%; background: var(--orange);
    box-shadow: 0 0 8px var(--orange); animation: pulse 2s infinite;
    display: inline-block; margin-right: 4px;
  }
  @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.3; } }

  .chat-viewport {
    flex: 1; overflow-y: auto; padding: 12px 16px;
    display: flex; flex-direction: column; gap: 10px;
    -webkit-overflow-scrolling: touch;
  }

  .msg-container { display: flex; flex-direction: column; max-width: 85%; animation: fadeIn 0.3s ease-out; }
  @keyframes fadeIn { from { opacity: 0; transform: translateY(8px); } to { opacity: 1; transform: translateY(0); } }
  
  .msg-container.rx { align-self: flex-start; }
  .msg-container.tx { align-self: flex-end; align-items: flex-end; }
  .msg-container.sys { align-self: center; max-width: 90%; }

  .bubble {
    padding: 10px 14px; border-radius: 12px; font-size: 14px;
    line-height: 1.4; word-break: break-word; font-weight: 500;
    box-shadow: 0 4px 12px rgba(0,0,0,0.2);
  }

  .rx .bubble {
    background: var(--surface); border: 1px solid var(--surface-border);
    border-left: 4px solid var(--crimson); border-bottom-left-radius: 4px;
  }
  .tx .bubble {
    background: linear-gradient(135deg, #1a0c12 0%, #0f060a 100%);
    border: 1px solid rgba(255, 42, 84, 0.3); border-right: 4px solid var(--orange);
    border-bottom-right-radius: 4px; color: var(--crimson);
  }
  .sys .bubble {
    background: rgba(255, 115, 0, 0.1); border: 1px solid rgba(255, 115, 0, 0.2);
    border-radius: 20px; padding: 4px 12px; font-size: 11px; color: var(--orange);
  }

  .meta { font-size: 10px; color: var(--text-dim); margin-top: 2px; padding: 0 4px; }

  .controller {
    background: #080306; border-top: 1px solid var(--surface-border);
    padding: 10px 16px; padding-bottom: calc(10px + env(safe-area-inset-bottom));
    display: flex; gap: 8px; flex-shrink: 0;
  }

  .input-core {
    flex: 1; background: #030102; border: 1px solid var(--surface-border);
    border-radius: 8px; padding: 0 12px; color: #fff;
    font-size: 16px; height: 42px; outline: none; transition: border 0.2s;
  }
  .input-core:focus { border-color: var(--crimson); box-shadow: 0 0 10px var(--crimson-glow); }

  .send-core {
    background: linear-gradient(135deg, var(--crimson) 0%, var(--orange) 100%);
    color: #fff; font-size: 13px; font-weight: 800; border: none;
    border-radius: 8px; padding: 0 16px; height: 42px;
    cursor: pointer; display: flex; align-items: center;
    box-shadow: 0 4px 15px var(--crimson-glow);
  }
  .send-core:active { opacity: 0.7; }
  .send-core:disabled { opacity: 0.4; }

  .empty { text-align: center; color: var(--text-dim); padding: 20px; }
</style>
</head>
<body>

<div class="header">
  <div class="title">🔴 ALPHA</div>
  <div class="subtitle">Two-Way Link Active</div>
</div>

<div class="metrics">
  <div class="metric-card">
    <div class="metric-lbl">Link</div>
    <div class="metric-val"><span class="status-dot" id="dot"></span><span id="st">OK</span></div>
  </div>
  <div class="metric-card">
    <div class="metric-lbl">Messages</div>
    <div class="metric-val" id="mc">0</div>
  </div>
  <div class="metric-card">
    <div class="metric-lbl">Signal</div>
    <div class="metric-val" id="rs">--</div>
  </div>
</div>

<div class="chat-viewport" id="chat"><div class="empty">Waiting for messages...</div></div>

<div class="controller">
  <input type="text" id="inp" class="input-core" placeholder="Type message..." maxlength="200" autocomplete="off">
  <button onclick="send()" class="send-core" id="btn">SEND</button>
</div>

<script>
let lastC = 0; let busy = false;

function send() {
  if (busy) return;
  const f = document.getElementById("inp"); const v = f.value.trim(); if (!v) return;
  busy = true; document.getElementById("btn").disabled = true;
  fetch("/send?msg=" + encodeURIComponent(v)).then(() => { f.value = ""; update(); }).finally(() => { busy = false; document.getElementById("btn").disabled = false; });
}
document.getElementById("inp").onkeydown = e => { if (e.key === "Enter") send(); };

function update() {
  fetch("/status").then(r => r.json()).then(d => {
    document.getElementById("mc").innerText = d.msgCount;
    document.getElementById("rs").innerText = d.rssi ? d.rssi + "dBm" : "--";
    document.getElementById("st").innerText = "OK";
    document.getElementById("dot").style.background = "var(--orange)";
    if (d.msgCount !== lastC) { lastC = d.msgCount; render(d.messages); }
  }).catch(() => { document.getElementById("st").innerText = "ERROR"; document.getElementById("dot").style.background = "#ff0033"; });
}

function render(arr) {
  const v = document.getElementById("chat");
  if (arr.length === 0) { v.innerHTML = '<div class="empty">No messages yet</div>'; return; }
  v.innerHTML = "";
  arr.forEach(m => {
    const d = document.createElement("div");
    if (m.ackReceived) {
      d.className = "msg-container sys";
      d.innerHTML = `<div class="bubble">✓ Delivered</div>`;
    } else if (m.isReceived) {
      d.className = "msg-container rx";
      d.innerHTML = `<div class="bubble">${m.text}</div>`;
    } else {
      d.className = "msg-container tx";
      d.innerHTML = `<div class="bubble">${m.text}</div>`;
    }
    v.appendChild(d);
  });
  v.scrollTo({ top: v.scrollHeight, behavior: 'smooth' });
}

setInterval(update, 1000); update();
</script>
</body>
</html>
)=====";

void sendMessage(String msg) {
  Serial.printf("[ALPHA] SENDING: %s\n", msg.c_str());
  LoRa.beginPacket();
  LoRa.print("ALPHA:" + msg);
  LoRa.endPacket();
  
  if (msgIndex >= 10) {
    for (int i = 0; i < 9; i++) messages[i] = messages[i+1];
    msgIndex = 9;
  }
  messages[msgIndex].text = msg;
  messages[msgIndex].ackReceived = false;
  messages[msgIndex].isReceived = false;
  msgIndex++;
  
  unsigned long st = millis();
  while (millis() - st < 2500) {
    if (LoRa.parsePacket()) {
      String a = "";
      while (LoRa.available()) a += (char)LoRa.read();
      if (a == "ACK:ALPHA:" + msg) {
        messages[msgIndex-1].ackReceived = true;
        lastRssi = LoRa.packetRssi();
        Serial.println("[ALPHA] ACK received");
        break;
      }
    }
    delay(10);
  }
  updateOLED();
}

void handleRoot() { txServer.send_P(200, "text/html", WEB_PAGE); }

void handleSend() {
  if (txServer.hasArg("msg")) {
    String m = txServer.arg("msg");
    m.trim();
    if (m.length() > 0) {
      sendMessage(m);
      txServer.send(200, "text/plain", "OK");
      return;
    }
  }
  txServer.send(400, "text/plain", "ERR");
}

void handleStatus() {
  String j = "{\"msgCount\":" + String(msgIndex) + ",\"rssi\":" + String(lastRssi) + ",\"messages\":[";
  for (int i = 0; i < msgIndex; i++) {
    if (i > 0) j += ",";
    String t = messages[i].text;
    t.replace("\\","\\\\");
    t.replace("\"","\\\"");
    j += "{\"text\":\"" + t + "\",\"ackReceived\":" + String(messages[i].ackReceived ? "true" : "false") + ",\"isReceived\":" + String(messages[i].isReceived ? "true" : "false") + "}";
  }
  j += "]}";
  txServer.send(200, "application/json", j);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) oledAvailable = true;
  
  nvs_flash_erase();
  nvs_flash_init();
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200);

  Serial.println("\n[ALPHA] Compile: " __DATE__ " " __TIME__);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(txSsid, txPassword, 1, 0, 4);

  LoRa.setPins(PIN_SS, PIN_RST, PIN_DIO0);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("[ERROR] LoRa failed");
    while (1);
  }
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(500E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0xA5);
  LoRa.enableCrc();

  txServer.on("/", handleRoot);
  txServer.on("/send", handleSend);
  txServer.on("/status", handleStatus);
  txServer.begin();

  if (oledAvailable) updateOLED();

  Serial.println("[ALPHA] READY - SSID: Romeo / PASS: Sierra");
}

void loop() {
  txServer.handleClient();
  
  int sz = LoRa.parsePacket();
  if (sz) {
    String m = "";
    while (LoRa.available()) m += (char)LoRa.read();
    int r = LoRa.packetRssi();
    
    Serial.printf("[ALPHA] RECEIVED: %s\n", m.c_str());
    
    // Message FROM DELTA
    if (m.startsWith("DELTA:")) {
      String text = m.substring(6);
      
      // Add to message list
      if (msgIndex >= 10) {
        for (int i = 0; i < 9; i++) messages[i] = messages[i+1];
        msgIndex = 9;
      }
      messages[msgIndex].text = text;
      messages[msgIndex].ackReceived = false;
      messages[msgIndex].isReceived = true;
      msgIndex++;
      
      lastRssi = r;
      
      // Send ACK
      delay(150);
      LoRa.beginPacket();
      LoRa.print("ACK:DELTA:" + text);
      LoRa.endPacket();
      Serial.println("[ALPHA] ACK sent");
      updateOLED();
    }
  }
}

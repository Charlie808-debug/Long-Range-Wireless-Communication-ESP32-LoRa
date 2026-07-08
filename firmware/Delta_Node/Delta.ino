// Delta Node firmware
#include <WiFi.h>
  #include <WebServer.h>
  #include <LoRa.h>
  #include <nvs_flash.h>

  #define PIN_SS   5
  #define PIN_RST  14
  #define PIN_DIO0 2
  #define LORA_FREQ 433E6

  const char* ssid = "Sierra";
  const char* password = "Romeo";

  WebServer server(80);

  struct Message {
    String text;
    int rssi;
    float snr;
    String time;
    bool isAck;
    bool isSent;
  };

  Message messages[50];
  int msgCount = 0;

  String escapeJson(String s) {
    s.replace("\\", "\\\\");
    s.replace("\"", "\\\"");
    s.replace("\n", "\\n");
    s.replace("\r", "\\r");
    return s;
  }

  String getUptime() {
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    seconds %= 60;
    minutes %= 60;
    char buf[20];
    sprintf(buf, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return String(buf);
  }

  void addMessage(String text, int rssi, float snr, bool isAck, bool isSent) {
    if (msgCount >= 50) {
      for (int i = 0; i < 49; i++) messages[i] = messages[i + 1];
      msgCount = 49;
    }
    messages[msgCount].text = text;
    messages[msgCount].rssi = rssi;
    messages[msgCount].snr = snr;
    messages[msgCount].time = getUptime();
    messages[msgCount].isAck = isAck;
    messages[msgCount].isSent = isSent;
    msgCount++;
    Serial.printf("[DELTA] +MSG: %s (Total: %d)\n", text.c_str(), msgCount);
  }

  // ═══════════════════════════════════════════════════════════
  // TWO-WAY MOBILE-OPTIMIZED UI
  // ═══════════════════════════════════════════════════════════
  const char WEBPAGE[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html lang="en">
  <head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <title>DELTA // TWO-WAY</title>
  <style>
    :root {
      --bg: #030509;
      --surface: #0a0f1d;
      --surface-border: #131d38;
      --cyan: #00f3ff;
      --cyan-glow: rgba(0, 243, 255, 0.4);
      --green: #00ffaa;
      --text-main: #f0f6fc;
      --text-dim: #798da8;
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
      background: linear-gradient(180deg, #091021 0%, var(--bg) 100%);
      border-bottom: 1px solid var(--surface-border);
      padding: 14px 16px;
      flex-shrink: 0;
    }

    .title {
      font-size: 20px; font-weight: 900; color: var(--cyan);
      letter-spacing: 1.5px; text-shadow: 0 0 10px var(--cyan-glow);
    }
    .subtitle { font-size: 10px; color: var(--text-dim); margin-top: 2px; }

    .metrics {
      display: flex; gap: 8px; padding: 10px 16px;
      background: var(--surface); border-bottom: 1px solid var(--surface-border);
      flex-shrink: 0;
    }
    .metric-card {
      flex: 1; background: rgba(3, 5, 9, 0.6);
      border: 1px solid var(--surface-border); border-radius: 6px;
      padding: 6px 10px;
    }
    .metric-lbl { font-size: 9px; color: var(--text-dim); font-weight: 700; }
    .metric-val { font-size: 14px; font-weight: 800; color: #fff; margin-top: 2px; }
    
    .status-dot {
      width: 6px; height: 6px; border-radius: 50%; background: var(--green);
      box-shadow: 0 0 8px var(--green); animation: pulse 2s infinite;
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
      border-left: 4px solid var(--cyan); border-bottom-left-radius: 4px;
    }
    .tx .bubble {
      background: linear-gradient(135deg, #091a2f 0%, #051321 100%);
      border: 1px solid rgba(0, 243, 255, 0.3); border-right: 4px solid var(--green);
      border-bottom-right-radius: 4px; color: var(--cyan);
    }
    .sys .bubble {
      background: rgba(0, 255, 170, 0.1); border: 1px solid rgba(0, 255, 170, 0.2);
      border-radius: 20px; padding: 4px 12px; font-size: 11px; color: var(--green);
    }

    .meta { font-size: 10px; color: var(--text-dim); margin-top: 2px; padding: 0 4px; }

    .controller {
      background: #050a14; border-top: 1px solid var(--surface-border);
      padding: 10px 16px; padding-bottom: calc(10px + env(safe-area-inset-bottom));
      display: flex; gap: 8px; flex-shrink: 0;
    }

    .input-core {
      flex: 1; background: #020408; border: 1px solid var(--surface-border);
      border-radius: 8px; padding: 0 12px; color: #fff;
      font-size: 16px; height: 42px; outline: none; transition: border 0.2s;
    }
    .input-core:focus { border-color: var(--cyan); box-shadow: 0 0 10px var(--cyan-glow); }

    .send-core {
      background: linear-gradient(135deg, var(--cyan) 0%, var(--green) 100%);
      color: #000; font-size: 13px; font-weight: 800; border: none;
      border-radius: 8px; padding: 0 16px; height: 42px;
      cursor: pointer; display: flex; align-items: center;
      box-shadow: 0 4px 15px var(--cyan-glow);
    }
    .send-core:active { opacity: 0.7; }
    .send-core:disabled { opacity: 0.4; }

    .empty { text-align: center; color: var(--text-dim); padding: 20px; }
  </style>
  </head>
  <body>

  <div class="header">
    <div class="title">🔵 DELTA</div>
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
    fetch("/data").then(r => r.json()).then(d => {
      document.getElementById("mc").innerText = d.msgCount;
      document.getElementById("rs").innerText = d.lastRssi ? d.lastRssi + "dBm" : "--";
      document.getElementById("st").innerText = "OK";
      document.getElementById("dot").style.background = "var(--green)";
      if (d.msgCount !== lastC) { lastC = d.msgCount; render(d.messages); }
    }).catch(() => { document.getElementById("st").innerText = "ERROR"; document.getElementById("dot").style.background = "#ff2a54"; });
  }

  function render(arr) {
    const v = document.getElementById("chat");
    if (arr.length === 0) { v.innerHTML = '<div class="empty">No messages yet</div>'; return; }
    v.innerHTML = "";
    arr.forEach(m => {
      const d = document.createElement("div");
      if (m.isAck) {
        d.className = "msg-container sys";
        d.innerHTML = `<div class="bubble">✓ Confirmed</div>`;
      } else if (m.isSent) {
        d.className = "msg-container tx";
        d.innerHTML = `<div class="bubble">${m.text}</div>`;
      } else {
        d.className = "msg-container rx";
        d.innerHTML = `<div class="bubble">${m.text}</div><div class="meta">📶 ${m.rssi}dBm</div>`;
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

  void handleRoot() { server.send_P(200, "text/html", WEBPAGE); }

  void handleSend() {
    if (server.hasArg("msg")) {
      String m = server.arg("msg");
      m.trim();
      if (m.length() > 0) {
        Serial.printf("[DELTA] SENDING: %s\n", m.c_str());
        LoRa.beginPacket();
        LoRa.print("DELTA:" + m);
        LoRa.endPacket();
        addMessage(m, 0, 0, false, true);
        server.send(200, "text/plain", "OK");
        return;
      }
    }
    server.send(400, "text/plain", "ERR");
  }

  void handleData() {
    String j = "{\"msgCount\":" + String(msgCount) + ",\"lastRssi\":";
    int r = 0;
    for (int i = msgCount - 1; i >= 0; i--) {
      if (!messages[i].isSent && !messages[i].isAck) { r = messages[i].rssi; break; }
    }
    j += String(r) + ",\"messages\":[";
    for (int i = 0; i < msgCount; i++) {
      if (i > 0) j += ",";
      j += "{\"text\":\"" + escapeJson(messages[i].text) + "\",\"rssi\":" + String(messages[i].rssi) + ",\"isAck\":" + String(messages[i].isAck ? "true" : "false") + ",\"isSent\":" + String(messages[i].isSent ? "true" : "false") + "}";
    }
    j += "]}";
    server.send(200, "application/json", j);
  }

  void setup() {
    Serial.begin(115200);
    delay(1000);
    
    nvs_flash_erase();
    nvs_flash_init();
    WiFi.persistent(false);
    WiFi.disconnect(true, true);
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(200);

    Serial.println("\n[DELTA] Compile: " __DATE__ " " __TIME__);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password, 1, 0, 4);

    server.on("/", handleRoot);
    server.on("/send", handleSend);
    server.on("/data", handleData);
    server.begin();

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

    Serial.println("[DELTA] READY - SSID: Sierra / PASS: Romeo");
  }

  void loop() {
    server.handleClient();
    
    int sz = LoRa.parsePacket();
    if (sz) {
      String m = "";
      while (LoRa.available()) m += (char)LoRa.read();
      int r = LoRa.packetRssi();
      float s = LoRa.packetSnr();
      
      Serial.printf("[DELTA] RECEIVED: %s\n", m.c_str());
      
      // Message FROM ALPHA
      if (m.startsWith("ALPHA:")) {
        String text = m.substring(6);
        addMessage(text, r, s, false, false);
        
        // Send ACK
        delay(150);
        LoRa.beginPacket();
        LoRa.print("ACK:ALPHA:" + text);
        LoRa.endPacket();
        Serial.println("[DELTA] ACK sent");
      }
      // ACK for our message
      else if (m.startsWith("ACK:DELTA:")) {
        addMessage("", r, s, true, false);
      }
    }
  }

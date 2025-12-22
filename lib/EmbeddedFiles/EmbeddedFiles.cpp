#include "EmbeddedFiles.h"

#ifdef CHAIR_MASTER
// Define the HTML content
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP32 Serial Monitor</title>
  <link rel="stylesheet" type="text/css" href="/style.css">
  <script src="/script.js"></script>
</head>
<body>
  <h2>ESP32 Wireless Serial Monitor</h2>
  <div id="serial"></div>
  <button onclick="triggerReconnect()">Reconnect to Slave</button>
  <script>
    function triggerReconnect() {
      fetch('/trigger-reconnect')
        .then(response => {
          if (response.ok) {
            return response.text();
          } else {
            throw new Error('Failed to reconnect to slave');
          }
        })
        .then(message => alert(message))
        .catch(error => alert(error.message));
    }
  </script>
</body>
</html>
)rawliteral";

// Define the CSS content
const char style_css[] PROGMEM = R"rawliteral(
:root {
    --background-color: #121212;
    --serial-background-color: #1e1e1e;
    --blue: #0b4af9;
    --pink: #ed006c;
}

body {
    font-family: "Courier New", Courier, monospace;
    background-color: var(--background-color);
    color: var(--blue);
    margin: 0;
    padding: 24px; 
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    height: 100vh;
    box-sizing: border-box; 
}

h2 {
    color: var(--blue); 
    margin-bottom: 24px;
    text-align: center;
}

#serial {
    width: 80%; 

    height: 80%; 
    overflow-y: auto; 
    border: 1px solid var(--pink);
    padding: 12px;
    background-color: var(--serial-background-color);
    color: white;
    box-shadow: 0px 0px 12px black;
    box-sizing: border-box; 
}

::-webkit-scrollbar {
    width: 12px;
}

::-webkit-scrollbar-thumb {
    background: var(--blue);
}

::-webkit-scrollbar-thumb:hover {
    background: var(--pink);
}
)rawliteral";

// Define the JavaScript content
const char script_js[] PROGMEM = R"rawliteral(
var gateway = `ws://${window.location.hostname}:81/`;
var websocket;
var userScrolled = false;

window.addEventListener("load", onLoad);

function onLoad(event) {
    initWebSocket();
    var serialDiv = document.getElementById("serial");
    serialDiv.addEventListener('scroll', function() {
        if (serialDiv.scrollTop + serialDiv.clientHeight < serialDiv.scrollHeight) {
            userScrolled = true;
        } else {
            userScrolled = false;
        }
    });
}

function initWebSocket() {
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log("Connection opened");
}

function onClose(event) {
    console.log("Connection closed");
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    console.log(event.data);
    var serialDiv = document.getElementById("serial");
    serialDiv.innerHTML += event.data + "<br>";
    if (!userScrolled) {
        serialDiv.scrollTop = serialDiv.scrollHeight; // Auto-scroll to the bottom
    }
}

)rawliteral";
const char captive_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Robotic Bar Stools Interface</title>
  <style>
    :root {
      --bg: #0f1115;
      --panel: #1b1f27;
      --accent: #ff8a00;
      --text: #f4f4f4;
      --muted: #8a93a6;
    }
    * {
      box-sizing: border-box;
    }
    body {
      margin: 0;
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      background: var(--bg);
      font-family: "Courier New", Courier, monospace;
      color: var(--text);
      padding: 24px;
    }
    .card {
      width: min(480px, 100%);
      background: var(--panel);
      border-radius: 16px;
      padding: 32px;
      box-shadow: 0 16px 40px rgba(0, 0, 0, 0.45);
    }
    h1 {
      margin-top: 0;
      font-size: 1.8rem;
      color: var(--accent);
    }
    p {
      line-height: 1.5;
      color: var(--muted);
    }
    .instructions {
      margin: 20px 0;
      padding: 16px;
      border: 1px dashed rgba(255, 255, 255, 0.2);
      border-radius: 12px;
    }
    label {
      display: block;
      margin-bottom: 8px;
      font-weight: bold;
    }
    input, select {
      width: 100%;
      padding: 10px 14px;
      margin-bottom: 16px;
      border-radius: 8px;
      border: 1px solid rgba(255, 255, 255, 0.1);
      background: #0f131a;
      color: var(--text);
      font-size: 1rem;
    }
    small {
      display: block;
      margin-top: -12px;
      margin-bottom: 18px;
      color: var(--muted);
    }
    code {
      display: block;
      background: #0f131a;
      padding: 10px;
      border-radius: 8px;
      margin-top: 8px;
      color: #79ffe1;
      font-size: 0.95rem;
    }
    .cta {
      margin-top: 12px;
      display: inline-block;
      color: var(--accent);
      text-decoration: none;
      font-weight: bold;
    }
    .status-indicator {
      display: flex;
      align-items: center;
      margin-bottom: 20px;
      padding: 12px;
      border-radius: 8px;
      background: rgba(255, 255, 255, 0.05);
    }
    .status-dot {
      width: 12px;
      height: 12px;
      border-radius: 50%;
      margin-right: 10px;
      animation: pulse 2s infinite;
    }
    .status-connected {
      background: #4ade80;
    }
    .status-disconnected {
      background: #f87171;
    }
    @keyframes pulse {
      0% { opacity: 1; }
      50% { opacity: 0.5; }
      100% { opacity: 1; }
    }
  </style>
</head>
<body>
  <div class="card">
    <h1>Welcome to the Robotic Bar Stools Interface</h1>
    
    <div class="status-indicator" id="slaveStatus">
      <div class="status-dot status-disconnected" id="statusDot"></div>
      <span id="statusText">Checking slave connection...</span>
    </div>
    
    <p>In order to control the speed and the rotation of the chair, change the value in the control link.</p>

    <div class="instructions">
      <strong>Speed (1-10)</strong>
      <p>Speed 1: 90° rotation takes 20 seconds (slow)<br/>
         Speed 10: 90° rotation takes 5 seconds (fast)</p>
      <strong>Rotation</strong>
      <p>Choose the angle in degrees and direction:<br />
         Clockwise rotates one way, Counter-Clockwise rotates the opposite way.</p>
    </div>

    <label for="chair">Chair Selection</label>
    <select id="chair">
      <option value="master">Master Chair (This Chair)</option>
      <option value="slave">Slave Chair</option>
      <option value="both">Both Chairs</option>
    </select>
    <small>Choose which chair to control</small>

    <label for="speed">Speed (1-10)</label>
    <input id="speed" type="number" min="1" max="10" value="5" placeholder="Enter value 1-10" />
    <small>1 = slowest, 10 = fastest</small>

    <label for="degrees">Degrees</label>
    <input id="degrees" type="number" min="1" max="360" value="90" placeholder="Degrees to rotate (1-360)" />
    <small>Rotation angle (always positive)</small>

    <label for="direction">Direction</label>
    <select id="direction">
      <option value="clockwise">Clockwise</option>
      <option value="counter-clockwise">Counter-Clockwise</option>
    </select>
    <small>Choose rotation direction</small>

    <a class="cta" href="http://serialmonitor.local">Open Serial Monitor</a>
        <button id="rotateBtn">Rotate</button>
    <p id="status" style="margin-top:10px; color: var(--muted); font-size: 0.9rem;"></p>

    <script>
      // Function to update slave connection status
      async function updateSlaveStatus() {
        try {
          const response = await fetch('/slave-status');
          const data = await response.json();
          const statusDot = document.getElementById('statusDot');
          const statusText = document.getElementById('statusText');
          
          if (data.connected) {
            statusDot.className = 'status-dot status-connected';
            statusText.textContent = `Slave Chair Connected (${data.ip})`;
          } else {
            statusDot.className = 'status-dot status-disconnected';
            statusText.textContent = 'Slave Chair Disconnected';
          }
        } catch (error) {
          console.error('Failed to check slave status:', error);
          const statusDot = document.getElementById('statusDot');
          const statusText = document.getElementById('statusText');
          statusDot.className = 'status-dot status-disconnected';
          statusText.textContent = 'Status check failed';
        }
      }
      
      // Check status immediately and then every 3 seconds
      updateSlaveStatus();
      setInterval(updateSlaveStatus, 3000);
      
      document.getElementById('rotateBtn').addEventListener('click', async () => {
        const statusEl = document.getElementById('status');
        const chairInput = document.getElementById('chair');
        const speedInput = document.getElementById('speed');
        const degreesInput = document.getElementById('degrees');
        const directionInput = document.getElementById('direction');

        // Get values from inputs
        let chair = chairInput.value || 'master'; // default master
        let speed = parseInt(speedInput.value) || 5; // default speed 5
        let degrees = parseInt(degreesInput.value) || 90; // default 90 degrees
        let direction = directionInput.value || 'clockwise'; // default clockwise

        // Clamp speed to 1-10 range
        if (speed < 1) speed = 1;
        if (speed > 10) speed = 10;
        
        // Ensure degrees is positive
        degrees = Math.abs(degrees);

        statusEl.textContent = `Sending rotate command to ${chair} chair(s) (${degrees}° ${direction}, speed ${speed})...`;

        try {
          const resp = await fetch(`/rotate?chair=${chair}&degrees=${degrees}&direction=${direction}&speed=${speed}`);
          const text = await resp.text();
          statusEl.textContent = `Response: ${text} (${chair} chair(s): ${degrees}° ${direction} at speed ${speed})`;
        } catch (e) {
          console.error(e);
          statusEl.textContent = "Failed to contact ESP32.";
        }
      });
    </script>
  </div>
  
</body>
</html>
)rawliteral";
// Function to serve the HTML content
void serveIndexHtml(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", index_html);
}

// Function to serve the CSS content
void serveStyleCss(AsyncWebServerRequest *request) {
  request->send_P(200, "text/css", style_css);
}

// Function to serve the JS content
void serveScriptJs(AsyncWebServerRequest *request) {
  request->send_P(200, "application/javascript", script_js);
}
#endif  // CHAIR_MASTER

// =======================================================================
// WebPage.h
// Generates the HTML user interface for the ESP32 web server.
// Dynamically displays all GPIO controls and their current states.
// Uses PROGMEM to store static HTML/CSS/JS in flash memory.
// =======================================================================

#pragma once
#include <pgmspace.h>      // For storing large strings in flash memory
#include "GpioControl.h"   // Access to GPIO controls and their state

// HTML header section (includes styling and JavaScript)
// R"rawliteral(...)" allows for multi-line string literals with quotes inside
const char HTML_HEADER[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>IC-905 ESP32 Control</title>
  <link rel="icon" href="data:,">
  <style>
    /* CSS styling for page layout and buttons */
    body{background:linear-gradient(135deg,#232526 0%,#414345 100%);color:#fff;font-family:'Segoe UI',Helvetica,Arial,sans-serif;margin:0;padding:0;min-height:100vh;}
    .container{background:rgba(255,255,255,0.08);border-radius:16px;box-shadow:0 6px 32px rgba(0,0,0,0.2);max-width:520px;margin:40px auto;padding:32px 24px 24px 24px;text-align:center;}
    h1{margin-bottom:8px;font-size:2.2em;font-weight:700;letter-spacing:2px;}
    .status{display:inline-block;margin-left:10px;padding:3px 12px;border-radius:12px;font-size:0.95em;font-weight:600;background:#333;color:#fafafa;}
    .status.on{background:#4CAF50;color:#fff;}
    .status.off{background:#f44336;color:#fff;}
    .control-group{margin:22px 0 10px 0;padding-bottom:16px;border-bottom:1px solid rgba(255,255,255,0.1);}
    .toggle-btn{padding:16px 40px;font-size:1.15em;border-radius:8px;border:none;cursor:pointer;transition:background 0.2s;margin-top:10px;margin-bottom:4px;}
    .toggle-btn.on{background:#4CAF50;color:#fff;}
    .toggle-btn.off{background:#f44336;color:#fff;}
    @media (max-width:600px){.container{padding:12px 4px;}.toggle-btn{width:95%;font-size:1.05em;}}
  </style>
  <script>
    // JavaScript function to send GPIO toggle requests by changing the URL
    function toggleGPIO(pin, state){
      window.location.href='/' + pin + '/' + state;
    }
  </script>
</head>
<body>
  <div class="container">
    <h1>IC-905 Control</h1>
    <p style="margin-bottom:28px; color:#bbb;">ESP32 Web Server Interface</p>
)rawliteral";

// HTML footer section (closes the container and HTML page)
const char HTML_FOOTER[] PROGMEM = R"rawliteral(
  </div>
</body>
</html>
)rawliteral";

// Generates and sends the HTML page to the client
// Each control group shows the label, current ON/OFF state, and a toggle button
void sendHtmlPage(WiFiClient &client) {
  client.print(FPSTR(HTML_HEADER)); // Send the HTML header and open container

  // Loop through each GPIO control and create a UI control group
  for (size_t i = 0; i < CONTROL_COUNT; i++) {
    client.print("<div class=\"control-group\">"); // Group for each GPIO
    // Display the GPIO label
    client.print("<span>");
    client.print(controls[i].label);
    client.println("</span>");
    // Display the ON/OFF state with appropriate color
    client.print("<span class=\"status ");
    client.print(controls[i].state == "on" ? "on" : "off"); // Set status class
    client.print("\">");
    client.print(controls[i].state);                        // Show state text
    client.println("</span>");
    // Create a toggle button with correct color and label
    client.print("<button class=\"toggle-btn ");
    client.print(controls[i].state == "on" ? "on" : "off"); // Set button class
    client.print("\" onclick=\"toggleGPIO('");
    client.print(controls[i].pin);                          // Pin number
    client.print("','");
    client.print(controls[i].state == "on" ? "off" : "on"); // Toggle to opposite state
    client.print("')\">");
    client.print(controls[i].state == "on" ? "Turn OFF" : "Turn ON"); // Button text
    client.println("</button>");
    client.println("</div>");
  }
  client.print(FPSTR(HTML_FOOTER)); // Send the HTML footer to close the page
}
// Load Wi-Fi library
#include <WiFi.h>
#include <WiFiMulti.h>

WiFiMulti wifiMulti;
// Replace with your network credentials
//const char* ssid = "kb2rww";  //Replace with your network ID
//const char* ssid = "KB2RWW Silverado";  //Replace with your network ID
//const char* ssid = "kb2rwwp";  //Replace with your network ID
const char* password = "1244600000";  // Replace with your network password


// Helper to generate a control group with a toggle button
void generateControlGroup(WiFiClient &client, const char* label, int pin, const String& state) {
  client.println("<div class=\"control-group\">");
  client.print("<span>");
  client.print(label);
  client.println("</span>");
  client.print("<span class=\"status ");
  client.print(state == "on" ? "on" : "off");
  client.print("\">");
  client.print(state);
  client.println("</span>");
  client.print("<button class=\"toggle-btn ");
  client.print(state == "on" ? "on" : "off");
  client.print("\" onclick=\"toggleGPIO('");
  client.print(pin);
  client.print("','");
  client.print(state == "on" ? "off" : "on");
  client.print("')\">");
  client.print(state == "on" ? "Turn OFF" : "Turn ON");
  client.println("</button>");
  client.println("</div>");
}
// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output0State = "off";
String output2State = "off";
String output4State = "off";
String output32State = "off";
String output25State = "off";
String output27State = "off";
String output16State = "off";
String output17State = "off";
String output21State = "off";
String output22State = "off";

// Assign output variables to GPIO pins
const int output0 = 0;
const int output4 = 4;
const int output2 = 2;
const int output32 = 32;
const int output25 = 25;
const int output27 = 27;
const int output16 = 16;
const int output17 = 17;
const int output21 = 21;
const int output22 = 22;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
 
  // For DNS, you could add: WiFi.config(local_ip, gateway, subnet, primaryDNS, secondaryDNS);

  // Initialize the output variables as outputs
  pinMode(output0, OUTPUT);
  pinMode(output2, OUTPUT);
  pinMode(output4, OUTPUT);
  pinMode(output32, OUTPUT);
  pinMode(output25, OUTPUT);
  pinMode(output27, OUTPUT);
  pinMode(output16, OUTPUT);
  pinMode(output17, OUTPUT);
  pinMode(output21, OUTPUT);
  pinMode(output22, OUTPUT);
  
  // Set outputs to LOW
  digitalWrite(output0, LOW);
  digitalWrite(output2, LOW);
  digitalWrite(output4, LOW);
  digitalWrite(output32, LOW);
  digitalWrite(output25, LOW);
  digitalWrite(output27, LOW);
  digitalWrite(output16, LOW);
  digitalWrite(output17, LOW);
  digitalWrite(output21, LOW);
  digitalWrite(output22, LOW);
  
  // Connect to Wi-Fi network with SSID and password
  wifiMulti.addAP("kb2rww", password);
  wifiMulti.addAP("kb2rwwp", password);
  wifiMulti.addAP("KB2RWW Silverado", password);

  Serial.println("Connecting Wifi...");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Connected to SSID: ");
    Serial.println(WiFi.SSID());
    delay(500);
    Serial.print(".");
  }
  // Print the connnect IP address and start web server
  Serial.println("");
  Serial.println("Connected. The device can be found at IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client found.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /0/on") >= 0) {
              Serial.println("GPIO 0 On");
              output0State = "on";
              digitalWrite(output0, HIGH);
            } else if (header.indexOf("GET /0/off") >= 0) {
              Serial.println("GPIO 0 Off");
              output0State = "off";
              digitalWrite(output0, LOW);
            } else if (header.indexOf("GET /4/on") >= 0) {
              Serial.println("GPIO 4 on");
              output4State = "on";
              digitalWrite(output4, HIGH);
            } else if (header.indexOf("GET /4/off") >= 0) {
              Serial.println("GPIO 4 off");
              output4State = "off";
              digitalWrite(output4, LOW);
            }
            else if (header.indexOf("GET /2/on") >= 0) {
              Serial.println("GPIO 2 on");
              output2State = "on";
              digitalWrite(output2, HIGH);
            } 
            else if (header.indexOf("GET /2/off") >= 0) {
              Serial.println("GPIO 2 off");
              output2State = "off";
              digitalWrite(output2, LOW);
            }
            else if (header.indexOf("GET /32/on") >= 0) {
              Serial.println("GPIO 32 on");
              output32State = "on";
              digitalWrite(output32, HIGH);
            } else if (header.indexOf("GET /32/off") >= 0) {
              Serial.println("GPIO 32 off");
              output32State = "off";
              digitalWrite(output32, LOW);
            }
           else if (header.indexOf("GET /25/on") >= 0) {  
              Serial.println("GPIO 25 on");
              output25State = "on";
              digitalWrite(output25, HIGH);
            } else if (header.indexOf("GET /25/off") >= 0) {
              Serial.println("GPIO 25 off");
              output25State = "off";
              digitalWrite(output25, LOW);
            }
            else if (header.indexOf("GET /27/on") >= 0) {
              Serial.println("GPIO 27 on");
              output27State = "on";
              digitalWrite(output27, HIGH);
            } else if (header.indexOf("GET /27/off") >= 0) {
              Serial.println("GPIO 27 off");
              output27State = "off";
              digitalWrite(output27, LOW);
            }
            else if (header.indexOf("GET /16/on") >= 0) {
              Serial.println("GPIO 16 on");
              output16State = "on";
              digitalWrite(output16, HIGH);
            } else if (header.indexOf("GET /16/off") >= 0) {
              Serial.println("GPIO 16 off");
              output16State = "off";
              digitalWrite(output16, LOW);
            }
            else if (header.indexOf("GET /17/on") >= 0) {
              Serial.println("GPIO 17 on");
              output17State = "on";
              digitalWrite(output17, HIGH);
            } else if (header.indexOf("GET /17/off") >= 0) {
              Serial.println("GPIO 17 off");
              output17State = "off";
              digitalWrite(output17, LOW);
            }
            else if (header.indexOf("GET /21/on") >= 0) {
              Serial.println("GPIO 21 on");
              output21State = "on";
              digitalWrite(output21, HIGH);
            } else if (header.indexOf("GET /21/off") >= 0) {
              Serial.println("GPIO 21 off");
              output21State = "off";
              digitalWrite(output21, LOW);
            }
            else if (header.indexOf("GET /22/on") >= 0) {
              Serial.println("GPIO 22 on");
              output22State = "on";
              digitalWrite(output22, HIGH);
            } else if (header.indexOf("GET /22/off") >= 0) {
              Serial.println("GPIO 22 off");
              output22State = "off";
              digitalWrite(output22, LOW);
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
           client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
           "<title>IC-905 ESP32 Control</title>"
           "<link rel=\"icon\" href=\"data:,\">"
           "<style>"
           "body{background:linear-gradient(135deg,#232526 0%,#414345 100%);color:#fff;font-family:'Segoe UI',Helvetica,Arial,sans-serif;margin:0;padding:0;min-height:100vh;}"
           ".container{background:rgba(255,255,255,0.08);border-radius:16px;box-shadow:0 6px 32px rgba(0,0,0,0.2);max-width:520px;margin:40px auto;padding:32px 24px 24px 24px;text-align:center;}"
           "h1{margin-bottom:8px;font-size:2.2em;font-weight:700;letter-spacing:2px;}"
           ".status{display:inline-block;margin-left:10px;padding:3px 12px;border-radius:12px;font-size:0.95em;font-weight:600;background:#333;color:#fafafa;}"
           ".status.on{background:#4CAF50;color:#fff;}"
           ".status.off{background:#f44336;color:#fff;}"
           ".control-group{margin:22px 0 10px 0;padding-bottom:16px;border-bottom:1px solid rgba(255,255,255,0.1);}"
           ".toggle-btn{padding:16px 40px;font-size:1.15em;border-radius:8px;border:none;cursor:pointer;transition:background 0.2s;margin-top:10px;margin-bottom:4px;}"
           ".toggle-btn.on{background:#4CAF50;color:#fff;}"
           ".toggle-btn.off{background:#f44336;color:#fff;}"
           "@media (max-width:600px){.container{padding:12px 4px;}.toggle-btn{width:95%;font-size:1.05em;}}"
           "</style>"
           "<script>"
           "function toggleGPIO(pin, state){window.location.href='/' + pin + '/' + state;}"
           "</script></head><body>"
           "<div class=\"container\">"
           "<h1>IC-905 Control</h1>"
          "<p style=\"margin-bottom:28px; color:#bbb;\">ESP32 Web Server Interface</p>"
         );

         generateControlGroup(client, "Motor forward", 0, output0State);
         generateControlGroup(client, "Motor reverse", 4, output4State);
         generateControlGroup(client, "144 to 1296 triband", 32, output32State);
         generateControlGroup(client, "2304Ghz", 25, output25State);
         generateControlGroup(client, "Omnie enable", 27, output27State);
         generateControlGroup(client, "5760Ghz", 16, output16State);
         generateControlGroup(client, "triband dish", 17, output17State);
         generateControlGroup(client, "IO21", 21, output21State);
         generateControlGroup(client, "IO22", 22, output22State);
         generateControlGroup(client, "LED", 2, output2State);

         client.println("</div></body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
                                  
          } else {   // if you got a newline, then clear currentLine
            currentLine = "";
          }

      
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
        Serial.println("Client disconnected.");
    Serial.println("");
  }
}
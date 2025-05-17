// Load Wi-Fi library
#include <WiFi.h>
#include <WiFiMulti.h>

WiFiMulti wifiMulti;
// Replace with your network credentials
//const char* ssid = "kb2rww";  //Replace with your network ID
//const char* ssid = "KB2RWW Silverado";  //Replace with your network ID
//const char* ssid = "kb2rwwp";  //Replace with your network ID
const char* password = "1244600000";  // Replace with your network password

/* Put IP Address details */
IPAddress local_ip(192,168,0,123);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

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
           "<link rel=\"icon\" href=\"data:,\">"
            "<style>"
            "body{background:linear-gradient(135deg,#232526 0%,#414345 100%);color:#fff;font-family:'Segoe UI',Helvetica,Arial,sans-serif;margin:0;padding:0;min-height:100vh;}"
            ".container{background:rgba(255,255,255,0.08);border-radius:16px;box-shadow:0 6px 32px rgba(0,0,0,0.2);max-width:500px;margin:40px auto;padding:32px 24px;text-align:center;}"
            "h1{margin-bottom:8px;font-size:2.2em;font-weight:700;letter-spacing:2px;}"
            ".status{display:inline-block;margin-left:10px;padding:3px 12px;border-radius:12px;font-size:0.95em;font-weight:600;background:#333;color:#fafafa;}"
            ".status.on{background:#4CAF50;color:#fff;}"
            ".status.off{background:#f44336;color:#fff;}"
            ".control-group{margin:22px 0 10px 0;padding-bottom:16px;border-bottom:1px solid rgba(255,255,255,0.1);}"
            ".button{background-color:#555555;border:none;color:white;padding:16px 40px;text-decoration:none;font-size:1.15em;margin:6px 3px;cursor:pointer;border-radius:8px;transition:background 0.2s,transform 0.1s;box-shadow:0 2px 8px rgba(0,0,0,0.18);}"
            ".button2{background-color:#4CAF50;}"
            ".button:hover,.button2:hover{background-color:#222;transform:scale(1.04);}"
            "@media (max-width:600px){.container{padding:12px 4px;}.button,.button2{width:95%;font-size:1.05em;}}"
            "</style></head><body>");
            client.println("<div class=\"container\">");
            client.println("<h1>IC-905 Control</h1>");
            client.println("<p style=\"margin-bottom:28px; color:#bbb;\">ESP32 Web Server Interface</p>");

            // Motor forward
            client.println("<div class=\"control-group\">");
            client.print("<span>Motor forward</span>");
            client.print("<span class=\"status ");
            client.print(output0State == "on" ? "on" : "off");
            client.print("\">");
            client.print(output0State);
            client.println("</span>");
            client.println("<a href=\"/0/on\"><button class=\"button\">ON</button></a>");
            client.println("<a href=\"/0/off\"><button class=\"button button2\">OFF</button></a>");
            client.println("</div>");

            // Motor reverse
            client.println("<div class=\"control-group\">");
            client.print("<span>Motor reverse</span>");
            client.print("<span class=\"status ");
            client.print(output4State == "on" ? "on" : "off");
            client.print("\">");
            client.print(output4State);
            client.println("</span>");
            client.println("<a href=\"/4/on\"><button class=\"button\">ON</button></a>");
            client.println("<a href=\"/4/off\"><button class=\"button button2\">OFF</button></a>");
            client.println("</div>");

            // 144 to 1296 triband
            client.println("<div class=\"control-group\">");
            client.print("<span>144 to 1296 triband</span>");
            client.print("<span class=\"status ");
            client.print(output32State == "on" ? "on" : "off");
            client.print("\">");
            client.print(output32State);
            client.println("</span>");
            client.println("<a href=\"/32/on\"><button class=\"button\">ON</button></a>");
            client.println("<a href=\"/32/off\"><button class=\"button button2\">OFF</button></a>");
            client.println("</div>");

            // 2304Ghz
            client.println("<div class=\"control-group\">");
            client.print("<span>2304Ghz</span>");
            client.print("<span class=\"status ");
            client.print(output25State == "on" ? "on" : "off");
            client.print("\">");
            client.print(output25State);
            client.println("</span>");
            client.println("<a href=\"/25/on\"><button class=\"button\">ON</button></a>");
            client.println("<a href=\"/25/off\"><button class=\"button button2\">OFF</button></a>");
            client.println("</div>");

            // Omnie enable
            client.println("<div class=\"control-group\">");
            client.print("<span>Omnie enable</span>");
            client.print("<span class=\"status ");
            client.print(output27State == "on" ? "on" : "off");
            client.print("\">");
            client.print(output27State);
            client.println("</span>");
            client.println("<a href=\"/27/on\"><button class=\"button\">ON</button></a>");
            client.println("<a href=\"/27/off\"><button class=\"button button2\">OFF</button></a>");
            client.println("</div>");

            // 5760Ghz
            client.println("<div class=\"control-group\">");
            client.print("<span>5760Ghz</span>");
            client.print("<span class=\"status ");
            client.print(output16State == "on" ? "on" : "off");
            client.print("\">");
            client.print(output16State);
            client.println("</span>");
            client.println("<a href=\"/16/on\"><button class=\"button\">ON</button></a>");
            client.println("<a href=\"/16/off\"><button class=\"button button2\">OFF</button></a>");
            client.println("</div>");

            // triband dish
            client.println("<div class=\"control-group\">");
            client.print("<span>triband dish</span>");
            client.print("<span class=\"status ");
            client.print(output17State == "on" ? "on" : "off");
            client.print("\">");
            client.print(output17State);
            client.println("</span>");
            client.println("<a href=\"/17/on\"><button class=\"button\">ON</button></a>");
            client.println("<a href=\"/17/off\"><button class=\"button button2\">OFF</button></a>");
            client.println("</div>");

            // IO21
            client.println("<div class=\"control-group\">");
            client.print("<span>IO21</span>");
            client.print("<span class=\"status ");
            client.print(output21State == "on" ? "on" : "off");
            client.print("\">");
            client.print(output21State);
            client.println("</span>");
            client.println("<a href=\"/21/on\"><button class=\"button\">ON</button></a>");
            client.println("<a href=\"/21/off\"><button class=\"button button2\">OFF</button></a>");
            client.println("</div>");

            // IO22
            client.println("<div class=\"control-group\">");
            client.print("<span>IO22</span>");
            client.print("<span class=\"status ");
            client.print(output22State == "on" ? "on" : "off");
            client.print("\">");
            client.print(output22State);
            client.println("</span>");
            client.println("<a href=\"/22/on\"><button class=\"button\">ON</button></a>");
            client.println("<a href=\"/22/off\"><button class=\"button button2\">OFF</button></a>");
            client.println("</div>");

            // LED
            client.println("<div class=\"control-group\">");
            client.print("<span>LED</span>");
            client.print("<span class=\"status ");
            client.print(output2State == "on" ? "on" : "off");
            client.print("\">");
            client.print(output2State);
            client.println("</span>");
            client.println("<a href=\"/2/on\"><button class=\"button\">ON</button></a>");
            client.println("<a href=\"/2/off\"><button class=\"button button2\">OFF</button></a>");
            client.println("</div>");

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
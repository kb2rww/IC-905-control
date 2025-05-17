// Load Wi-Fi library
#include <WiFi.h>

// Replace with your network credentials
const char* ssid = "kb2rww";  //Replace with your network ID
//const char* ssid = "KB2RWW Silverado";  //Replace with your network ID
//const char* ssid = "kb2rwwp";  //Replace with your network ID
const char* password = "1244600000";  // Replace with your network password

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
  Serial.print("Connecting to Network:");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
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
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            //client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println(".button { background-color: #555555; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            //client.println(".button2 {background-color: #555555;}</style></head>");
            client.println(".button2 {background-color: #4CAF50;}</style></head>");

            // Web Page Heading
            client.println("<body><h1>IC-905 control</h1>");
            client.println("<body><h1>ESP32 Web Server</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 0  
            client.println("<p>Motor forword - Currently " + output0State + "</p>");
            // If the output0State is off, it displays the ON button       
            if (output0State=="off") {
              client.println("<p><a href=\"/0/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/0/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for GPIO 4  
            client.println("<p>Motor revers - Currently" + output4State + "</p>");
            // If the output4State is off, it displays the ON button       
            if (output4State=="off") {
              client.println("<p><a href=\"/4/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/4/off\"><button class=\"button button2\">OFF</button></a></p>");
            }

            // Display current state, and ON/OFF buttons for GPIO 32  
            client.println("<p>144 to 1296 triband  - Currently" + output32State + "</p>");
            // If the output32State is off, it displays the ON button       
            if (output32State=="off") {
              client.println("<p><a href=\"/32/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/32/off\"><button class=\"button button2\">OFF</button></a></p>");
            }

            // Display current state, and ON/OFF buttons for GPIO 25  
            client.println("<p>2304Ghz - Currently" + output25State + "</p>");
            // If the output25State is off, it displays the ON button       
            if (output25State=="off") {
              client.println("<p><a href=\"/25/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/25/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            // Display current state, and ON/OFF buttons for GPIO 27  
            client.println("<p>Omnie enable - Currently" + output27State + "</p>");
            // If the output27State is off, it displays the ON button       
            if (output27State=="off") {
              client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            // Display current state, and ON/OFF buttons for GPIO 16  
            client.println("<p>5760Ghz - Currently" + output16State + "</p>");
            // If the output16State is off, it displays the ON button       
            if (output16State=="off") {
              client.println("<p><a href=\"/16/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/16/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            // Display current state, and ON/OFF buttons for GPIO 17  
            client.println("<p>triband dish - Currently" + output17State + "</p>");
            // If the output17State is off, it displays the ON button       
            if (output17State=="off") {
              client.println("<p><a href=\"/17/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/17/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            // Display current state, and ON/OFF buttons for GPIO 21  
            client.println("<p>IO21 - Currently" + output21State + "</p>");
            // If the output21State is off, it displays the ON button       
            if (output21State=="off") {
              client.println("<p><a href=\"/21/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/21/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            // Display current state, and ON/OFF buttons for GPIO 22  
            client.println("<p>IO22 - Currently" + output22State + "</p>");
            // If the output22State is off, it displays the ON button       
            if (output22State=="off") {
              client.println("<p><a href=\"/22/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/22/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            // Display current state, and ON/OFF buttons for GPIO 2  
            client.println("<p>LED - Currently" + output2State + "</p>");
            // If the output2State is off, it displays the ON button       
            if (output2State=="off") {
              client.println("<p><a href=\"/2/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/2/off\"><button class=\"button button2\">OFF</button></a></p>");
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
            }

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
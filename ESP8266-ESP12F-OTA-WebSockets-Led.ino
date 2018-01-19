// Watch https://www.youtube.com/watch?v=PNQiu9GexXI for info
// ESP12F controls Led String via GPIO using WebSockets, so dimming is immediate.
// ESP12F runs in station Mode. Set SSID and password as desired.
// and Led or Led String is atached to the specified GPIO pin
// Websocket sets PWM value of GPIO. 
// Over the Air update available via http://<<your ESP IP Address/firmware
// To upload through terminal you can use: curl -F "image=@firmware.bin" esp8266-webupdate.local/update 
// Extends Katz's code from http://www.esp8266.com/viewtopic.php?f=8&t=11887

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

const char* host = "esp8266-webupdate";  // Host name
const char* update_path = "/firmware";   // path for upgrade
const char* update_username = "admin";   // admin username
const char* update_password = "UpToYou"; // password 

const char *esp_ssid = "Globe";          // Esp SSID
const char *esp_password = "SSID";       // password
int LED=12;                              // PINnumber where your LED is
int websockMillis=50;                    // SocketVariables are sent to client every 50 milliseconds
int sliderVal=60;                        // Default value of the slider
int holdPin = 13;                        // Turn this pin to stay alive
int pirPin = 4 ;                         // monitors PIR output pin 
int pir = 1;                             // Set PIR to on as awake
ESP8266WebServer httpServer(80);


WebSocketsServer webSocket=WebSocketsServer(88);        
String webSite,javaScript,JSONtxt;
unsigned long websockCount=0UL,wait000=0UL,wait001=0UL;
int LEDmillis=9*(100-sliderVal)+100;
boolean LEDonoff=true;
ESP8266HTTPUpdateServer httpUpdater;


// Build the webpage. Simple HTML with some javascript
void buildWebsite(){
  buildJavascript();
  webSite="<!DOCTYPE HTML><HTML>\n";
  webSite+="<META name='viewport' content='width=device-width, initial-scale=1'>\n";
  webSite+=javaScript;
  webSite+="<BODY>\n";
  webSite+="<BR><B>This is the ESP website ...</B><BR><BR>\n";
  webSite+="Runtime = <A ID='runtime'></A><BR>\n";
  webSite+="websockCount = <A ID='websockCount'></A><BR><BR>\n";
  webSite+="<TABLE BORDER=1 WIDTH=200px BGCOLOR='cornsilk' STYLE='border-collapse:collapse;text-align:center'>\n"; 
  webSite+="<TR><TD>Slidervalue = <A ID='Sliderval'></A><BR>\n";
  webSite+="LEDspeed = <A ID='LEDmillis'></A> ms</TD></TR>\n";
  //********** ONCHANGE works in IE, use ONINPUT in Firefox and Chrome **********
  webSite+="<TR><TD><INPUT ID='slider' TYPE='range' min='0' max = '1023' ONINPUT='Slider()' STYLE='writing-mode:bt-lr;-webkit-appearance:slider-vertical;' orient='vertical'></TD></TR>\n";
  webSite+="<TR><TD><BUTTON ID='button' ONCLICK='button()' STYLE='width:110px;height:40px'></BUTTON></TD></TR>\n";
  webSite+="</TABLE>\n"; 
  webSite+="</BODY>\n";
  webSite+="</HTML>\n";
}
// Webpage Javascript - connects a websocket, offers an off button 
void buildJavascript(){
  javaScript="<SCRIPT>\n";                                                                          
  javaScript+="InitWebSocket();\n";
  javaScript+="function InitWebSocket(){\n";
  javaScript+="  websock=new WebSocket('ws://'+window.location.hostname+':88/');\n";             // Connect a websocket 
  javaScript+="  websock.onmessage=function(evt){\n";                                            // process a received message 
  javaScript+="    JSONobj=JSON.parse(evt.data);\n";                                             // using JSON
  javaScript+="    document.getElementById('runtime').innerHTML=JSONobj.runtime;\n";             // show runtime on webpage
  javaScript+="    document.getElementById('websockCount').innerHTML=JSONobj.websockCount;\n";   // 
  javaScript+="    document.getElementById('slider').value=JSONobj.sliderVal;\n";                // show slider value
  javaScript+="    document.getElementById('Sliderval').innerHTML=JSONobj.sliderVal;\n";      
  javaScript+="    document.getElementById('LEDmillis').innerHTML=JSONobj.LEDmillis;\n";         // Led brightness
  javaScript+="    document.getElementById('button').innerHTML=JSONobj.LEDonoff;\n";             
  javaScript+="  }\n";
  javaScript+="}\n";

  javaScript+="function button(){\n";                                                            // if off button pressed          
  javaScript+="  btn='LEDonoff=LED = ON';\n";                                                       
  javaScript+="  if(document.getElementById('button').innerHTML==='LED = ON')btn='LEDonoff=LED = OFF';\n";  // Toggle button
  javaScript+="  websock.send(btn);\n";                                                          // Send msg to ESP12F
  javaScript+="}\n";

  javaScript+="function Slider(){\n";                                                            // if the slider changes 
  javaScript+="  sliderVal=document.getElementById('slider').value;\n";                          // Get Value
  javaScript+="  websock.send('sliderVal='+sliderVal);\n";                                       // send value via websockets
  javaScript+="}\n";
 
  javaScript+="</SCRIPT>\n";
}

// convert runtime from milliseconds to HH:MM:SS
String millis2time(){                           
  String Time="";
  unsigned long ss;
  byte mm,hh;
  ss=millis()/1000;
  hh=ss/3600;
  mm=(ss-hh*3600)/60;
  ss=(ss-hh*3600)-mm*60;
  if(hh<10)Time+="0";
  Time+=(String)hh+":";
  if(mm<10)Time+="0";
  Time+=(String)mm+":";
  if(ss<10)Time+="0";
  Time+=(String)ss;
  return Time;
}


// handler function for requests for 
void handleRoot() {
  buildWebsite();
  httpServer.send(200,"text/html",webSite);
}

// Process a received websocket message
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t wslength){
  String payloadString=(const char *)payload;
  //Serial.println("payload: '"+payloadString+"', channel: "+(String)num);
  if(type==WStype_TEXT){                                                               // Message received
    byte separator=payloadString.indexOf('=');                                         // parse information
    String var=payloadString.substring(0,separator);
    String val=payloadString.substring(separator+1);
    if(var=="LEDonoff"){                                                               // OFF message received
      LEDonoff=false;
      if(val=="LED = ON")LEDonoff=true;
      digitalWrite(LED,LEDonoff);
      if(val=="LED = OFF") {
         Serial.println("+++ TURNING OFF, going to sleep");
         digitalWrite(holdPin, LOW);  // set to 0 to take CH_PD Low and turn off 
         ESP.deepSleep(0);                                                            // Send ESP12F to sleep, wake via a trigger
         }
     
    }else if(var=="sliderVal"){                                                       // SliderValue has changed
      sliderVal=val.toInt();                                                          // Get Value
      // LEDmillis=9*(100-sliderVal)+100;
      LEDmillis = sliderVal;                                                          // Set Led PWM value ie brightness
    }
  }
}

// handler for not found
void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += httpServer.uri();
  message += "\nMethod: ";
  message += (httpServer.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += httpServer.args();
  message += "\n";
  for (uint8_t i=0; i<httpServer.args(); i++){
    message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
  }
  httpServer.send(404, "text/plain", message);
}



void setup(){
  
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting Sketch...");
  pinMode(LED,OUTPUT);                                    // Pin that Led String is connected to 
  pinMode(holdPin,OUTPUT);                               
  pinMode(pirPin,INPUT);                                  // Reads value of PIR
  digitalWrite(holdPin, HIGH);                            // keeps esp on when PIR turns off 
  WiFi.mode(WIFI_AP);                                     // Wifi AP mode
  WiFi.softAP(esp_ssid, esp_password);
  
  IPAddress accessIP = WiFi.softAPIP();                  
  Serial.print("ESP AccessPoint IP address: ");
  Serial.println(accessIP);
  webSocket.begin();                                      // start WebSockets
  webSocket.onEvent(webSocketEvent);                      // Add handler for websockets

  
  httpServer.on("/", handleRoot);                         // Set up http server and error handlers
  httpServer.on("/inline", [](){
  httpServer.send(200, "text/plain", "this works as well");
  });
  httpServer.onNotFound(handleNotFound);
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin(); `                                   // Start Webserver


  Serial.println("HTTP server started");                  // Debug and monitor message
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);
}

// Loop processing http and websocket requests
void loop(){                                               
           
  webSocket.loop();                                       
  httpServer.handleClient();
  /* Flicker - v2 possibly, flicker the leds...
  if(millis()>wait000&&LEDonoff==true){
    digitalWrite(LED,!digitalRead(LED));
    wait000=millis()+LEDmillis;
  } */ 

     analogWrite(LED, sliderVal);                         // Set LedString to value set on webpage via slider.
    
    if(millis()>wait001){
    websockCount++;
    String LEDswitch="LED = OFF";
    if(LEDonoff==true)LEDswitch="LED = ON";
    JSONtxt="{\"runtime\":\""+millis2time()+"\","+        // JSON requires double quotes
             "\"websockCount\":\""+(String)websockCount+"\","+
             "\"sliderVal\":\""+(String)sliderVal+"\","+
             "\"LEDmillis\":\""+(String)LEDmillis+"\","+
             "\"LEDonoff\":\""+LEDswitch+"\"}";
    webSocket.broadcastTXT(JSONtxt);                      // update connected webpage(s) with information via Websocket
    wait001=millis()+websockMillis;
  }
}

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include "lib/StringTokenizer/StringTokenizer.h"

const char WIFI_AP_PASSWORD[] = "sparkfun";

const int LED_PIN = 5; // Thing's onboard, green LED
const int ANALOG_PIN = A0; // The only analog pin on the Thing
const int BUTTON_PIN = 12; // Digital pin to be read

WiFiServer server(80);
String url = "";
String ssid = "";
String password = "";

bool configMode = true;
int buttonState = 0;

void setup()
{
  initHardware();

  delay(1000);
  buttonState = digitalRead(BUTTON_PIN);
  //if button is pressed for 1 sec, enter config mode
  if (buttonState == HIGH) {
    Serial.println("Entering config mode.");
    setupWiFiAP();
  } else {
    configMode = false;
    loadData();
    if (!connectAsStation()){
      Serial.println("");
      Serial.println("Connect timed out, entering config mode.");
      configMode = true;
      setupWiFiAP();
    }
  }
}

void loop()
{
  if (configMode)
    loopConfigMode();
  else
    loopRunMode();
}

void loopRunMode() {
  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == HIGH) {
    digitalWrite(LED_PIN, HIGH);
    sendHttpRequest();
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void loopConfigMode() {

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  int startParamsIndex = req.indexOf("?");
  if (startParamsIndex != -1) {
    StringTokenizer paramsListTokenizer(req.substring(startParamsIndex + 1), "&");
    while (paramsListTokenizer.hasNext()) {
      String param = paramsListTokenizer.nextToken();
      StringTokenizer paramTokenizer(param, "=");
      String paramKey = paramTokenizer.nextToken();
      String paramValue = paramTokenizer.nextToken();
      if (paramKey == "url") {
        url = paramValue.substring(0, paramValue.indexOf(" "));
      } else if (paramKey == "ssid") {
        ssid = paramValue;
      } else if (paramKey == "password") {
        password = paramValue;
      }
    }
  }

  client.flush();
  // Prepare the response. Start with the common header:
  String s = "HTTP/1.1 200 OK\r\n";
  s += "Content-Type: text/html\r\n\r\n";
  s += "<!DOCTYPE HTML>\r\n<html>\r\n";
  if (url == "" || ssid == "" || password == "")
  {
    s += "Error: Invalid url, ssid or password.";
    Serial.println("Empty: " + url + " " + ssid + " " + password);
  }
  else
  {
    // save to memory
    int address = 0;
    
    writeString(address, ssid);
    writeString(address, password);
    writeString(address, url);
    
    EEPROM.commit();
    Serial.println("Saved data: " + url + " " + ssid + " " + password);
    s += "Done.";
  }
  s += "</html>\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  client.flush();
  EEPROM.end();
  Serial.println("Client disonnected");
}

bool connectAsStation() {
  WiFi.mode(WIFI_STA);
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    WiFi.begin(ssid.c_str(), password.c_str());
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to " + ssid);
      configMode = false;
      return true;
    }
    delay(500);
    Serial.print(WiFi.status());
    c++;
  }
  
  return false;
}

void readString(int &addr, String& str) {
  char ch = char(EEPROM.read(addr));
  while ( ch != '\0') {
    str += ch;
    addr++;
    ch = char(EEPROM.read(addr));
  }
  addr++;
}

void writeString(int& addr, String str) {
  for (int i=0; i < str.length(); i++) {
    EEPROM.put(addr + i, str.charAt(i));
  }
  addr += str.length();

  EEPROM.put(addr, '\0');
  addr++;
}

void loadData() {
  int address = 0;
  digitalWrite(LED_PIN, HIGH);
  delay(3000);
  digitalWrite(LED_PIN, LOW);

  Serial.println("Reading data");

  readString(address, ssid);
  Serial.println("ssid: " + ssid);

  readString(address, password);
  Serial.println("password: " + password);

  readString(address, url);
  Serial.println("url: " + url);
}


void setupWiFiAP()
{
  WiFi.mode(WIFI_AP);

  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd) to "Thing-":
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String AP_NameString = "ESP8266 Thing " + macID;

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i = 0; i < AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  WiFi.softAP(AP_NameChar, WIFI_AP_PASSWORD);
  server.begin();
}

void initHardware()
{
  Serial.begin(9600);
  EEPROM.begin(512);
  delay(10);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}


void sendHttpRequest() {
  Serial.print("Connecting to server ...");
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  
  String host = "";
  if (url.indexOf('/') > -1)
    host = url.substring(0, url.indexOf('/'));
  else
    host = url; 

  const int httpPort = 8000;
  if (!client.connect(host.c_str(), httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URI for the request

  String path = "";
  if (url.indexOf('/') > -1)
    path = url.substring(url.indexOf('/'));
  else 
    path = "/";
  
  Serial.print("Requesting URL: ");
  Serial.println(path + " on " + host);
  
  // This will send the request to the server
  client.print(String("GET ") + path + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.println(line);
  }
  
  Serial.println();
  Serial.println("Closing connection");
}


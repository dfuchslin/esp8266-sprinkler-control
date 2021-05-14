#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>

#include <uri/UriBraces.h>


#define RELAY_ON  LOW
#define RELAY_OFF HIGH
#define RELAY_MASTER_PIN D7
#define RELAY_1_PIN D6
#define RELAY_2_PIN D5
#define RELAY_3_PIN 9  //D4
#define RELAY_4_PIN 10 //D3
#define RELAY_5_PIN D2
#define RELAY_6_PIN D1
#define RELAY_7_PIN D0

const char* hostname = "sprinkler-control";
const byte relays[] = { RELAY_1_PIN, RELAY_2_PIN, RELAY_3_PIN, RELAY_4_PIN, RELAY_5_PIN, RELAY_6_PIN, RELAY_7_PIN };

WiFiManager wifiManager;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer updater;

void health()
{
  server.send(200, F("text/plain"), F("ok"));
}

void status()
{
  String status = "{";
  status += "\"status\":{";
  status += "\"master\":" + relayStatus(RELAY_MASTER_PIN) + ",";
  status += "\"relays\":[";
  for (int i = 0; i < sizeof(relays); i++)
  {
    if (i > 0) status += ",";
    status += relayStatus(relays[i]);
  }
  status += "]";
  status += "}";
  status += "}";

  server.send(200, F("application/json"), status);
}

String relayStatus(byte relayPin)
{
  return isRelayOn(relayPin) ? "\"On\"" : "\"Off\"";
}

void metrics()
{
  String message = F("# HELP esp8266_up Is this host up\n");
  message += F("# HELP esp8266_up gauge\n");
  message += F("esp8266_up 1\n");

  message += F("# HELP sprinkler_relay Relay pin status\n");
  message += F("# HELP sprinkler_relay gauge\n");
  message += getSprinklerRelayMetric(F("master"), RELAY_MASTER_PIN);
  for (int i = 0; i < sizeof(relays); i++)
  {
    message += getSprinklerRelayMetric(String(i), relays[i]);
  }

  server.send(200, F("text/plain"), message);
}

String getSprinklerRelayMetric(String name, byte relayPin)
{
  return "sprinkler_relay{name=\"" + name + "\",gpio=\"" + String(relayPin) + "\"} " + String(isRelayOn(relayPin)) + "\n";
}

bool isRelayOn(byte relayPin)
{
  return digitalRead(relayPin) ? RELAY_ON : RELAY_OFF;
}

void relayOn(byte relayPin)
{
  digitalWrite(RELAY_MASTER_PIN, RELAY_ON);
  digitalWrite(relayPin, RELAY_ON);
}

void relayOff(byte relayPin)
{
  digitalWrite(relayPin, RELAY_OFF);
  turnMasterRelayOff(relayPin);
}

void turnMasterRelayOff(byte relayPin)
{
  bool aRelayIsActive = false;
  for (int i = 0; i < sizeof(relays); i++)
  {
    if (isRelayOn(relays[i]))
    {
      aRelayIsActive = true;
      break;
    }
  }
  if (!aRelayIsActive)
  {
    digitalWrite(RELAY_MASTER_PIN, RELAY_OFF);
  }
}

void on()
{
  byte relay = server.pathArg(0).toInt();
  if (relay < 0 || relay >= sizeof(relays))
  {
    server.send(400, F("text/plain"), "Invalid relay '" + server.pathArg(0) + "'");
    return;
  }
  relayOn(relays[relay]);
  status();
}

void off()
{
  byte relay = server.pathArg(0).toInt();
  if (relay < 0 || relay >= sizeof(relays))
  {
    server.send(400, F("text/plain"), "Invalid relay '" + server.pathArg(0) + "'");
    return;
  }
  if (server.pathArg(0).equals("all"))
  {
    turnOffAll();
  }
  else
  {
    relayOff(relays[relay]);
  }
  status();
}

void updateProgress(unsigned int, unsigned int)
{
  turnOffAll();
}

void turnOffAll()
{
  for (int i = 0; i < sizeof(relays); i++)
  {
    relayOff(relays[i]);
  }
  relayOff(RELAY_MASTER_PIN);
}

void reset()
{
  server.send(200, F("text/plain"), F("All network settings reset. Please reboot and reconfigure."));
  wifiManager.resetSettings();
}

void reboot()
{
  ESP.reset();
}

void routing()
{
  server.on("/", HTTP_GET, []() {
    server.send(200, F("text/plain"),
                F("Sprinkler controller"));
  });
  server.on(F("/health"), HTTP_GET, health);
  server.on(F("/metrics"), HTTP_GET, metrics);
  server.on(F("/status"), HTTP_GET, status);
  server.on(F("/reset"), HTTP_DELETE, reset);
  server.on(F("/reboot"), HTTP_POST, reboot);
  server.on(UriBraces("/on/{}"), HTTP_GET, on);
  server.on(UriBraces("/off/{}"), HTTP_GET, off);
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, F("text/plain"), message);
}

void setupRelays()
{
  digitalWrite(RELAY_MASTER_PIN, RELAY_OFF);
  pinMode(RELAY_MASTER_PIN, OUTPUT);

  for (int i = 0; i < sizeof(relays); i++)
  {
    digitalWrite(relays[i], RELAY_OFF);
    pinMode(relays[i], OUTPUT);
  }
}


void setup(void)
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  wifiManager.setHostname(hostname);
  wifiManager.autoConnect();

  if (MDNS.begin(hostname))
  {
  }

  routing();
  server.onNotFound(handleNotFound);
  updater.setup(&server);
  server.begin();

  MDNS.addService("http", "tcp", 80);

  setupRelays();
}

void loop(void)
{
  MDNS.update();
  server.handleClient();
}

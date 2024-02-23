#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include <DNSServer.h>
#include <vector>

#include "fauxmoESP.h"

#define MAYOR 1
#define MINOR 4
#define PATCH 3
#define WIFI_SSID "JAVI"
#define WIFI_PASS "xavier1234"
#define DEVICE "sala"
#define TCP_PORT 7050
#define relay D2
#define LED 2
AsyncWebServer server(8080);

fauxmoESP fauxmo;

long int counter = 0;

static DNSServer DNS;

static std::vector<AsyncClient *> clients; // a list to hold all clients

String version = String(MAYOR) + "." + String(MINOR) + "." + String(PATCH);
/* clients events */
static void handleError(void *arg, AsyncClient *client, int8_t error)
{
  Serial.printf("\n connection error %s from client %s \n", client->errorToString(error), client->remoteIP().toString().c_str());
}

static void handleData(void *arg, AsyncClient *client, void *data, size_t len)
{
  String IP_address_client = client->remoteIP().toString().c_str();
  Serial.printf("\n data received from client %s \n", IP_address_client.c_str());

  Serial.write((uint8_t *)data, len);

  // reply to client
  if (strcmp((char *)data, "T") == 0)
  {

    if (client->space() > 32 && client->canSend())
    {
      char reply[32];
      uint32_t seconds = (uint32_t)(millis() / 1000);
      Serial.println(seconds);
      sprintf(reply, "%d\n", seconds);
      client->add(reply, strlen(reply));
      client->send();
    }
  }
  // reply to client
  if (strcmp((char *)data, "V") == 0)
  {

    if (client->space() > 32 && client->canSend())
    {
      char reply[32];
      uint32_t seconds = (uint32_t)(millis() / 1000);
      Serial.println(seconds);
      sprintf(reply, "v%d.%d.%d\n", MAYOR, MINOR, PATCH);
      client->add(reply, strlen(reply));
      client->send();
    }
  }
}

static void handleDisconnect(void *arg, AsyncClient *client)
{
  Serial.printf("\n client %s disconnected \n", client->remoteIP().toString().c_str());
}

static void handleTimeOut(void *arg, AsyncClient *client, uint32_t time)
{
  Serial.printf("\n client ACK timeout ip: %s \n", client->remoteIP().toString().c_str());
}

/* server events */
static void handleNewClient(void *arg, AsyncClient *client)
{
  Serial.printf("\n new client has been connected to server, ip: %s", client->remoteIP().toString().c_str());

  // add to list
  clients.push_back(client);

  // register events
  client->onData(&handleData, NULL);
  client->onError(&handleError, NULL);
  client->onDisconnect(&handleDisconnect, NULL);
  client->onTimeout(&handleTimeOut, NULL);
}

void setup()
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  pinMode(relay, OUTPUT);

  digitalWrite(relay, LOW);
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB
  }
  // Serial.println("ESP8266 Server");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS); // change it to your ussid and password
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {       uint32_t seconds = (uint32_t)(millis() / 1000);
                    char reply[32];
                    Serial.println(seconds);
                    sprintf(reply, "%d\n", seconds);
              request->send(200, "text/plain", reply); });

  AsyncElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();
  // TCP SOCKET TO SEND UPTIME AND VERSION
  //AsyncServer *server_tcp = new AsyncServer(TCP_PORT); // start listening on tcp port 7050
  //server_tcp->onClient(&handleNewClient, server_tcp);
  //server_tcp->begin();

  // Serial.println("HTTP server started");

  fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char value)
                    {
    // Callback when a command from Alexa is received.
    // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
    // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
    // Just remember not to delay too much here, this is a callback, exit as soon as possible.
    // If you have to do something more involved here set a flag and process it in your main loop.

    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

    // Checking for device_id is simpler if you are certain about the order they are loaded and it does not change.
    // Otherwise comparing the device_name is safer.

    if (strcmp(device_name, DEVICE) == 0) {
      digitalWrite(LED, state);
      digitalWrite(relay,state);
    } });
  fauxmo.createServer(true); // not needed, this is the default value
  fauxmo.setPort(80);        // This is required for gen3 devices
  fauxmo.enable(true);       // Disabling it will prevent the devices from being discovered and switched
  fauxmo.addDevice(DEVICE);
}

void loop()
{

  DNS.processNextRequest();

  fauxmo.handle();

  // This is a sample code to output free heap every 5 seconds
  // This is a cheap way to detect memory leaks
  static unsigned long last = millis();
  if (millis() - last > 5000)
  {
    last = millis();
    ESP.getFreeHeap();
    // Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
  }
}

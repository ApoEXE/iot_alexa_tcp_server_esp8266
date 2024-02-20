#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <vector>

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "fauxmoESP.h"
#include "config.h"

#define MAYOR 1
#define MINOR 1
#define PATCH 0
#define WIFI_SSID "JAVI"
#define WIFI_PASS "xavier1234"
#define DEVICE "sala"

fauxmoESP fauxmo;

long int counter = 0;

static DNSServer DNS;


static std::vector<AsyncClient*> clients;  // a list to hold all clients


void initOTA();

/* clients events */
static void handleError(void* arg, AsyncClient* client, int8_t error) {
  Serial.printf("\n connection error %s from client %s \n", client->errorToString(error), client->remoteIP().toString().c_str());
}

static void handleData(void* arg, AsyncClient* client, void* data, size_t len) {
  String IP_address_client = client->remoteIP().toString().c_str();
  Serial.printf("\n data received from client %s \n", IP_address_client.c_str());

  Serial.write((uint8_t*)data, len);



  // reply to client
  if (strcmp((char*)data, "T") == 0) {

    if (client->space() > 32 && client->canSend()) {
      char reply[32];
      uint32_t seconds = (uint32_t)(millis() / 1000);
      Serial.println(seconds);
      sprintf(reply, "%d\n", seconds);
      client->add(reply, strlen(reply));
      client->send();
    }
  }
  // reply to client
  if (strcmp((char*)data, "V") == 0) {

    if (client->space() > 32 && client->canSend()) {
      char reply[32];
      uint32_t seconds = (uint32_t)(millis() / 1000);
      Serial.println(seconds);
      sprintf(reply, "v%d.%d.%d\n", MAYOR, MINOR, PATCH);
      client->add(reply, strlen(reply));
      client->send();
    }
  }
}

static void handleDisconnect(void* arg, AsyncClient* client) {
  Serial.printf("\n client %s disconnected \n", client->remoteIP().toString().c_str());
}

static void handleTimeOut(void* arg, AsyncClient* client, uint32_t time) {
  Serial.printf("\n client ACK timeout ip: %s \n", client->remoteIP().toString().c_str());
}


/* server events */
static void handleNewClient(void* arg, AsyncClient* client) {
  Serial.printf("\n new client has been connected to server, ip: %s", client->remoteIP().toString().c_str());

  // add to list
  clients.push_back(client);

  // register events
  client->onData(&handleData, NULL);
  client->onError(&handleError, NULL);
  client->onDisconnect(&handleDisconnect, NULL);
  client->onTimeout(&handleTimeOut, NULL);
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  Serial.begin(115200);
  delay(20);
  Serial.println("ESP8266 Server");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);  // change it to your ussid and password
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  initOTA();
  fauxmo.onSetState([](unsigned char device_id, const char* device_name, bool state, unsigned char value) {
    // Callback when a command from Alexa is received.
    // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
    // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
    // Just remember not to delay too much here, this is a callback, exit as soon as possible.
    // If you have to do something more involved here set a flag and process it in your main loop.

    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

    // Checking for device_id is simpler if you are certain about the order they are loaded and it does not change.
    // Otherwise comparing the device_name is safer.

    if (strcmp(device_name, DEVICE) == 0) {
      digitalWrite(BUILTIN_LED, state ? HIGH : LOW);
    }
  });
  fauxmo.createServer(true);  // not needed, this is the default value
  fauxmo.setPort(80);         // This is required for gen3 devices
  fauxmo.enable(true);        // Disabling it will prevent the devices from being discovered and switched
  fauxmo.addDevice(DEVICE);

  AsyncServer* server = new AsyncServer(TCP_PORT);  // start listening on tcp port 7050
  server->onClient(&handleNewClient, server);
  server->begin();
}

void loop() {
  ArduinoOTA.handle();
  DNS.processNextRequest();
  fauxmo.handle();

  // This is a sample code to output free heap every 5 seconds
  // This is a cheap way to detect memory leaks
  static unsigned long last = millis();
  if (millis() - last > 5000) {
    last = millis();
    ESP.getFreeHeap();
    //Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
  }
}


void initOTA() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}
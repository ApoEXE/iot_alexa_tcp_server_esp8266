#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <vector>

#include "config.h"

long int counter = 0;

static DNSServer DNS;


static std::vector<AsyncClient*> clients;  // a list to hold all clients


void blink() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }
  digitalWrite(LED_BUILTIN, HIGH);
}


/* clients events */
static void handleError(void* arg, AsyncClient* client, int8_t error) {
  Serial.printf("\n connection error %s from client %s \n", client->errorToString(error), client->remoteIP().toString().c_str());
}

static void handleData(void* arg, AsyncClient* client, void* data, size_t len) {
  String IP_address_client = client->remoteIP().toString().c_str();
  Serial.printf("\n data received from client %s \n", IP_address_client.c_str());

  Serial.write((uint8_t*)data, len);

  

  // reply to client
  if (strcmp((char*)data, "A") == 0) {
  
    if (client->space() > 32 && client->canSend()) {
      char reply[32];
      uint32_t seconds =  (uint32_t)(millis()/1000);
      Serial.println(seconds);
      sprintf(reply, "time %d", seconds);
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
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  delay(20);
  Serial.println("ESP8266 Server");
  WiFi.mode(WIFI_STA);
  WiFi.begin("TP-JAVI", "xavier1234");  // change it to your ussid and password
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

  AsyncServer* server = new AsyncServer(TCP_PORT);  // start listening on tcp port 7050
  server->onClient(&handleNewClient, server);
  server->begin();
}

void loop() {
  DNS.processNextRequest();
}

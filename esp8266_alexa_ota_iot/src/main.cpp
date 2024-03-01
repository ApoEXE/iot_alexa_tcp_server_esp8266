#include "Arduino.h"
#include <Esp.h>
// WATCHDOG

volatile uint32_t lastMillis = 0;

#define TIMER_INTERVAL_MS 500

// OTA
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

// ALEXA
#include "fauxmoESP.h"
#define DEVICE "test"

#define relay 4

#define MAYOR 1
#define MINOR 4
#define PATCH 10
#define WIFI_SSID "JAVI"
#define WIFI_PASS "xavier1234"

String version = String(MAYOR) + "." + String(MINOR) + "." + String(PATCH);
// Init ESP8266 timer 1

//=======================================================================

// OTA
AsyncWebServer server(8080);

// ALEXA
fauxmoESP fauxmo;
void conf_device_alexa()
{
  fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char value)
                    {


    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);


    if (strcmp(device_name, DEVICE) == 0) {
      //digitalWrite(LED_BUILTIN, state);
      digitalWrite(relay,state);
    } });
  fauxmo.createServer(true); // not needed, this is the default value
  fauxmo.setPort(80);        // This is required for gen3 devices
  fauxmo.enable(true);       // Disabling it will prevent the devices from being discovered and switched
  fauxmo.addDevice(DEVICE);
}

void setup()
{
  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relay, OUTPUT);
  Serial.begin(9600);
  Serial.printf("Reset reason: %s\n", ESP.getResetReason().c_str());
  ESP.wdtEnable(2000);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB
  }
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

  conf_device_alexa();

  Serial.println("Delta ms = " + String(millis() - lastMillis) + " " + version);
}

void loop()
{
   ESP.wdtFeed();
   fauxmo.handle();

  // This is a sample code to output free heap every 5 seconds
  // This is a cheap way to detect memory leaks
  
  static unsigned long last = millis();
  if (millis() - last > 2000)
  {
    last = millis();
    ESP.getFreeHeap();
    Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.println("");
  }
  
}

#include "Arduino.h"
#include <Esp.h>
// WATCHDOG

volatile uint32_t lastMillis = 0;

#define TIMER_INTERVAL_MS 500

// OTA
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <ElegantOTA.h>

// ALEXA
#include "fauxmoESP.h"
#define DEVICE "test"

#define relay 4

#define MAYOR 1
#define MINOR 4
#define PATCH 17
#define WIFI_SSID "JAVI"
#define WIFI_PASS "xavier1234"

bool state = 1;
String version = String(MAYOR) + "." + String(MINOR) + "." + String(PATCH);

// OTA
unsigned long ota_progress_millis = 0;
AsyncWebServer server(8080);
void onOTAStart()
{
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final)
{
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000)
  {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success)
{
  // Log when OTA has finished
  if (success)
  {
    Serial.println("OTA update finished successfully!");
  }
  else
  {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

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
                    sprintf(reply, "%d %s\n", seconds,version.c_str());
              request->send(200, "text/plain", reply); });

  ElegantOTA.begin(&server); // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
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
    digitalWrite(LED_BUILTIN, state);
    state = !state;
    last = millis();
    ESP.getFreeHeap();
    Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.println("");
  }
}

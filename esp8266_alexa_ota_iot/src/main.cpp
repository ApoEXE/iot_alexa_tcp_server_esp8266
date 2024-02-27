#include "Arduino.h"

//OTA
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

//ALEXA
#include "fauxmoESP.h"
#define DEVICE "sala"

#define relay 4

#define MAYOR 1
#define MINOR 4
#define PATCH 3
#define WIFI_SSID "JAVI"
#define WIFI_PASS "xavier1234"

//OTA
AsyncWebServer server(8080);
String version = String(MAYOR) + "." + String(MINOR) + "." + String(PATCH);


//ALEXA
fauxmoESP fauxmo;
void conf_device_alexa(){
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
      digitalWrite(LED_BUILTIN, state);
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
}

void loop()
{
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

#include "Arduino.h"
#include <Esp.h>
// WATCHDOG

// Timer
#if !defined(ESP8266)
#error This code is designed to run on ESP8266 and ESP8266-based boards! Please check your Tools->Board setting.
#endif
// Select a Timer Clock
#define USING_TIM_DIV1 false  // for shortest and most accurate timer
#define USING_TIM_DIV16 false // for medium time and medium accurate timer
#define USING_TIM_DIV256 true // for longest timer but least accurate. Default
#include "ESP8266TimerInterrupt.h"

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
#define PATCH 8
#define WIFI_SSID "JAVI"
#define WIFI_PASS "xavier1234"

String version = String(MAYOR) + "." + String(MINOR) + "." + String(PATCH);
// Init ESP8266 timer 1
ESP8266Timer ITimer;
volatile bool statusLed = false;
//=======================================================================
void IRAM_ATTR TimerHandler()
{
  static bool started = false;

  if (!started)
  {
    started = true;
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(relay, OUTPUT);
  }

  digitalWrite(LED_BUILTIN, statusLed); // Toggle LED Pin
  //digitalWrite(relay, statusLed);       // Toggle LED Pin
  statusLed = !statusLed;
  Serial.println("Delta ms = " + String(millis() - lastMillis) +" "+version);
  lastMillis = millis();

#if (TIMER_INTERRUPT_DEBUG > 0)
  Serial.println("Delta ms = " + String(millis() - lastMillis));
  lastMillis = millis();
#endif
}

// OTA
AsyncWebServer server(8080);


// ALEXA
/*fauxmoESP fauxmo;
void conf_device_alexa()
{
  fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char value)
                    {
    // Callback when a command from Alexa is received.
    // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
    // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
    // Just remember not to delay too much here, this is a callback, exit as soon as possible.
    // If you have to do something more involved here set a flag and process it in your main loop.

    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
//TIMER
    // Checking for device_id is simpler if you are certain about the order they are loaded and it does not change.
    // Otherwise comparing the device_name is safer.

    if (strcmp(device_name, DEVICE) == 0) {
      //digitalWrite(LED_BUILTIN, state);
      digitalWrite(relay,state);
    } });
  fauxmo.createServer(true); // not needed, this is the default value
  fauxmo.setPort(80);        // This is required for gen3 devices
  fauxmo.enable(true);       // Disabling it will prevent the devices from being discovered and switched
  fauxmo.addDevice(DEVICE);
}
*/
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

  //conf_device_alexa();

  Serial.println(ARDUINO_BOARD);
  Serial.println(ESP8266_TIMER_INTERRUPT_VERSION);
  Serial.print(F("CPU Frequency = "));
  Serial.print(F_CPU / 1000000);
  Serial.println(F(" MHz"));
  // Interval in microsecs
  if (ITimer.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    lastMillis = millis();
    Serial.print(F("Starting ITimer OK, millis() = "));
    Serial.println(lastMillis);
  }
  else
    Serial.println(F("Can't set ITimer correctly. Select another freq. or interval"));

Serial.println("Delta ms = " + String(millis() - lastMillis) +" "+version);
}

void loop()
{
 // ESP.wdtFeed();
  //fauxmo.handle();

  // This is a sample code to output free heap every 5 seconds
  // This is a cheap way to detect memory leaks
  /*
  static unsigned long last = millis();
  if (millis() - last > 2000)
  {
    last = millis();
    ESP.getFreeHeap();
    Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.println("");
  }
  */
}

#include <ArduinoJson.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <PubSubClient.h>
#include <Secrets.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

/**
* Reading temperature and humidity using ESP8266 and DHT22.
**/

#define DHTPIN 4      // Digital pin DHT22 is connected to
#define DHTTYPE DHT22 // Kind of DHT sensor

DHT dht(DHTPIN, DHTTYPE);

// MQTT settings (read from Secrets.h file outside repository)
// const char* MQTT_SERVER = "*****";
// const char* MQTT_USERNAME = "*****";
// const char* MQTT_PASSWORD = "*****";
const char *MQTT_CLIENT_ID = "dht22";

// WiFi settings (read from Secrets.h file outside repository)
// const char* WIFI_SSID = "*****";
// const char* WIFI_PASS = "*****";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void connectToWiFi()
{
  // Connect to Wifi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  // WiFi fix: https://github.com/esp8266/Arduino/issues/2186
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long wifiConnectStart = millis();

  while (WiFi.status() != WL_CONNECTED)
  {
    if (WiFi.status() == WL_CONNECT_FAILED)
    {
      Serial.println("Failed to connect to WIFI. Please verify credentials.");
      Serial.print("SSID: ");
      Serial.println(WIFI_SSID);
      Serial.print("Password: ");
      Serial.println(WIFI_PASS);
    }

    delay(500);
    Serial.println("...");
    // Try for 10 seconds
    if (millis() - wifiConnectStart > 10000)
    {
      Serial.println("Failed to connect to WiFi. Timeout.");
      return;
    }
  }

  Serial.println("WiFi connected!");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

const char *humidityTopic = "mgl/home/dht22/sensor/humidity";
const char *temperatureTopic = "mgl/home/dht22/sensor/temperature";
const char *heatindexTopic = "mgl/home/dht22/sensor/heatindex";
const char *statusTopic = "mgl/home/dht22/status";
const char *uptimeTopic = "mgl/home/dht22/uptime";

boolean connectToMqqtBroker()
{
  Serial.print("Connecting to MQTT broker...");

  if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD, statusTopic, 1, true, "offline"))
  {
    Serial.println("Connected!");
    client.publish(statusTopic, "online", true);
  }
  else
  {
    Serial.print("Failed. State: ");
    Serial.println(client.state());
  }

  return client.connected();
}

void publish(const char *topic, String message, boolean retained)
{
  char messageCharArray[message.length() + 1];
  message.toCharArray(messageCharArray, message.length() + 1);

  client.publish(topic, messageCharArray, retained);
}

void setup()
{
  Serial.begin(9600);
  Serial.setTimeout(2000);

  // Wait for serial to initialize.
  while (!Serial)
  {
  }

  Serial.println("Device started!");
  Serial.println("-------------------------------------");

  connectToWiFi();

  client.setServer(MQTT_SERVER, 1883);
}

int timeSinceLastRead = 0;
unsigned long lastReconnectAttemptAt = 0;
unsigned long lastSystemMessageAt = 0;

void loop()
{
  unsigned long currentMillis = millis();

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Disconnected from WiFi.");
    connectToWiFi();
  }

  if (!client.connected())
  {
    if (currentMillis - lastReconnectAttemptAt >= 5000)
    {
      lastReconnectAttemptAt = currentMillis;

      if (connectToMqqtBroker())
      {
        lastReconnectAttemptAt = 0;
      }
    }
  }
  else
  {
    client.loop();

    if (currentMillis - lastSystemMessageAt >= 60000)
    {
      lastSystemMessageAt = currentMillis;
      publish(uptimeTopic, String(currentMillis / 1000), true);
    }
  }

  // Report every 10 seconds
  if (timeSinceLastRead > 10000)
  {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t))
    {
      Serial.println("Failed to read from DHT sensor!");
      timeSinceLastRead = 0;
      return;
    }

    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);

    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" *C\t");
    Serial.print("Heat index: ");
    Serial.print(hic);
    Serial.println(" *C");

    char valueAsChar[5];
    dtostrf(h, 4, 1, valueAsChar);
    publish(humidityTopic, valueAsChar, true);

    dtostrf(t, 4, 1, valueAsChar);
    publish(temperatureTopic, valueAsChar, true);

    dtostrf(hic, 4, 1, valueAsChar);
    publish(heatindexTopic, valueAsChar, true);
    
    timeSinceLastRead = 0;
  }

  delay(100);

  timeSinceLastRead += 100;
}

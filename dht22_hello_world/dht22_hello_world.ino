#include <ArduinoJson.h>
#include <DHT.h>
#include <DHT_U.h>
#include <PubSubClient.h>

/**
* Reading temperature and humidity using ESP8266 and DHT22.
**/

#define DHTPIN 4      // Digital pin DHT22 is connected to
#define DHTTYPE DHT22 // Kind of DHT sensor

DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  Serial.begin(9600);
  Serial.setTimeout(2000);

  // Wait for serial to initialize.
  while (!Serial)
  {
  }

  Serial.println("Device Started");
  Serial.println("-------------------------------------");
  Serial.println("Running DHT!");
  Serial.println("-------------------------------------");
}

int timeSinceLastRead = 0;

void loop()
{
  // Report every 2 seconds.
  if (timeSinceLastRead > 2000)
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

    // Compute heat index in Celsius (isFahrenheit = false)
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

    timeSinceLastRead = 0;
  }
  delay(100);
  timeSinceLastRead += 100;
}
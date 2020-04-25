#include <Adafruit_CCS811.h>
#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
// http://kstobbe.dk/2019/01/28/first-sensor-array/

#include <ThingSpeak.h>

//
const char* ssid = "Own wifi SSID";
const char* password = "Own wifi Password";
WiFiClient client;

#define DHTPIN 13        //pin where the dht11 is connected
DHTesp dht;

uint16_t eCo2 = 0U;
uint16_t tvoc = 0U;

static Adafruit_CCS811 ccs;

/* 
After you have created a thingspeak channel with 4 fields 
https://nl.mathworks.com/help/thingspeak/collect-data-in-a-new-channel.html

Put in your own channelNumber and api write key
*/
unsigned long channelNumber = "channelNumber"; //channel nummer
const char * aPIKey = "aPIKey"; // API Key

void setup() {
  Serial.begin(115200);

  // setting up wifi
  delay(100);
  Serial.print("Connecting to ");
  Serial.println("xxxxxxxx"); 
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid,password);   //your password
  //You will hang up in this loop until you connect...
  while (WiFi.status() != WL_CONNECTED) { delay(200); Serial.print("."); }
  Serial.print("\r\nConnected: local ip address is http://");
  Serial.println(WiFi.localIP());
  Serial.println();

  // setting up DHT
  dht.setup(DHTPIN, DHTesp::DHT22); // Connect DHT sensor to GPIO 17
  ThingSpeak.begin(client);

  // setting up CCS811
  if ( !ccs811_init() ) {
    Serial.println("CRITICAL: Failed to start CCS811 CO2/TVOC sensor - check wiring.");
    while(1);
  }
}

void loop() {
  float temperature = NAN;
  float humidity = NAN;

  // get dht data
  if (!getDhtData(&temperature, &humidity)) {
    return;
  }

  // read CCS811 and updates data
  ccs811_read( temperature, humidity);

  // send to thingspeak
  updateThingSpeak(temperature, humidity);

  delay(20000U);
}


bool getDhtData(float* temperature, float* humidity) {
  delay(dht.getMinimumSamplingPeriod());

  *humidity = dht.getHumidity();
  *temperature = dht.getTemperature();

  if (dht.getStatusString()) {
    return true;
  }

  return false;
}

// setup ccs811
bool ccs811_init() {
  bool result = true;
  if (ccs.begin()) {
    ccs.setDriveMode(CCS811_DRIVE_MODE_10SEC);
  } else {
    result = false;
  }

  return result;
}

// read ccs811 and updates Environmental Data
void ccs811_read( float temp, float hum) {
  if (!isnan(temp) && !isnan(hum)) {
    ccs.setEnvironmentalData((uint8_t)hum, (double)temp);
  }
  if (0U == ccs.readData()) {
    uint16_t tempCo2 = ccs.geteCO2();
    uint16_t tempTvoc = ccs.getTVOC();
    if ((tempCo2 + tempTvoc) > 0U) {
      eCo2 = tempCo2;
      tvoc = tempTvoc;
    }
  }
}

// send to thingspeak
void updateThingSpeak( float temperature, float humidity) {
  ThingSpeak.setField( 1, temperature);
  ThingSpeak.setField( 2, humidity);
  ThingSpeak.setField( 3, eCo2);
  ThingSpeak.setField( 4, tvoc);
  ThingSpeak.writeFields(channelNumber, aPIKey);
}

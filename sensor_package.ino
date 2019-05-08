// Sensor platform for HCDE 440 19sp Midterm Project
// This file gathers weather data from the DHT and MPL115A2 sensors
// and publishes the data to an MQTT server with the topic AlexBanh/ambientdisplay
/////

#include <ESP8266WiFi.h>    //Requisite Libraries . . .
#include "Wire.h"           //
#include <PubSubClient.h>   //
#include <ArduinoJson.h>    //

//DHT required libraries
#include "DHT.h"
//MPL115A2 required libraries
#include <Wire.h>
#include <Adafruit_MPL115A2.h>


#define wifi_ssid "University of Washington"   //You have seen this before
#define wifi_password "" //

#define DHTPIN 12     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Initialize MPL115A2 sensor
Adafruit_MPL115A2 mpl115a2;

//////////
//So to clarify, we are connecting to and MQTT server
//that has a login and password authentication
//I hope you remember the user and password
//////////

#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

//////////
//We also need to publish and subscribe to topics, for this sketch are going
//to adopt a topic/subtopic addressing scheme: topic/subtopic
//////////

WiFiClient espClient;             //blah blah blah, espClient
PubSubClient mqtt(espClient);     //blah blah blah, tie PubSub (mqtt) client to WiFi client

//////////
//We need a 'truly' unique client ID for our esp8266, all client names on the server must be unique.
//Every device, app, other MQTT server, etc that connects to an MQTT server must have a unique client ID.
//This is the only way the server can keep every device separate and deal with them as individual devices/apps.
//The client ID is unique to the device.
//////////

char mac[6]; //A MAC address is a 'truly' unique ID for each device, lets use that as our 'truly' unique user ID!!!

//////////
//In our loop(), we are going to create a c-string that will be our message to the MQTT server, we will
//be generous and give ourselves 200 characters in our array, if we need more, just change this number
//////////

char message[201]; //201, as last character in the array is the NULL character, denoting the end of the array

unsigned long currentMillis, timerOne, timerTwo, timerThree; //we are using these to hold the values of our timers

/////SETUP/////
void setup() {
  Serial.begin(115200);
  setup_wifi();
  // Set the MQTT server and port
  mqtt.setServer(mqtt_server, 1883);
  // Initialize our DHT sensor
  dht.begin();
  // Initialize our MPL115A2 sensor
  mpl115a2.begin();
}

/////SETUP_WIFI/////
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  Serial.println(WiFi.macAddress());  //.macAddress returns a byte array 6 bytes representing the MAC address
}                                     //5C:CF:7F:F0:B0:C1 for example

/////CONNECT/RECONNECT/////Monitor the connection to MQTT server, if down, reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("AlexBanh/ambientdisplay");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/////LOOP/////
void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }

  mqtt.loop(); //this keeps the mqtt connection 'active'

  //Here we will deal with a JSON string

      // Reading temperature or humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      float h = dht.readHumidity();
      // Read temperature as Celsius (the default)
      float t = dht.readTemperature();
      // Read temperature as Fahrenheit (isFahrenheit = true)
      float f = dht.readTemperature(true);

        // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  }

  // Variables initalized to store the current pressure and temperature
  float pressureKPA = 0, temperatureC = 0;

  // Gets and sets both the pressure and temperature variables with data from the MPL115A2 sensor
  mpl115a2.getPT(&pressureKPA,&temperatureC);


    char str_temp[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character
    char str_humd[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character
    char str_pres[7]; //a temp array of size 7 to hold "XXX.XX" + the terminating character

    //take temp, format it into 5 char array with a decimal precision of 2, and store it in str_temp
    dtostrf(f, 5, 2, str_temp);
    //ditto
    dtostrf(h, 5, 2, str_humd);
    //ditto2
    dtostrf(pressureKPA, 6, 2, str_pres);

    /////
    //For proper JSON, we need the "name":"value" pair to be in quotes, so we use internal quotes
    //in the string, which we tell the compiler to ignore by escaping the inner quotes with the '/' character
    /////

    // Format our string
    sprintf(message, "{\"temp\":\"%s\", \"humd\":\"%s\", \"pres\":\"%s\"}", str_temp, str_humd, str_pres);
    // Publish the string representing JSON data to the AlexBanh/ambientdisplay topic on the MQTT server
    mqtt.publish("AlexBanh/ambientdisplay", message);

    // 10 second delay so we don't overload the server
    delay(10000);


}//end Loop

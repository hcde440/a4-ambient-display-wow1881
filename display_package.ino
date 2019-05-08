/*
 * Retrieves calendar events from a google script using the HTTPSRedirect library
 * and also pulls Temperature, Humidity, and Pressure data from a MQTT server
 * then publishes all the information to an AdafruitIO dashboard.
 *
 * Platform: ESP8266 using Arduino IDE
 * Based on: http://www.coertvonk.com/technology/embedded/esp8266-clock-import-events-from-google-calendar-15809
 * with use of the HTTPSRedirect client from https://github.com/electronicsguy/ESP8266/tree/master/HTTPSRedirect
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <HTTPSRedirect.h>
#include "config.h"
#include <ArduinoJson.h>

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
PubSubClient mqtt(espClient); //blah blah blah, tie PubSub (mqtt) client to WiFi client

char mac[6]; //A MAC address is a 'truly' unique ID for each device, lets use that as our 'truly' unique user ID!!!

unsigned long currentMillis, previousMillis; //we are using these to track the interval for our weather data packages

// replace with your network credentials
char const * const ssid = "University of Washington";   // ** UPDATE ME **
char const * const passwd = "";   // ** UPDATE ME **

// fetch events from Google Calendar
const char* dstHost = "script.google.com";
// Unique url path to my custom google script (gives access to see my next x calendar events)
// Note for the instructors: I left out my url for security reasons as it gives access to my calendar events!
// If you would like the url just let me know :)
char const * const dstPath = "/macros/s/unique-url-that-I-left-out-here/exec";
int const dstPort = 443;
int32_t const timeout = 5000;

// Initialize the adafruitIO feed for the events
AdafruitIO_Feed *events = io.feed("events");

// Initialize the adafruitIO feed for the temperature
AdafruitIO_Feed *temperature = io.feed("temperature");

// Initialize the adafruitIO feed for the humidity
AdafruitIO_Feed *humidity = io.feed("humidity");

// Initialize the adafruitIO feed for the pressure
AdafruitIO_Feed *pressure = io.feed("pressure");

void setup() {
  // Begin our serial monitor
  Serial.begin(115200); delay(500);

  Serial.print("This board is running: ");
  Serial.println(F(__FILE__));                            //These four lines give description of of file name and date
  Serial.print("Complied: ");
  Serial.println(F(__DATE__ " " __TIME__));

  // Connect to the wifi
  Serial.print("\n\nWifi ");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" done");

  // Connect to AdafruitIO
  io.connect();

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());

  // Set our MQTT server
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback); //register the callback function
}

/////CONNECT/RECONNECT/////Monitor the connection to MQTT server, if down, reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("AlexBanh/ambientdisplay"); //we are subscribing to 'theTopic' and all subtopics below that topic
    } else {                        //please change 'theTopic' to reflect your topic you are subscribing to
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  // Initialize our interval timer for sending calendar data
  unsigned long currentMillis = millis();

  // Run adafruitIO
  io.run();

  if (!mqtt.connected()) {
    reconnect(); //Reconnect if lost connection
  }

  mqtt.loop(); //this keeps the mqtt connection 'active'

  if (currentMillis - previousMillis > 360000) { //a periodic report, every hour
    // Create a new HTTPSRedirect client
    HTTPSRedirect client;
    // Connect to our host with the default port of 443
    client.connect(dstHost, dstPort);
    // We don't want our response body printed
    client.setPrintResponseBody(false);
    // Get the response from our url (custom api script)
    client.GET(String(dstPath), dstHost);
    // Store the response body in a string
    String upcomingEvents = client.getResponseBody();
    // Print the string to serial for debugging purposes
    // Serial.println(upcomingEvents);
    // Publish the string to our events feed on adafruitIO
    events->save(upcomingEvents);
    previousMillis = currentMillis;
  }
}

/////CALLBACK/////
//The callback is where we attacch a listener to the incoming messages from the server.
//By subscribing to a specific channel or topic, we can listen to those topics we wish to hear.
//We place the callback in a separate tab so we can edit it easier . . . (will not appear in separate
//tab on github!)
/////

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.println();
  Serial.println("//////////////////");                 //To seperate the sends
  Serial.println();

  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); //'topic' refers to the incoming topic name, the 1st argument of the callback function
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer; //blah blah blah a DJB
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  if (!root.success()) { //well?
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }

  // Record values from JSON and store them as strings
  String temp = root["temp"].as<String>();
  String humd = root["humd"].as<String>();
  String pres = root["pres"].as<String>();

  // Print out the information for debugging purposes
  Serial.print("The temperature in F is: ");
  Serial.println(temp);
  Serial.print("The humidity is: ");
  Serial.println(humd);
  Serial.print("The pressure in KPa is: ");
  Serial.println(pres);

  // Save the temperature, humidity, and pressure information to our adafruitIO dashboard
  temperature->save(temp);
  humidity->save(humd);
  pressure->save(pres);

}

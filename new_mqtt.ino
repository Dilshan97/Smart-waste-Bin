/*
   Smart Public Wastebin

   this project build for IOT module in 4 year 2 semester SLIIT

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTTYPE DHT11   // DHT type (DHT 11

// route the credentials
const char* ssid = "Dialog 4G";
const char* password = "68N3L04H290";

// MQTT broker
const char* mqtt_server = "broker.emqx.io";

// Initializes the espClient.
WiFiClient espClient;
PubSubClient client(espClient);

const int DHTPin = D3;
const int lamp = D5;
const int buzzer = D8;

const int trigPin = D6;
const int echoPin = D7;

const int moisturePin = A0;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

long duration;
int distance;

// onnects ESP8266 to your router
void setup_wifi() {
  beep(50);
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
  beep(150);
}

void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  if (topic == "dilshanR/room/lamp") {
    Serial.print("Changing Room lamp to ");
    if (messageTemp == "on") {
      digitalWrite(lamp, HIGH);
      Serial.print("On");
      beep(100);
    }
    else if (messageTemp == "off") {
      digitalWrite(lamp, LOW);
      Serial.print("Off");
      beep(100);
    }
  }
  Serial.println();
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("dilshanR/room/lamp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(lamp, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(moisturePin, INPUT);

  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if (!client.loop())
    client.connect("ESP8266Client");

  now = millis();
  // Publishes new temperature and humidity every 30 seconds
  if (now - lastMeasure > 30000) {
    lastMeasure = now;
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Computes temperature values in Celsius
    float hic = dht.computeHeatIndex(t, h, false);
    static char temperatureTemp[7];
    dtostrf(hic, 6, 2, temperatureTemp);

    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);

    // Published Temperature and Humidity values
    client.publish("dilshanR/room/temperature", temperatureTemp);
    client.publish("dilshanR/room/humidity", humidityTemp);

    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t Heat index: ");
    Serial.print(hic);
    Serial.println(" *C ");


    //publish wastebin level every 10 seconds
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;

    static char trashlevel[7];
    dtostrf(distance, 6, 2, trashlevel);
    client.publish("dilshanR/wastebin/wastelevel", trashlevel);

    Serial.print("Trash Level: ");
    Serial.println(distance);

    // publish moisture level
    int moisture = analogRead(moisturePin);

    static char moisturelevel[7];
    dtostrf(moisture, 6, 2, moisturelevel);
    client.publish("dilshanR/wastebin/moisture", moisturelevel);

    Serial.print("Moisture :");
    Serial.println(moisture);
  }


}

void beep(int i) {
  digitalWrite(buzzer, HIGH);
  delay(i);
  digitalWrite(buzzer, LOW);
}

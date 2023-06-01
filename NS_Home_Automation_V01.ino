/*
Excellenta Teknologi Solution
Neo Semeru Brata
1 Jul 2020
NodeMCU ESP 8266
3 Relay
1 PIR
1 DHT
1 MQ2
MQTT 
Time

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
//#include <NTPClient.h>
//#include <WiFiUdp.h>
#include <MQ2.h>

// Update these with values suitable for your network.
//WIFI
WiFiClient espClient;
const char* ssid = "EM-10";
const char* password = "C0nn3ctwifi";
const char* mqtt_server = "broker.mqtt-dashboard.com";
PubSubClient client(espClient);

//MQTT =======================================
const char* MQTTRelay01 = "1-EM10C4/12/Relay";   //pompa shower
const char* MQTTRelay02 = "2-EM10C4/12/Relay";   //dispenser
const char* MQTTRelay03 = "3-EM10C4/12/Relay";   //kitchen set
const char* MQTTDHTTemp = "EM10C4/12/DHT/Temp"; 
const char* MQTTDHTHum = "EM10C4/12/DHT/Hum";
const char* MQTTPIR01 = "1-EM10C4/12/PIR";       //Status PIR
const char* MQTTLPG01 = "1-EM10C4/12/LPG";
const char* MQTTCO01 = "1-EM10C4/12/CO";
const char* MQTTSmoke01 = "1-EM10C4/12/Smoke";

// NodeMCU PIN
#define pinRelay01 D0
#define pinRelay02 D1
#define pinRelay03 D2
#define DHTPIN D4
#define pinPIR01 D5
#define pinMQ201 A0

//Initial Status Relays
bool R01on = LOW;
bool R02on = LOW;
bool R03on = LOW;

int state = LOW;
int val = 0;

//MQ2 =======
int lpg, co, smoke;
MQ2 mq2(pinMQ201);

//DHT ========================================
       
#define DHTTYPE DHT22     // DHT 11
float h_temp,t_temp;
float hh,tt;
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];


void sendSensor(float t, float h){
  int i = 0;
    
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.println (t_temp);
  if (t_temp != t){
    t_temp = t;
    client.publish(MQTTDHTTemp, String(t).c_str(),true);
  }
  
  Serial.println (h_temp);
  if (h_temp != h){
    h_temp = h;
    client.publish(MQTTDHTHum, String(h).c_str(),true);
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (topic[0] == MQTTRelay01[0]){
     if ((char)payload[0] == '1') {
      digitalWrite (pinRelay01,R01on);
     } else {
      digitalWrite (pinRelay01,!R01on);
     }
  }
  else if (topic[0] == MQTTRelay02[0]) {
    if ((char)payload[0] == '1') {
      digitalWrite (pinRelay02,R02on);
     } else {
      digitalWrite (pinRelay02,!R02on);
     }
  } else if (topic[0] == MQTTRelay03[0]){
    if ((char)payload[0] == '1') {
      digitalWrite (pinRelay03,R03on);
     } else {
      digitalWrite (pinRelay03,!R03on);
     }      
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      //Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish(MQTTCountR01, "Hello Nheiyo");
      // ... and resubscribe
      client.subscribe(MQTTRelay01);
      client.subscribe(MQTTRelay02);
      client.subscribe(MQTTRelay03);
    } else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode (pinRelay01,OUTPUT);
  pinMode (pinRelay02,OUTPUT);
  pinMode (pinRelay03,OUTPUT);
  pinMode (pinPIR01, INPUT);
  
  digitalWrite (pinRelay01,!R01on);
  digitalWrite (pinRelay02,!R02on);
  digitalWrite (pinRelay03,!R03on);    

  mq2.begin();
  dht.begin();

}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  unsigned long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;

    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    //DHT
    hh = dht.readHumidity();
    tt = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

    if ((h_temp != hh) || (t_temp != tt)){
      sendSensor(tt,hh);
    }

    //GAS
    float* values = mq2.read(true);
    lpg = mq2.readLPG();
    co = mq2.readCO();
    smoke = mq2.readSmoke();

    Serial.print("LPG: ");
    Serial.println(lpg);

    snprintf (msg, MSG_BUFFER_SIZE, "%2d", lpg); 
    client.publish(MQTTLPG01, msg);   
 
    Serial.print("CO: ");
    Serial.println(co);
    snprintf (msg, MSG_BUFFER_SIZE, "%2d", co); 
    client.publish(MQTTCO01, msg);   

    Serial.print("Smoke: ");
    Serial.println(smoke);
    snprintf (msg, MSG_BUFFER_SIZE, "%2d", smoke); 
    client.publish(MQTTSmoke01, msg);   
  
  //PIR
    val = digitalRead(pinPIR01);
    Serial.print ("PIR : ");
    Serial.println (val);
    if(val == HIGH){    
      delay(100);
      if (state == LOW){
        Serial.println("Motion Detected!");  
        state = HIGH;
        client.publish(MQTTPIR01,"1");
      }  
    } else {
      delay(100);
      if (state == HIGH){
        Serial.println("Motion stopped!");
        state = LOW;
        client.publish(MQTTPIR01,"0");  
      }
    }
  }
}

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <IPAddress.h>
#include <PubSubClient.h>

#define SERIAL_BAUD    115200

const char* ssid = "your_wifi";
const char* password = "your_wifi_password";

const char* clientId = "RockySwitch1";
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
// Username và password để kết nối đến MQTT server nếu server có
// bật chế độ xác thực trên MQTT server
// Nếu không dùng thì cứ để vậy
const char* mqttUsername = "<MQTT_BROKER_USERNAME>";
const char* mqttPassword = "<MQTT_BROKER_PASSWORD>";

const char* relayStatusTopics[] = {
  "/Room1/Light1/Status", 
  "/Room1/Light2/Status",
  "/Room1/Light3/Status",
  "/Room1/Light4/Status"
}; // should change to something unique

const char* relayCmdTopics[] = {
  "/Room1/Light1/Command", 
  "/Room1/Light2/Command",
  "/Room1/Light3/Command",
  "/Room1/Light4/Command"
}; // should change to something unique

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastReconnectTime = 0;
const long reconnectInterval = 30000;

const byte buttonPins[4] = {D0, D5, D6, D7};
const byte relayPins[4]  = {D1, D2, D3, D4};
byte relayStatus[4] = {0, 0, 0, 0};

// used for button debouncing
const byte debounceDelay = 100; // Debounce time;
unsigned long lastDebounceTime = 0; //The last time button was toggled
byte currentButtonStatus[4];
byte lastButtonStatus[4];

void setup() {
  Serial.begin(SERIAL_BAUD);

  Serial.println("Rocky Switch is starting");

  // Configure input pins and output pins
  for(int i=0; i<sizeof(relayPins); i++)
  {
    pinMode(relayPins[i], OUTPUT); // Configure pin2,3,4 as an output 
    pinMode(buttonPins[i], INPUT); // Configure pin5,6,7 as an pull-up input
    digitalWrite(relayPins[i], HIGH); //init OFF all relay
    lastButtonStatus[i] = 0;
  }

  setup_wifi();

  client.setServer(mqttServer, mqttPort);
  client.setCallback(onMessageReceived);
  reconnect();

  // Waiting for connection ready before sending update
  delay(500); 
  sendStatusUpdate();
}

void loop() {

  if (client.connected()) {
    client.loop();
  }

  // Read input pins to check button is clicked or not
  boolean buttonPressed = false;
  for (byte i=0; i<sizeof(buttonPins); i++)
  {
    byte reading = digitalRead(buttonPins[i]); // Read the state of the switch

    if( reading == HIGH)                 
    {
      relayStatus[i] = 1 - relayStatus[i];
      digitalWrite(relayPins[i], 1 - relayStatus[i]);
      Serial.print("Changed relay "); Serial.print(i); Serial.print(" status to "); Serial.println(relayStatus[i]);
      sendSingleStatusUpdate(i);
    }
    lastButtonStatus[i] = reading;
  }
}

void setup_wifi() {
  Serial.print("Connecting to "); Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void onMessageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  for(int i=0; i<sizeof(relayPins); i++) {
    if (strcmp(topic, relayCmdTopics[i]) == 0) {
      Serial.print("Received message for relay "); Serial.println(i);
      if ((char)payload[0] == '1') {
        Serial.println("Command is to turn on");
        digitalWrite(relayPins[i], LOW);
        relayStatus[i] = HIGH;
      } else if ((char)payload[0] == '0') {
        Serial.println("Command is to turn off");
        digitalWrite(relayPins[i], HIGH);
        relayStatus[i] = LOW;
      } else {
        Serial.print("Invalid command received: "); Serial.println(payload[0]);
      }
      break;
    }
  }
}


void reconnect() {
  Serial.print("Attempting MQTT connection...");
  // Attempt to connect
  if (client.connect(clientId, mqttUsername, mqttPassword)) {
    Serial.println("connected");
    for(int i=0; i<sizeof(relayPins); i++) {
      Serial.print("Subscribe to topic: "); Serial.print(relayCmdTopics[i]);
      client.subscribe(relayCmdTopics[i]);
      Serial.println("... done");
      delay(200);
    }
    Serial.println("Subscribed all topics to broker");
    //sendStatusUpdate();
  } else {
    Serial.print("failed, rc="); Serial.println(client.state());
  }
}


/*  when a relay is manually switched by button, this sends all status update
    to server
*/

void sendStatusUpdate()
{  
  for(int i=0; i<sizeof(relayStatus); i++) {
    Serial.println("Sending status update to broker");
    char status [4];
    sprintf (status, "%d", relayStatus[i]);
    client.publish(relayStatusTopics[i], status);
    delay(50);
  }
}


void sendSingleStatusUpdate(int relayNo)
{  
  Serial.println("Sending status update to broker");
  char status [4];
  sprintf (status, "%d", relayStatus[relayNo]);
  client.publish(relayStatusTopics[relayNo], status);
}

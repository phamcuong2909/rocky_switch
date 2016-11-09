#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <IPAddress.h>
#include <PubSubClient.h>

// Khai báo các chân nối với nút cảm ứng và relay
const byte buttonPins[4] = {D7, D6, D5, D0};
const byte relayPins[4]  = {D1, D2, D3, D4};

const char* WIFI_SSID = "Sandiego";
const char* WIFI_PWD = "0988807067";

// Cấu hình cho giao thức MQTT
const char* clientId = "RockySwitch1";
const char* mqttServer = "192.168.1.110";
const int mqttPort = 1883;
// Username và password để kết nối đến MQTT server nếu server có
// bật chế độ xác thực trên MQTT server
// Nếu không dùng thì cứ để vậy
const char* mqttUsername = "<MQTT_BROKER_USERNAME>";
const char* mqttPassword = "<MQTT_BROKER_PASSWORD>";

// MQTT topic để cập nhật trạng thái công tắc về server
const char* relayStatusTopics[] = {
  "/easytech.vn/LivingRoom/Light1/Status", 
  "/easytech.vn/LivingRoom/Light2/Status",
  "/easytech.vn/LivingRoom/Light3/Status",
  "/easytech.vn/LivingRoom/Light4/Status",
};

// MQTT topic để nhận lệnh điều khiển bật tắt công tắc từ server
const char* relayCmdTopics[] = {
  "/easytech.vn/LivingRoom/Light1/Command", 
  "/easytech.vn/LivingRoom/Light2/Command",
  "/easytech.vn/LivingRoom/Light3/Command",
  "/easytech.vn/LivingRoom/Light4/Command"
};

WiFiClient espClient;
PubSubClient client(espClient);

// Lưu trạng thái hiện tại của relay
byte relayStatus[4] = {0, 0, 0, 0};

void setup() {
  Serial.begin(115200);

  // Thiết lập chế độ hoạt động của các chân nối với relay và công tắc cảm ứng
  for(int i=0; i<sizeof(relayPins); i++)
  {
    pinMode(relayPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT);
    digitalWrite(relayPins[i], HIGH); //tắt các relay khi bắt đầu
  }

  connectWifi();
}

void loop() {

  // Kiểm tra xem có message từ MQTT server
  if (client.connected()) {
    client.loop();
  }

  // Đọc trạng thái các nút nhấn và xử lý
  boolean buttonPressed = false;
  for (byte i=0; i<sizeof(buttonPins); i++)
  {
    byte reading;

    reading = digitalRead(buttonPins[i]);

    // Nếu phát hiện nút nào được nhấn thì đảo trạng thái relay về 
    // cập nhật trạng thái về server
    if( reading == HIGH)                 
    {
      // Đảo chiều trạng thái, nếu relay đang bật (LOW, 0) thì thành tắt (HIGH, 1)
      relayStatus[i] = 1 - relayStatus[i];
      digitalWrite(relayPins[i], 1 - relayStatus[i]);
      Serial.print("Thay doi trang thai relay "); Serial.print(i); Serial.print(" thanh "); Serial.println(relayStatus[i]);
      sendSingleStatusUpdate(i);
      delay(500);
    }
  } 
}

void connectWifi() {
  Serial.print("Dang ket noi wifi "); Serial.print(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PWD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("thanh cong.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Ket noi toi MQTT server...");

  client.setServer(mqttServer, mqttPort);
  client.setCallback(onMessageReceived);
 
  if (client.connect(clientId, mqttUsername, mqttPassword)) {
    Serial.println("thanh cong");
    
    for(int i=0; i<sizeof(relayPins); i++) {
      Serial.print("Dang ky nhan lenh dieu khien tu server thong qua topic: "); Serial.print(relayCmdTopics[i]);
      client.subscribe(relayCmdTopics[i]);
      Serial.println("... thanh cong");
      delay(200);
    }

    // Gửi cập nhật trạng thái về server khi ket noi thanh cong
    sendStatusUpdate();
    
  } else {
    Serial.print("that bai, error code ="); Serial.println(client.state());
  }
}

void onMessageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nhan dc MQTT message [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  for(int i=0; i<sizeof(relayPins); i++) {
    if (strcmp(topic, relayCmdTopics[i]) == 0) {
      if ((char)payload[0] == '1') {
        Serial.print("Nhan duoc lenh bat relay "); Serial.println(i);
        digitalWrite(relayPins[i], LOW);
        relayStatus[i] = HIGH;
      } else if ((char)payload[0] == '0') {
        Serial.print("Nhan duoc lenh tat relay "); Serial.println(i);
        digitalWrite(relayPins[i], HIGH);
        relayStatus[i] = LOW;
      } else {
        Serial.print("Lenh nhan duoc khong hop le: "); Serial.println(payload[0]);
      }
      break;
    }
  }
}

/*  
 *   Cập nhật trạng thái của tất cả relay về server
*/

void sendStatusUpdate()
{  
  Serial.println("Cap nhat trang thai tat ca relay ve server");
  for(int i=0; i<sizeof(relayStatus); i++) {
    char status [4];
    sprintf (status, "%d", relayStatus[i]);
    client.publish(relayStatusTopics[i], status);
    delay(50);
  }
}


/*  
 *   Cập nhật trạng thái của 1 relay về server
*/

void sendSingleStatusUpdate(int relayNo)
{  
  Serial.print("Cap nhat trang thai relay "); Serial.print(relayNo); Serial.println(" ve server");
  char status [4];
  sprintf (status, "%d", relayStatus[relayNo]);
  client.publish(relayStatusTopics[relayNo], status);
}

#include <ESP8266WiFi.h>
#include <string.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

// Khai báo các chân nối với nút cảm ứng và relay
const byte buttonPins[4] = {D7, D6, D5, D0};
const byte relayPins[4]  = {D1, D2, D3, D4};

int relayStatus[] = {0, 0, 0, 0};

byte relaysInfo[][7] = { 
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0}, 
};

// Khai báo web server hỗ trợ bật tắt qua giao diện web
ESP8266WebServer server(80);

// Cấu hình thư viện NTP để lấy giờ hiện tại từ Internet
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);
static const char ntpServerName[] = "us.pool.ntp.org";
const int timeZone = 7; // GMT của Việt Nam, GMT+7
time_t lastClockUpdateTime = 0; // Lưu thời gian lần cuối cập nhật đồng hồ

String content = "<!DOCTYPE html>"
"<html lang='en' > "
"<head> "
"    <meta name = 'viewport' content = 'width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0'> "
"    <title>Smart Home Of Bao Quoc</title> "
"    <script src='https://cdnjs.cloudflare.com/ajax/libs/jquery/3.1.1/jquery.min.js' integrity='sha256-hVVnYaiADRTO2PzUGmuLJr8BLUSjGIZsDYGmIJLv2b8=' crossorigin='anonymous'></script> "
"    <script src='https://cdnjs.cloudflare.com/ajax/libs/angular.js/1.6.0/angular.min.js' integrity='sha256-GLClIJWIFuZzDwfYm61IwyRLzobEmISkmMvJ76zDp1s=' crossorigin='anonymous'></script>"
"    <!-- Latest compiled and minified CSS --> "
"    <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css' integrity='sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u' crossorigin='anonymous'> "
"    <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap-theme.min.css' integrity='sha384-rHyoN1iRsVXV4nD0JutlnGaslCJuC7uwjduW9SVrLvRYooPp2bWYgmgJQIXwl/Sp' crossorigin='anonymous'> "
"    <link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/bootstrap-datetimepicker/4.17.43/css/bootstrap-datetimepicker-standalone.min.css' integrity='sha256-+CTjwODD2mYru0lguUnWuJ0c6zYdassaASkIFVtD5mY=' crossorigin='anonymous' /> "
"    <script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js' integrity='sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa' crossorigin='anonymous'></script> "
"    <script type='text/javascript' src='https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.15.1/moment.min.js' integrity='sha256-4PIvl58L9q7iwjT654TQJM+C/acEyoG738iL8B8nhXg=' crossorigin='anonymous'></script> "
"    <script type='text/javascript' src='https://cdnjs.cloudflare.com/ajax/libs/bootstrap-datetimepicker/4.17.43/js/bootstrap-datetimepicker.min.js' integrity='sha256-I8vGZkA2jL0PptxyJBvewDVqNXcgIhcgeqi+GD/aw34=' crossorigin='anonymous'></script> "
"    <script type='text/javascript'> "
"        var app = angular.module('myApp', []);"
"        app.controller('myCtrl', function($scope, $http, $interval) {"
"            $scope.switches = [{status: 0, timer: 0, repeat: 0, on: '06:00', onHour: 6, onMin: 0, off: '18:00', offHour:18, offMin: 0},"
"                {status: 0, timer: 0, repeat: 0, on: '06:00', onHour: 6, onMin: 0, off: '18:00', offHour:18, offMin: 0},"
"                {status: 0, timer: 0, repeat: 0, on: '06:00', onHour: 6, onMin: 0, off: '18:00', offHour:18, offMin: 0},"
"                {status: 0, timer: 0, repeat: 0, on: '06:00', onHour: 6, onMin: 0, off: '18:00', offHour:18, offMin: 0}];"
"                "
"            $scope.time = moment().format('HH:mm');"
"            $scope.date = moment().format('DD/MM/YYYY');"
"            "
"            $scope.loggedin = false;"
"                "
"            $http.get('/status').then(function(response) {"
"                console.log(response.data);"
"                for(var i=0; i<response.data.length; i++) {"
"                    $scope.switches[i].status = response.data[i].status;"
"                    $scope.switches[i].timer = response.data[i].timer;"
"                    $scope.switches[i].repeat = response.data[i].repeat;"
"                    $scope.switches[i].on = response.data[i].on;"
"                    $scope.switches[i].off = response.data[i].off;"
"                    $scope.ip = response.data[i].ip;"
"                }"
"            });"
"            "
"            $interval(function() {"
"                $scope.time = moment().format('HH:mm');"
"                $scope.date = moment().format('DD/MM/YYYY');"
"                $http.get('/status').then(function(response) {"
"                    console.log(response.data);"
"                    for(var i=0; i<response.data.length; i++) {"
"                        $scope.switches[i].status = response.data[i].status;"
"                    }"
"                });"
"            }, 5000);"
"            "
"            $scope.save = function() {"
"                for(var i=0; i<$scope.switches.length; i++) {"
"                    $scope.switches[i].onHour = parseInt($scope.switches[i].on.split(':')[0]);"
"                    $scope.switches[i].onMin = parseInt($scope.switches[i].on.split(':')[1]);"
"                    $scope.switches[i].offHour = parseInt($scope.switches[i].off.split(':')[0]);"
"                    $scope.switches[i].offMin = parseInt($scope.switches[i].off.split(':')[1]);"
"                }"
"                console.log($scope.switches);"
"                $http({"
"                    url: '/save',"
"                    method: 'POST',"
"                    headers: { 'Content-Type': 'application/json' },"
"                    data: $scope.switches"
"                }).then(function(data) {"
"                    console.log(data);"
"                });"
"            };"
"            "
"            $scope.changeStatus = function(index) {"
"                var new_state = 1 - $scope.switches[index].status;"
"                $scope.switches[index].status = new_state;"
"                var params = {"
"                    relayNo: index,"
"                    action: new_state"
"                };"
"                $http.get('/control', "
"                    {"
"                        params: params"
"                    }).then(function(data) {"
"                        console.log(data);"
"                    });"
"            };"
"            "
"            $scope.login = function() {"
"                if ($scope.username == 'admin' && $scope.password == 'admin')"
"                    $scope.loggedin = true;"
"                else "
"                    alert('Invalid username or password');"
"                    "
"            };"
"        });"
"        "
"        $(function () { "
"            /*$('[id^=timePicker').datetimepicker({ "
"                format: 'HH:mm' "
"            });*/"
"        }); "
"    </script> "
"</head> "
"<body ng-app='myApp' ng-controller='myCtrl'> "
"    <div class='container col-xs-6 col-xs-offset-3'> "
"        <div class='page-header' align='center'> "
"          <h1>SMART HOME OF BAO QUOC</h1> "
"          <div class='container'>"
"              <div class='row'>"
"                <div class='col-sm-2'>"
"                    <button class='btn btn-lg btn-primary' ng-click='login()' ng-show='!loggedin' >Login</button>"
"                    <input type='text' class='form-control' name='username' ng-model='username' placeholder='Username' required='' autofocus='' ng-show='!loggedin' />"
"                    <input type='password' class='form-control' name='password' ng-model='password' placeholder='Password' required='' ng-show='!loggedin'/>"
"                </div>"
"                <div class='col-sm-3'>"
"                  <h3>{{time}} <small> {{date}}</small></h3> "
"                  <h4>{{ip}}</h4> "
"                </div>"
"                <div class='col-sm-3'>"
"                </div>"
"              </div>"
"          </div>"
"        </div> "
"        <!--<form method='post'> -->"
"            <div class='container'>"
"              <div class='row'>"
"                <div class='col-sm-1'>                  "
"                </div>"
"                <div class='col-sm-1'>"
"                  Status"
"                </div>"
"                <div class='col-sm-2'>"
"                  Timer"
"                </div>"
"                <div class='col-sm-2'>"
"                  ON"
"                </div>"
"                <div class='col-sm-2'>"
"                  OFF"
"                </div>"
"              </div>"
"              "
"              <div class='row' ng-repeat='switch in switches track by $index'>"
"                <div class='col-xs-1'>"
"                    Switch {{$index+1}}"
"                </div>"
"                <div class='col-xs-1'>"
"                  <button type='button' class='btn btn-success' ng-if='switch.status' ng-click='changeStatus($index)' ng-disabled='!loggedin'>On</button>"
"                  <button type='button' class='btn btn-danger' ng-if='!switch.status' ng-click='changeStatus($index)' ng-disabled='!loggedin'>Off</button>"
"                </div>"
"                <div class='col-xs-2'>"
"                  <input type='checkbox' ng-model='switch.timer' ng-true-value='1' ng-false-value='0' ng-disabled='!loggedin'> Enabled "
"                  <br><input type='checkbox' ng-model='switch.repeat' ng-true-value='1' ng-false-value='0' ng-disabled='!loggedin'> Repeat"
"                </div>"
"                <div class='col-xs-2'>"
"                  <div class='input-group date' id='timePicker{{$index+1}}1'> "
"                        <input type='text' class='form-control' name='time' ng-model='switch.on' ng-disabled='!loggedin' /> "
"                        <!--<span class='input-group-addon'> "
"                            <span class='glyphicon glyphicon-time'></span> "
"                        </span>--> "
"                    </div>"
"                </div>"
"                <div class='col-xs-2'>"
"                    <div class='input-group date' id='timePicker{{$index+1}}2'> "
"                        <input type='text' class='form-control' name='time' ng-model='switch.off' ng-disabled='!loggedin'  /> "
"                        <!--<span class='input-group-addon'> "
"                            <span class='glyphicon glyphicon-time'></span> "
"                        </span>--> "
"                    </div>"
"                </div>"
"              </div>"
"              "
"            </div>"
"            <div class='row' align='center'>"
"                <button ng-click='save()' class='btn btn-primary'>Save</button> "
"            </div>"
"        <!--</form> -->"
"    </div> "
"</body> "
"</html> ";

void setup(void)
{
  Serial.begin(115200);

  // Thiết lập chế độ hoạt động của các chân nối với relay và công tắc cảm ứng
  for(int i=0; i<sizeof(relayPins); i++) {
    pinMode(relayPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT);
    digitalWrite(relayPins[i], HIGH); //tắt các relay khi bắt đầu
    relayStatus[i] = 0;
  }

  //saveSettings();
  //delay(1000);
  loadSettings();
  
  /*
  
  // Kết nối Wifi và chờ đến khi kết nối thành công
  WiFi.begin("Sandiego", "0988807067");

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  
  */

  WiFiManager wifiManager;
  
  // Reset cấu hình wifi
  //wifiManager.resetSettings();
  
  // Đọc wifi ssid và password từ bộ nhớ eeprom để kết nối wifi
  // Nếu không kết nối được thì bật chế độ access point với tên wifi là
  // "SmartSwitchAP" và chờ kết nối từ user để cấu hình
  wifiManager.autoConnect("SmartSwitchAP");

  // Nếu đã đến bước này thì là đã kết nối wifi thành công
  Serial.println("Wifi connected");

  // Kết nối tới NTP server dùng UDP để cập nhật thời gian
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  launchWeb();
}


void loop() {
  updateTime();
  server.handleClient();
  checkTimer();
}

/*---------------------- Web server setup ----------------------*/

void launchWeb() {
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.on("/status", handleGetStatus);
  server.on("/save", handleSave);
  
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Server started"); 
  Serial.print("Connect to http://");
  Serial.println(WiFi.localIP());
}

void handleControl() {
  if (!server.hasArg("relayNo") || !server.hasArg("action")) return returnFail("BAD ARGS");
  int relayNo = server.arg("relayNo").toInt();
  int action = server.arg("action").toInt();
  if (relayNo < 0 || relayNo > 3 || (action != 0 && action != 1)) {
    return returnFail("BAD ARGS");
  }
  relayStatus[relayNo] = action;
  relaysInfo[relayNo][0] = action;
  digitalWrite(relayPins[relayNo], 1-relayStatus[relayNo]);
  handleRoot();
}

void handleRoot()
{
  server.send(200, "text/html", content);
}

void returnJSON(String jsonString) {
  server.send(200, "application/json", jsonString);
}

void handleGetStatus() {
  Serial.println("Call get status");
  DynamicJsonBuffer jsonBuffer;
  JsonArray& root = jsonBuffer.createArray();
  
  //JsonArray& data = jsonBuffer.createArray();
  for (int i=0; i<4; i++) {
    //Serial.print(relaysInfo[i][0]); Serial.print(relaysInfo[i][1]); Serial.print(relaysInfo[i][2]); Serial.print(relaysInfo[i][3]); 
    //Serial.print(relaysInfo[i][4]); Serial.print(relaysInfo[i][5]); Serial.print(relaysInfo[i][6]);
    //Serial.println();
    //data.add(relayStatus[i]);
    JsonObject& relay = root.createNestedObject();
    relay["status"] = relaysInfo[i][0];
    relay["timer"] = relaysInfo[i][1];
    relay["repeat"] = relaysInfo[i][2];
    relay["on"] = String(relaysInfo[i][3]) + String(":") + String(relaysInfo[i][4]);
    relay["off"] = String(relaysInfo[i][5]) + String(":") + String(relaysInfo[i][6]);
    relay["ip"] = WiFi.localIP().toString();
  }
  String result; 
  root.printTo(result);
  //Serial.println(result);
  returnJSON(result);
}

void handleSave()
{
  DynamicJsonBuffer jsonBuffer;
  JsonArray& root = jsonBuffer.parseArray(server.arg(0));
  
  Serial.println("User saves new settings");
  for (int i=0; i<4; i++) {
    JsonObject& relay = root[i];  
    relay.printTo(Serial);
    relaysInfo[i][1] = int(relay["timer"]);
    relaysInfo[i][2] = int(relay["repeat"]);
    relaysInfo[i][3] = int(relay["onHour"]);
    relaysInfo[i][4] = int(relay["onMin"]);
    relaysInfo[i][5] = int(relay["offHour"]);
    relaysInfo[i][6] = int(relay["offMin"]);
    //Serial.println();
    //Serial.print(relaysInfo[i][0]); Serial.print(relaysInfo[i][1]); Serial.print(relaysInfo[i][2]); Serial.print(relaysInfo[i][3]); 
    //Serial.print(relaysInfo[i][4]); Serial.print(relaysInfo[i][5]); Serial.print(relaysInfo[i][6]);
  }
  
  saveSettings();
  
  server.send(200, "text/html", "");
}

void returnFail(String msg)
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


void saveSettings() {
  Serial.println();
  Serial.println("Writing settings to eeprom:");
  EEPROM.begin(512);
  for (int i = 0; i<24; i++) { 
    EEPROM.write(i, 0); 
  }
  EEPROM.commit();
  delay(100);

  EEPROM.begin(512);
  int pos = 0;
  for (int i=0; i<4; i++) {
    Serial.print("Relay "); Serial.print(i); Serial.print(": ");
    for (int j=1; j<7; j++) {
      EEPROM.write(pos, (byte)relaysInfo[i][j]);      
      Serial.print(relaysInfo[i][j], DEC);
      Serial.print(" ");
      pos++;
    }
    Serial.println("");
  }    
  EEPROM.commit();
}

void loadSettings() {
  Serial.println("Load settings from eeprom:");
  EEPROM.begin(512);
  delay(10);

  int pos = 0;
  for (int i=0; i<4; i++) {
    Serial.print("Relay "); Serial.print(i); Serial.print(": ");
    for (int j=1; j<7; j++) {
      relaysInfo[i][j] = -1;
      relaysInfo[i][j] = byte(EEPROM.read(pos));
      Serial.print(relaysInfo[i][j], DEC);
      Serial.print(" ");
      pos++;
    }
    Serial.println();    
  }
}

/*-------- Đoạn code cập nhật thời gian dùng NTP ----------*/

void updateTime() {
  if (timeStatus() != timeNotSet) {
    if (now() != lastClockUpdateTime) { // Chỉ update nếu thời gian đã thay đổi
      lastClockUpdateTime = now();
      Serial.print("Gio hien tai la ");
      Serial.print(hour()); Serial.print(":");
      Serial.print(minute()); Serial.print(":");
      Serial.print(second()); Serial.print(" ");

      Serial.print(day()); Serial.print("/");
      Serial.print(month()); Serial.print("/");
      Serial.print(year()); Serial.print(" ");

      switch (weekday())
      {
        case 1: Serial.print(" SUN"); break;
        case 2: Serial.print(" MON"); break;
        case 3: Serial.print(" TUE"); break;
        case 4: Serial.print(" WED"); break;
        case 5: Serial.print(" THU"); break;
        case 6: Serial.print(" FRI"); break;
        case 7: Serial.print(" SAT"); break;
      }

      Serial.println(); Serial.println();
    }
  }  
}

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  for(int i=0; i<5; i++) {
    IPAddress ntpServerIP; // NTP server's ip address

    while (Udp.parsePacket() > 0) ; // discard any previously received packets
    Serial.println("Transmit NTP Request");
    // get a random server from the pool
    WiFi.hostByName(ntpServerName, ntpServerIP);
    Serial.print(ntpServerName);
    Serial.print(": ");
    Serial.println(ntpServerIP);
    sendNTPpacket(ntpServerIP);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 2000) {
      int size = Udp.parsePacket();
      if (size >= NTP_PACKET_SIZE) {
        Serial.println("Receive NTP Response");
        Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
        unsigned long secsSince1900;
        // convert four bytes starting at location 40 to a long integer
        secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
        secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
        secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
        secsSince1900 |= (unsigned long)packetBuffer[43];
        return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      }
    }
    Serial.println("No NTP Response :-(");
  }
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void checkTimer() {
  int currentHour = hour();
  int currentMin = minute();
  for(int i=0; i<4; i++) {
    if (relaysInfo[i][1]) {
      // timer for this relay is enabled
      int onHour = relaysInfo[i][3];
      int onMin = relaysInfo[i][4];

      if (currentHour == onHour && currentMin==onMin && relaysInfo[i][0] != 1) {
        digitalWrite(relayPins[i], LOW);
        relaysInfo[i][0] = 1;
        Serial.print("Turn on relay: "); Serial.println(i);
      }
      int offHour = relaysInfo[i][5];
      int offMin = relaysInfo[i][6];
      if (currentHour == offHour && currentMin==offMin && relaysInfo[i][0] != 0) {
        digitalWrite(relayPins[i], HIGH);
        relaysInfo[i][0] = 0;
        Serial.print("Turn off relay: "); Serial.println(i);
        if (relaysInfo[i][2] == 0) {
          Serial.println("And also turn off timer as repeat is not enabled");
          relaysInfo[i][1] = 0;
          saveSettings();
        }
      }
    }
  }
}

#include <ESP8266WiFi.h>
#include <string.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

// Khai báo các chân nối với nút cảm ứng và relay
const byte buttonPins[4] = {D7, D6, D5, D0};
const byte relayPins[4]  = {D1, D2, D3, D4};

int relayStatus[] = {0, 0, 0, 0};

// Khai báo web server hỗ trợ bật tắt qua giao diện web
ESP8266WebServer server(80);

String header = "<!DOCTYPE html>"
"<html lang='en' > "
"<head> "
"    <meta name = 'viewport' content = 'width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0'> "
"    <title>Smart Home Of Bao Quoc</title> "
"    <script type='text/javascript' src='https://cdnjs.cloudflare.com/ajax/libs/jquery/3.1.1/jquery.slim.min.js' integrity='sha256-/SIrNqv8h6QGKDuNoLGA4iret+kyesCkHGzVUUV0shc=' crossorigin='anonymous'></script> "
"    <!-- Latest compiled and minified CSS --> "
"    <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css' integrity='sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u' crossorigin='anonymous'> "
"    <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap-theme.min.css' integrity='sha384-rHyoN1iRsVXV4nD0JutlnGaslCJuC7uwjduW9SVrLvRYooPp2bWYgmgJQIXwl/Sp' crossorigin='anonymous'> "
"    <script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js' integrity='sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa' crossorigin='anonymous'></script> "
"    <script type='text/javascript'> "
"        $(function () { "
"            "
"        }); "
"    </script> "
"</head> ";

String body = "<body> "
"    <div class='container col-xs-4 col-xs-offset-4'> "
"        <div class='page-header' align='center'> "
"          <h2>SMART HOME OF BAO QUOC</h2> "
"          <h4>%IP</h4> "
"        </div> "
"        <form method='post'> "
"            <div class='container'>"
"                <div class='form-group'>"
"                    Switch 1 <a href='control?relayNo=0%s1</a>"
"                </div>                "
"                <div class='form-group'>"
"                    Switch 2 <a href='control?relayNo=1%s2</a>"
"                </div>"
"                <div class='form-group'>"
"                    Switch 3 <a href='control?relayNo=2%s3</a>"
"                </div>                "
"                <div class='form-group'>"
"                    Switch 4 <a href='control?relayNo=3%s4</a>"
"                </div>"
"            </div>"
"        </form> "
"    </div> "
"</body> "
"</html> ";

String relayOnMsg = "&action=0' class='btn btn-success' role='button'>ON&nbsp;&nbsp;";
String relayOffMsg = "&action=1' class='btn btn-danger' role='button'>OFF";

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

  /*
  // Kết nối Wifi và chờ đến khi kết nối thành công
  WiFi.begin(WIFI_SSID, WIFI_PWD);

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

  launchWeb();
}


void loop() {
  server.handleClient();
}

/*---------------------- Web server setup ----------------------*/

void launchWeb() {
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  
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
  digitalWrite(relayPins[relayNo], 1-relayStatus[relayNo]);
  handleRoot();
}

void handleRoot()
{
  String newBody = body;

  newBody.replace("%IP", WiFi.localIP().toString());
  
  if (relayStatus[0]) {
    newBody.replace("%s1", relayOnMsg);
  } else {
    newBody.replace("%s1", relayOffMsg);
  }

  if (relayStatus[1]) {
    newBody.replace("%s2", relayOnMsg);
  } else {
    newBody.replace("%s2", relayOffMsg);
  }

  if (relayStatus[2]) {
    newBody.replace("%s3", relayOnMsg);
  } else {
    newBody.replace("%s3", relayOffMsg);
  }

  if (relayStatus[3]) {
    newBody.replace("%s4", relayOnMsg);
  } else {
    newBody.replace("%s4", relayOffMsg);
  }
    
  String html = header + newBody;
  server.send(200, "text/html", html);
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


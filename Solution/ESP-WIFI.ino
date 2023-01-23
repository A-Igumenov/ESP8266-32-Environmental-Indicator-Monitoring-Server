#include "MQ135.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h> 
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>                                          
#include <ESPAsyncWebServer.h> 
#include <ArduinoOTA.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include "time.h"

// new functions declaration
float MQCalibration();
void MQ_DHT_read();
void DHT_data();
String web_page_text_wraper(const String& var);
void send_events();
void Timeline();
void TimelineWeb();
void sensors_data_on_led_print();

// Sub 2 symbol to output on LCD
byte sub_two[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00110,
  0b01001,
  0b00010,
  0b00100,
  0b01111
};

// content of webpage saved inside of ESP micro scheme
const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html>
  <head>
    <title>NodeMCU Environment Data and LED Control page</title>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <link rel="icon" href="https://is5-ssl.mzstatic.com/image/thumb/Purple125/v4/53/c8/ea/53c8ea8d-124b-610d-aa1f-c37ae50ec236/source/256x256bb.jpg">
    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  </head>
  <body>
    <center>
      <h1 style='font-size:50px; font-weight:bold;'><i class="fas fa-calendar-days"></i><span class="reading" style="color:#3908fc;"><span id="date">%DATE%</span></span>
      </h1>
    </center><br>
    <table border=0px style='FONT-SIZE: 35px;margin-left:auto;margin-right:auto;'>
      <thead>
        <tr><th colspan="2">Temperature and Humidity</th></tr>
      </thead>
      <tbody>
        <tr><td style="text-align: center;"><i class="fas fa-thermometer-half" style="color:#e60707;"> Temperature</i></td><td style="text-align: right;"> <span class="reading"><span id="temp">%TEMPERATURE%</span> &deg;C</span></td></tr>
        <tr><td style="text-align: center;"><i class="fas fa-umbrella" style="color:#03e8fc;">Humidity</i></td><td style="text-align: right;"> <span class="reading"><span id="hum">%HUMIDITY%</span> &percnt;</span></td></tr>
        <tr><td style="text-align: center;"><i class="fas fa-wind" style="color:#0849fc;">CO<sub>2</sub></i></td><td style="text-align: right;"><span class="reading"><span id="co2">%CO2%</span> PPM</span></td></tr>
      </tbody>
    </table>
    <div style='margin:0 auto;text-align:center;'><p style='FONT-SIZE: 50px;'><i class="fas fa-lightbulb" style="color:#fce803;"></i> is now: <span class="reading"><span id="led">%LED_STATE%</span></span> 
    <br><br>
    <a href="/LED=ON"><button style='FONT-SIZE: 50px; HEIGHT: 200px;WIDTH: 300px; 126px; Z-INDEX: 0; TOP: 200px;'>Turn On </button></a>
    <a href="/LED=OFF"><button style='FONT-SIZE: 50px; HEIGHT: 200px;WIDTH: 300px; 126px; Z-INDEX: 0; TOP: 200px;'>Turn Off </button></a>
    <br>
    </div>
    <script>
      if (!!window.EventSource) {
        var source = new EventSource('/events');
        source.addEventListener('open', function(e) {
          console.log("Events Connected");
        }, false);
      source.addEventListener('error', function(e) {
          if (e.target.readyState != EventSource.OPEN) {
          console.log("Events Disconnected");
          }
        }, false);
      source.addEventListener('message', function(e) {
        console.log("message", e.data);
        }, false);
      source.addEventListener('date', function(e) {
        console.log("date", e.data);
        document.getElementById("date").innerHTML = e.data;
        }, false); 
      source.addEventListener('temperature', function(e) {
        console.log("temperature", e.data);
        document.getElementById("temp").innerHTML = e.data;
        }, false);
      source.addEventListener('humidity', function(e) {
        console.log("humidity", e.data);
        document.getElementById("hum").innerHTML = e.data;
        }, false);
      source.addEventListener('co2', function(e) {
        console.log("co2_state", e.data);
        document.getElementById("co2").innerHTML = e.data;
        }, false);
      source.addEventListener('led_state', function(e) {
        console.log("led_state", e.data);
        document.getElementById("led").innerHTML = e.data;
        }, false);
      }
    </script>
  </body>
  </html>
)rawliteral";

int ledPin                     = D4;            // For control of LED possibility use 2 or LED_BUILTIN or D4 value
int LED_logic_val              = 0;             // LED control logic variable

const char* ssid               = "xxxxx";       // your WiFi Name
const char* password           = "xxxxxxxx";    // Your Wifi Password
AsyncWebServer server(80);                      // Asinchronios server object with work port
AsyncEventSource events("/events");             // Asinchronios mesages events path

const char* ntpServer          = "pool.ntp.org";// Internet time server adress
const int   gmtOffset_sec      = 0;             // 0 timeline
const int   timeline           = 2;             // Yours living timeline
const int   utcOffsetInSeconds = timeline*3600; // offset time recalculation
char daysOfTheWeek[7][4]       = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}; // variable to convert weekdays from numbers to words
char FulldaysOfTheWeek[7][10]  = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; // variable to convert full weekdays from numbers to words
struct tm timeinfo;                             // Structure to fill local time data from NTP
char Time_line[40];                             // Converted to string time output variable
char Time_line_Web[40];                         // Converted to string time output variable for web

#define DHTTYPE DHT11                           // Type of DHT                                 
const int DHT11_PIN            = D3;            // DHT pin number
float   t = 0,  h = 0;                          // temperature and humidity variables 
DHT dht(DHT11_PIN, DHTTYPE);                    // DHT initialized object in aplication

#define MQ_PIN                   A0             // analog input use with MQ135
MQ135 gasSensor                = MQ135(MQ_PIN); // MQ135 object
int val                        = 0;             // raw values from MQ135 saving variable
float ppm                      = 0.;            // Direct CO2 value from MQ135
float cor_ppm                  = 0.;            // Corrected CO2 value by temperature and Humidity from MQ135

unsigned long previousMillis   = 0;             // Time loop variable store previous time value
unsigned long currentMillis    = 0;             // Time loop variable store curent time value
const long Check_Change_interval=20000;         // Time loop length 


#define I2C_ADDR                 0x3F           // I2C LCD screen adress 
#define LCD_COLUMNS              20             // LCD screen column number
#define  LCD_LINES               4              // LCD screen line number
LiquidCrystal_I2C lcd(I2C_ADDR,LCD_COLUMNS, LCD_LINES); // initialization of LCD screen object inside of sketch 

float sum                      = 0;             // to save rZero MQ135 averaged value
unsigned long int times        = 0; 
float averaged                 = 0.;            // variable to calculate MQ135 rZero averaged value
char A[20];                                     // variable to output strings on LCD 

void setup() { 
  configTime(gmtOffset_sec, utcOffsetInSeconds, ntpServer); // startup command to time synchronisation with NTP server
  Timeline();                                   // fill time info structure after synchronisation and create a text line of today's date and time to output it by request
  dht.begin();                                  // DHT11 sensor enabling 
  pinMode(MQ_PIN, INPUT);                       // Setup of MQ pin for reading data
  pinMode(ledPin, OUTPUT);                      // config LED pin to output
  digitalWrite(ledPin, HIGH);                   // disabling light on LED pin (because is used build in led - it has inverted control)
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, sub_two);                   // The Subscript 2 for output to LCD initialisation 
  lcd.setCursor(0, 0);
  sprintf(A, "Connecting to %6s", ssid);
  lcd.print(A);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  lcd.setCursor(0, 1);
  lcd.print("WiFi connected     ");
  delay(1000);
  lcd.setCursor(0, 2);
  lcd.print("Server started      ");
  delay(1000);
  lcd.setCursor(0, 2);
  lcd.print("Use this URL:       ");
  lcd.setCursor(0, 3);
  lcd.print(WiFi.localIP());
  delay(10000);
  lcd.clear();

  // Web Server Setup, Events programming 
  server.addHandler(&events);
  server.begin();             // start of Web service
  server.on("/", HTTP_GET, [ ](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, web_page_text_wraper);
    // Set ledPin according to the request
    digitalWrite(ledPin, !LED_logic_val);
    });
  server.on("/LED=ON", HTTP_GET, [ ](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, web_page_text_wraper);
    LED_logic_val = HIGH;
    // Set ledPin according to the request
    digitalWrite(ledPin, !LED_logic_val);
    });
  server.on("/LED=OFF", HTTP_GET, [ ](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, web_page_text_wraper);
    LED_logic_val = LOW;
    // Set ledPin according to the request
    digitalWrite(ledPin, !LED_logic_val);
    });
  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){ }
    // send an event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 1000);
    });
  delay(1000);
  MQ_DHT_read();                                 //data renew from sensors and from clock
  Timeline();                                    //
  send_events();                                 // first events sent to web client
  sensors_data_on_led_print();                   // data output doubling to LCD

  // OTA programming   
  ArduinoOTA.setHostname("myesp8266");           // ESP8266/32 Hostname in wifi network
  ArduinoOTA.setPassword((const char *)"123");   // OTA update password
  // OTA events programming 
  ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } 
    else {  // U_FS
      type = "filesystem";
    }
  });
  ArduinoOTA.onEnd([]() {
    //Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    //Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      //Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      //Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      //Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      //Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      //Serial.println("End Failed");
    }
  });
 }



void loop(){  
  ArduinoOTA.handle();                           // OTA checking on loop start
  currentMillis = millis();                      // Time loop, to minimize data sending on LCD and WEB page
  if (currentMillis- previousMillis >= Check_Change_interval) {
    send_events();
    sensors_data_on_led_print();
    previousMillis = millis();
  }
  
  delay(1000);
  sum+=MQCalibration();                          // Calibration calculating data
  times++;
  averaged=sum/times;
}

void Timeline(){
  getLocalTime(&timeinfo);                       // Function fill time info tm type structure after time synchronisation
  sprintf(Time_line, "%04d-%02d-%02d,%s,%02d:%02d",timeinfo.tm_year+1900, timeinfo.tm_mon+1,timeinfo.tm_mday,  String(daysOfTheWeek[timeinfo.tm_wday]),timeinfo.tm_hour,timeinfo.tm_min);
}
void TimelineWeb(){
  getLocalTime(&timeinfo);                       // Function fill time info tm type structure after time synchronisation
  sprintf(Time_line_Web, "Date: %04d-%02d-%02d, %s<br>%02d:%02d",timeinfo.tm_year+1900, timeinfo.tm_mon+1,timeinfo.tm_mday,  String(FulldaysOfTheWeek[timeinfo.tm_wday]),timeinfo.tm_hour,timeinfo.tm_min);
}

void send_events(){
    MQ_DHT_read();                               // reading of sensor data before send events to client web page
    TimelineWeb();                               // Convert time structure to text line before sending it as an event to the client web page
    // Events sending to client
    events.send("ping",NULL,millis());
    events.send(Time_line_Web,"date",millis());
    events.send(String(t,1).c_str(),"temperature",millis());
    events.send(String(h,1).c_str(),"humidity",millis());
    events.send(String(ppm,0).c_str(),"co2",millis()); // ppm / averaged for calibration data view on CO2 feald on web page
    events.send(web_page_text_wraper("LED_STATE").c_str(),"led_state",millis());
}

String web_page_text_wraper(const String& var){
  if(var == "DATE"){
    return String(Time_line_Web);
  }
  else if(var == "TEMPERATURE"){
    return String(t,1);
  }
  else if(var == "HUMIDITY"){
    return String(h,0);
  }
  else if(var == "CO2"){
    return String(ppm,0);                   // ppm or averaged for calibration data view on CO2 feald on web page
  }
  else if(var == "LED_STATE"){
    return String((LED_logic_val==0)?"OFF":"ON");
  }
  return String();
}

float MQCalibration(){
  delay(1000);
  return gasSensor.getRZero();
}

void MQ_DHT_read(){
  val     = analogRead(MQ_PIN);
  ppm     = gasSensor.getPPM();
  t       = dht.readTemperature();
  h       = dht.readHumidity();
  cor_ppm = gasSensor.getCorrectedPPM(t,h);
}

void sensors_data_on_led_print(){
  char A[20];
  MQ_DHT_read();
  Timeline();
  lcd.setCursor(0, 0);
  lcd.print(Time_line);
  lcd.setCursor(0, 1);
  sprintf(A, "H.: %3.0f%%  T.: %4.1f%cC",h,t,char(223));
  lcd.print(A); 
  lcd.setCursor(0, 2);
  sprintf(A, "CO2:      %4.0f (PPM)",ppm);
  lcd.print(A);
  lcd.setCursor(2, 2);
  lcd.write((uint8_t)0); 
  lcd.setCursor(0, 3);
  sprintf(A, "CO2 cor.: %4.0f (PPM)", cor_ppm);  // cor_ppm / averaged for calibration data view on LCD
  lcd.print(A); 
  lcd.setCursor(2, 3);
  lcd.write((uint8_t)0);
}
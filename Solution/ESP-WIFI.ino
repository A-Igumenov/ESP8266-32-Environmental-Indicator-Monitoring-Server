#include "MQ135.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h> 
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h> 
#include <ESP8266HTTPClient.h>
#include <Arduino_JSON.h>
#include <AsyncElegantOTA.h>
#include <ArduinoOTA.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include "time.h"


// new functions declaration
float   MQCalibration();
void    MQ_DHT_read_First();
void    MQ_DHT_read();
void    DHT_data();
String  web_page_text_wraper(const String& var);
void    send_events();
void    Timeline();
void    TimelineWeb();
void    sensors_data_on_led_print();
String  getSensorReadings();

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

JSONVar readings;
// content of webpage saved inside of ESP micro scheme
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>NodeMCU Environment Data and LED Control page</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
	  <link rel="icon" href="https://is5-ssl.mzstatic.com/image/thumb/Purple125/v4/53/c8/ea/53c8ea8d-124b-610d-aa1f-c37ae50ec236/source/256x256bb.jpg">
    <script src="http://cdn.rawgit.com/Mikhus/canvas-gauges/gh-pages/download/2.1.7/all/gauge.min.js"></script>
	<style>
		html        {font-family: Arial, Helvetica, sans-serif;display: inline-block; text-align: center;}
		h1          {font-size: 1.8rem; color: white;}
		p           {font-size: 1.4rem;}
		.topnav     {overflow: hidden; background-color: #0A1128;}
		body        {margin: 0;}
		.content    {padding: 5%;}
		.card-grid  {max-width: 1200px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}
		.card       {background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}
		.card-title {font-size: 1.2rem;	font-weight: bold;color: #034078}
    .my_button 		{box-shadow: 0px 0px 0px 2px #9fb4f2; background:linear-gradient(to bottom, #505359 5%, #202d3d 100%); background-color:#505359; border-radius:10px; border:1px solid #383d4d; display:inline-block;
					 cursor:pointer; color:#ffffff; font-family:Arial; font-size:25px; font-weight:bold; padding:20px 75px; text-decoration:none; text-shadow:0px 1px 0px #0c1a3b;}
    .my_button:hover{background:linear-gradient(to bottom, #202d3d 5%, #505359 100%); background-color:#202d3d;}
    .my_button:active{position:relative; top:1px;}
	</style>
  </head>
  <body>
    <div class="topnav">
      <h1>Home environment status page</h1>
    </div>
	<center>
    <h1 style="font-size:50px; font-weight:bold;"><i class="fas fa-calendar" style="color:#3908fc;"> </i>
	  <span class="reading" style="color:#3908fc;"><span id="date">%DATE%</span></span></h1>
    <h1 style="font-size:75px; font-weight:bold;"><i class="fas fa-clock" style="color:#3908fc;"> </i>
	  <span class="reading" style="color:#3908fc;"><span id="time">%TIME%</span></span>
    </h1>
    </center><br
    <div class="content">
      <div class="card-grid">
        <div class="card">
          <p class="card-title">Temperature</p>
          <canvas id="gauge-temperature"></canvas>
        </div>
    <div class="card">
          <p class="card-title">Humidity</p>
          <canvas id="gauge-humidity"></canvas>
        </div>
		<div class="card">
          <p class="card-title">CO<sub>2</sub></p>
          <canvas id="gauge-CO2"></canvas>
        </div>
      </div>
    </div><br>
	<div style='margin:5 auto;text-align:center;FONT-SIZE: 50px;'><i class="fas fa-lightbulb" style="color:#f2be02;"></i> <span class="reading" style="color:#f2be02;"><span id="led">%LED_STATE%</span></span> </div>
  <br>
  <div><a href="/LED=ON"><button class="my_button" >Turn On </button></a>
       <a href="/"><button class="my_button" >Turn Off </button></a>
  </div>     
  <br>
  <a href="/update"><button class="my_button" >Update ESP code</button></a>
  <script>
  var t,h,c,d,time,l; 
  // Create Temperature Gauge
  var gaugeTemp = new LinearGauge({
    renderTo: 'gauge-temperature',
    width: 120,
    height: 400,
    units: 'C',
    minValue: 0,
    startAngle: 90,
    ticksAngle: 180,
    maxValue: 40,
    colorValueBoxRect: "#049faa",
    colorValueBoxRectEnd: "#049faa",
    colorValueBoxBackground: "#f1fbfc",
    valueDec: 1,
    valueInt: 2,
    majorTicks: ["0","5","10","15","20","25","30","35","40"],
    minorTicks: 5,
    strokeTicks: true,
    highlights: [{"from": 30,"to": 40,"color": "rgba(200, 50, 50, .75)"}],
    colorPlate: "#fff",
    colorBarProgress: "#CC2936",
    colorBarProgressEnd: "#049faa",
    borderShadowWidth: 0,
    borders: false,
    needleType: "arrow",
    needleWidth: 2,
    needleCircleSize: 7,
    needleCircleOuter: true,
    needleCircleInner: false,
    animationDuration: 500,
    animationRule: "linear",
    barWidth: 10,
  }).draw();
  
// Create Humidity Gauge
var gaugeHum = new RadialGauge({
  renderTo: 'gauge-humidity',
  width: 300,
  height: 300,
  units: "%",
  minValue: 0,
  maxValue: 100,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueDec: 0,
  valueInt: 3,
  majorTicks: [ "0","20","40","60","80","100"],
  minorTicks: 5,
  strokeTicks: true,
  highlights: [{"from": 0,"to": 30,"color": "#e6e600"}, {"from": 30, "to": 60,"color": "#00b33c"},{"from": 60,"to": 100,"color": "#03C0C1"}],
  colorPlate: "#fff",
  borderShadowWidth: 0,
  borders: false,
  needleType: "line",
  colorNeedle: "#007F80",
  colorNeedleEnd: "#007F80",
  needleWidth: 2,
  needleCircleSize: 3,
  colorNeedleCircleOuter: "#007F80",
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 500,
  animationRule: "linear"
}).draw();

var gaugeCO2 = new RadialGauge({
  renderTo: 'gauge-CO2',
  width: 300,
  height: 300,
  units: "ppm",
  minValue: 0,
  maxValue: 5200,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueDec: 0,
  valueInt: 4,
  majorTicks: ["0","400","800","1200","1600","2000","2400","2800","3200","3600","4000","4400","4800","5200"],
  minorTicks: 5,
  strokeTicks: true,
  highlights: [{"from": 0,"to": 400,"color": "#66ff99"},{"from": 400,"to": 1000,"color": "#00b33c"},{"from": 1000,"to": 2000,"color": "#e6e600"}, {"from": 2000,"to": 5200,"color": "#e62e00"}],
  colorPlate: "#fff",
  borderShadowWidth: 0,
  borders: false,
  needleType: "line",
  colorNeedle: "#007F80",
  colorNeedleEnd: "#007F80",
  needleWidth: 2,
  needleCircleSize: 3,
  colorNeedleCircleOuter: "#007F80",
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 500,
  animationRule: "linear"
}).draw();

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
  source.addEventListener('new_readings', function(e) {
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    t = myObj.temperature; 
    h = myObj.humidity;
    c = myObj.CO2;
    d = myObj.date;
    time = myObj.time;
    l = myObj.led_state;
    gaugeTemp.value = t;
    gaugeHum.value = h;
    gaugeCO2.value = c;
    document.getElementById("date").innerHTML = d;
    document.getElementById("time").innerHTML = time;
    document.getElementById("led").innerHTML = l;   
  }, false);
  source.addEventListener('reset', function(e) {
    console.log("reset", e.data);
    gaugeTemp.value = t;
    gaugeHum.value = h;
    gaugeCO2.value = c;
    document.getElementById("date").innerHTML = d;
    document.getElementById("time").innerHTML = time;
    document.getElementById("led").innerHTML = l;   
  }, false);
    source.addEventListener('load', function(e) {
    console.log("load", e.data);
    gaugeTemp.value = t;
    gaugeHum.value = h;
    gaugeCO2.value = c;
    document.getElementById("date").innerHTML = d;
    document.getElementById("time").innerHTML = time;
    document.getElementById("led").innerHTML = l; 
  }, false);
}
	</script>
  </body>
</html>
)rawliteral";

int         ledPin             = D4;            // For control of LED possibility use 2 or LED_BUILTIN or D4 value
int         LED_logic_val      = 0;             // LED control logic variable

const char* ssid               = "xxxxx";       // your WiFi Name
const char* password           = "xxxxxxxxxxx"; // Your Wifi Password

AsyncWebServer server(80);                      // Asinchronios server object with work port
AsyncEventSource events("/events");             // Asinchronios mesages events path

const char* ntpServer          = "pool.ntp.org";// Internet time server adress
const int   gmtOffset_sec      = 0;             // 0 timeline
const int   timeline           = 2;             // Yours living timeline
const int   utcOffsetInSeconds = timeline*3600; // offset time recalculation
char        daysOfTheWeek[7][4]= {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}; // variable to convert weekdays from numbers to words
char        FulldaysOfTheWeek[7][10] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; // variable to convert full weekdays from numbers to words
struct      tm timeinfo;                        // Structure to fill local time data from NTP
char        Time_line[40];                      // Converted to string time output variable
char        Date_line_Web[40];                  // Converted to string Date output variable for web
char        Time_line_Web[40];                  // Converted to string time output variable for web

#define     DHTTYPE              DHT11          // Type of DHT                                 
const int   DHT11_PIN          = D3;            // DHT pin number
const int   DHT_PW_PIN         = D5;
float       t = 0,  h = 0;                      // temperature and humidity variables 
DHT         dht(DHT11_PIN, DHTTYPE);            // DHT initialized object in aplication
int openT = HIGH;                               // temperature/humidity sensor turn on/off logical swich

#define     MQ_PIN               A0             // analog input use with MQ135
#define     MQ_PW_PIN            D6
MQ135       gasSensor          = MQ135(MQ_PIN); // MQ135 object
int         val                = 0;             // raw values from MQ135 saving variable
float       ppm                = 0.;            // Direct CO2 value from MQ135
float       cor_ppm            = 0.;            // Corrected CO2 value by temperature and Humidity from MQ135
int openC = HIGH;                               // CO2 sensor turn on/off logical swich

unsigned long previousMillis   = 0;             // Time loop variable store previous time value
unsigned long currentMillis    = 0;             // Time loop variable store curent time value
const long Check_Change_interval=10000;         // Time loop length 
unsigned long previousMillisSensT= 0;           // Time loop variable store previous time value
const long Check_Change_interval_SensT=1*60*1000;// Temperature sensor work time 
int CLOSE_DELAY_T              = 10*60*1000;    // Temperature sensor work loop time 1 min every 10 min
unsigned long previousMillisSensC= 0;           // Time loop variable store previous time value
const long Check_Change_interval_SensC=10*60*1000;// CO2 sensor work time
int CLOSE_DELAY_C              = 30*60*1000;    // CO2 sensor work loop time 10 min every 30 min

#define     I2C_ADDR             0x3F           // I2C LCD screen adress 
#define     LCD_COLUMNS          20             // LCD screen column number
#define     LCD_LINES            4              // LCD screen line number
LiquidCrystal_I2C lcd(I2C_ADDR,LCD_COLUMNS, LCD_LINES); // initialization of LCD screen object inside of sketch 

float       sum                = 0;             // to save rZero MQ135 averaged value
unsigned long int times        = 0; 
float       averaged           = 0.;            // variable to calculate MQ135 rZero averaged value
char        A[20];                              // variable to output strings on LCD 

void setup() { 
  configTime(gmtOffset_sec, utcOffsetInSeconds, ntpServer); // startup command to time synchronisation with NTP server
  Timeline();                                   // fill time info structure after synchronisation and create a text line of today's date and time to output it by request
  dht.begin();                                  // DHT11 sensor enabling 
  pinMode(MQ_PIN, INPUT);                       // Setup of MQ pin for reading data
  pinMode(ledPin, OUTPUT);                      // config LED pin to output
  pinMode(DHT_PW_PIN, OUTPUT);                  // turn on DHT sensor power pin
  pinMode(MQ_PW_PIN, OUTPUT);                   // turn on MQ sensor power pin
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
  delay(100);
  lcd.setCursor(0, 2);
  lcd.print("Server started      ");
  delay(100);
  lcd.setCursor(0, 2);
  lcd.print("Use this URL:       ");
  lcd.setCursor(0, 3);
  lcd.print(WiFi.localIP());
  delay(10000);
  lcd.clear();
  MQ_DHT_read_First();                          // First reading of sensors data
  // Web Server Setup, Events programming 
  server.addHandler(&events);
  AsyncElegantOTA.begin(&server,"user","123");  // Start ElegantOTA web interface by adress Device_IP/update
  server.begin();                               // start of all Web services
  delay(1000);                                  // delay necessary to start all services
  // Request programming
  server.on("/", HTTP_GET, [ ](AsyncWebServerRequest *request){ // default page open event Device_IP, in same time is used for turn LED off
    request->send_P(200, "text/html", index_html, web_page_text_wraper);
    // Set ledPin according to the request and send sensors data on open page
    LED_logic_val = LOW;
    digitalWrite(ledPin, !LED_logic_val);send_events();
    });
  server.on("/LED=ON", HTTP_GET, [ ](AsyncWebServerRequest *request){ // page open event Device_IP/LED=ON, used for turn LED ON
    request->send_P(200, "text/html", index_html, web_page_text_wraper);
    LED_logic_val = HIGH;
    // Set ledPin according to the request and send sensors data on open page
    digitalWrite(ledPin, !LED_logic_val);send_events();
    });
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){ // request sending to client events page
    // Sending of JSON file to client with all generated sensors data
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  }); 
  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){ }
    // send an event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 1000);
    });
  delay(1000);
  MQ_DHT_read_First();                           // data renew from sensors and from clock
  send_events();                                 // first events sent to web client on startup
  sensors_data_on_led_print();                   // data output doubling to LCD

  // OTA programming   
  ArduinoOTA.setHostname("myesp8266");           // ESP8266/32 Hostname in wifi network
  ArduinoOTA.setPassword((const char *)"123");   // OTA update password if You need change it here 
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
  
  send_events();
}

void loop(){  
  ArduinoOTA.handle();                           // WIFI OTA checking on loop start
  currentMillis = millis();                      // Time loop, to minimize data sending on LCD and WEB page
  if (currentMillis- previousMillis >= Check_Change_interval) {
    send_events();
    sensors_data_on_led_print();
    previousMillis = millis();
  }
  delay(1000);
  if (openC==HIGH){
  sum+=MQCalibration();                          // CO2 calibration calculating data
  times++;
  averaged=sum/times;
  }
}

void Timeline(){
  getLocalTime(&timeinfo);                       // Function fill time info tm type structure after time synchronisation
  sprintf(Time_line, "%04d-%02d-%02d,%s,%02d:%02d",timeinfo.tm_year+1900, timeinfo.tm_mon+1,timeinfo.tm_mday,  String(daysOfTheWeek[timeinfo.tm_wday]),timeinfo.tm_hour,timeinfo.tm_min);
}
void TimelineWeb(){
  getLocalTime(&timeinfo);                       // Function fill time info tm type structure after time synchronisation
  sprintf(Date_line_Web, "%04d-%02d-%02d, %s",timeinfo.tm_year+1900, timeinfo.tm_mon+1,timeinfo.tm_mday,  String(FulldaysOfTheWeek[timeinfo.tm_wday]));
  sprintf(Time_line_Web, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
}

void send_events(){
    MQ_DHT_read();                               // reading of sensor data before send events to client web page
    // Events sending to client
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    
}

String web_page_text_wraper(const String& var){
  if(var == "DATE"){
    return String(Date_line_Web);                // Time_line_Web  or for test get_loc()
  }
  else if(var == "TIME"){
    return String(Time_line_Web);
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
if (!openT && millis()-previousMillisSensT > CLOSE_DELAY_T) { //300*1000
  openT=HIGH;                                    // change state
  digitalWrite(DHT_PW_PIN, openT);
  delay(2000);
  t       = dht.readTemperature();
  h       = dht.readHumidity();
  previousMillisSensT = millis();                // rearm timestamp
  } 
else if (openT && millis()- previousMillisSensT > Check_Change_interval_SensT) { //60*1000
  openT=LOW;
  digitalWrite(DHT_PW_PIN, openT);
  previousMillisSensT = millis();                // rearm timestamp
  }
if (!openC && millis()-previousMillisSensC > CLOSE_DELAY_C) { //300*1000
  openC=HIGH;                                    // change state
  digitalWrite(MQ_PW_PIN, openC);
  MQCalibration();
  delay(2000);
  val     = analogRead(MQ_PIN);
  ppm     = gasSensor.getPPM();
  cor_ppm = gasSensor.getCorrectedPPM(t,h);
  previousMillisSensT = millis();                // rearm timestamp
  } 
else if (openC && millis()- previousMillisSensC > Check_Change_interval_SensC) { //60*1000
  openC=LOW;
  digitalWrite(MQ_PW_PIN, openC);
  previousMillisSensT = millis();                // rearm timestamp
  }
}

void MQ_DHT_read_First(){
  digitalWrite(DHT_PW_PIN, HIGH);
  delay(5000);
  t       = dht.readTemperature();
  h       = dht.readHumidity();
  digitalWrite(DHT_PW_PIN, LOW);
  digitalWrite(MQ_PW_PIN, HIGH);
  delay(5000);
  MQCalibration();
  val     = analogRead(MQ_PIN);
  ppm     = gasSensor.getPPM();
  cor_ppm = gasSensor.getCorrectedPPM(t,h);
  digitalWrite(MQ_PW_PIN, LOW);
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
  sprintf(A, "CO2:%10.0f (PPM)",ppm);
  lcd.print(A);
  lcd.setCursor(2, 2);
  lcd.write((uint8_t)0); 
  lcd.setCursor(0, 3);
  sprintf(A, "CO2 cor.: %4.0f (PPM)", cor_ppm);   //averaged / cor_ppm
  lcd.print(A); 
  lcd.setCursor(2, 3);
  lcd.write((uint8_t)0);
}

String getSensorReadings(){
  MQ_DHT_read();
  TimelineWeb();
  readings["temperature"] = String(t);
  readings["humidity"] =  String(h);
  readings["CO2"] =  String(ppm);
  readings["date"] = String(Date_line_Web);
  readings["time"] = String(Time_line_Web);
  readings["led_state"] = web_page_text_wraper("LED_STATE");
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

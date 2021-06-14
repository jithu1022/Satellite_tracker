#include <Sgp4.h>
#include <Ticker.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <WiFi.h>
#include "time.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

Sgp4 sat;
Ticker tkSecond;
int sel = 0;


// Replace with your network credentials
const char* ssid = "Srujan_home";
const char* password = "Idonrknowitlol123";

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";
unsigned long epochTime; 

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

unsigned long unixtime;
float timezone = 5.5 ;  //utc + 12
int framerate;

int  year; int mon; int day; int hr; int mini; double sec;

//motor things

float azi,ele =0;
AccelStepper azimuth(AccelStepper :: FULL4WIRE, 18,12,19,13); 
AccelStepper elevation(AccelStepper:: FULL4WIRE, 21,25,22,26);
MultiStepper steppers;
long positions[2];





// HTML web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
@import url('https://fonts.googleapis.com/css2?family=Berkshire+Swash&display=swap');
body{
background: linear-gradient(to bottom, #ffffff 14%, #8acae5 100%) no-repeat center center fixed;
-webkit-background-size: cover;
  -moz-background-size: cover;
  -o-background-size: cover;
  background-size: cover;
}
.text-center {
  text-align: center;
}

.button {
  background-color: #FFFFFF; /* White*/
  border: none;
  color: #000000;
  padding: 15px 32px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}



</style>
<html>


  <head>
    
   
    
  </head>
  <body>
    <h1><center style = "font-family: Berkshire Swash ">Satellite Tracker</center></h1>
    <div class="text-center">

    <button class="button" onclick="toggleCheckbox('on')">Track</button>
    <button class="button" onclick="toggleCheckbox('off')">Reset</button>
    </div>
   <script>
  

   function toggleCheckbox(x) {
     var xhr = new XMLHttpRequest();
     xhr.open("GET", "/" + x, true);
     xhr.send();
   }
  </script>
  </body>
</html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

AsyncWebServer server(80);




void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}




void setup() {
  

  

  Serial.begin(115200);
  Serial.println();
initWiFi();
  configTime(0, 0, ntpServer);

//motor things

azimuth.setMaxSpeed(100);
elevation.setMaxSpeed(100);
steppers.addStepper(azimuth);
steppers.addStepper(elevation);
unixtime = getTime();


  
  sat.site( 13.114527,77.634103,920); //set site latitude[°], longitude[°] and altitude[m]

  char satname[] = "STARLINK-71";
  char tle_line1[] = "1 44252U 19029T   21076.88301941  .00000452  00000-0  38295-4 0  9990";  //Line one from the TLE data
  char tle_line2[] = "2 44252  53.0011  48.7296 0001383  27.9613 332.1454 15.16924686 99477";  //Line two from the TLE data
  
  sat.init(satname,tle_line1,tle_line2);     //initialize satellite parameters 

  //Display TLE epoch time
  double jdC = sat.satrec.jdsatepoch;
  invjday(jdC , timezone, true, year, mon, day, hr, mini, sec);
  Serial.println("Epoch: " + String(day) + '/' + String(mon) + '/' + String(year) + ' ' + String(hr) + ':' + String(mini) + ':' + String(sec));
  Serial.println();

  tkSecond.attach(1,Second_Tick);


   // Send web page to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Receive an HTTP GET request
  server.on("/on", HTTP_GET, [] (AsyncWebServerRequest *request) {
    
   sel = 1;
    request->send(200, "text/plain", "ok");
  });
  //recieve off
 server.on("/off", HTTP_GET, [] (AsyncWebServerRequest *request) {
    sel=0;
    request->send(200, "text/plain", "ok");
  });


  
  server.onNotFound(notFound);
  server.begin();
}




void loop() {
  




while(sel == 1)
{
  sat.findsat(unixtime);
  framerate += 1;


azi=sat.satAz*5.63;
positions[0] =azi;

ele = sat.satEl*5.63;
positions[1] = ele;


steppers.moveTo(positions);
steppers.runSpeedToPosition();



if (sel == 0)
{
   positions[0]=0;
  positions[1]=0;
  steppers.moveTo(positions);
  steppers.runSpeedToPosition();
  delay(10000);
}

}



}





void Second_Tick()
{
  unixtime += 1;
       
  invjday(sat.satJd , timezone,true, year, mon, day, hr, mini, sec);
  Serial.println(String(day) + '/' + String(mon) + '/' + String(year) + ' ' + String(hr) + ':' + String(mini) + ':' + String(sec));
  Serial.println("azimuth = " + String( sat.satAz) + " elevation = " + String(sat.satEl) + " distance = " + String(sat.satDist));
  Serial.println("latitude = " + String( sat.satLat) + " longitude = " + String( sat.satLon) + " altitude = " + String( sat.satAlt));

  switch(sat.satVis){
              case -2:
                  Serial.println("Visible : Under horizon");
                  break;
              case -1:
                  Serial.println("Visible : Daylight");
                  break;
              default:
                  Serial.println("Visible : " + String(sat.satVis));   //0:eclipsed - 1000:visible
                  break;
          }

  Serial.println("Framerate: " + String(framerate) + " calc/sec");
  Serial.println();
     
  framerate=0;
}
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ThingSpeak.h>

#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>


//================ WIFI =================

const char* ssid = "Delviaa";
const char* password = "dedel12345";


//============== THINGSPEAK =============

unsigned long channelID = 3441844;

const char* writeAPI = "TQO4792EXV0NDWQ6";

WiFiClient client;



//============== DHT21 ==================

#define DHTPIN D4
#define DHTTYPE DHT21

DHT dht(DHTPIN,DHTTYPE);



//============== SOIL SENSOR ============

#define SOIL_PIN A0



//============== RELAY ==================

#define RELAY_POMPA D5
#define RELAY_KIPAS D6




//============== LCD ====================

#define SDA_PIN D2
#define SCL_PIN D1


LiquidCrystal_I2C lcd(0x27,20,4);


//============== STATUS OUTPUT ==========

bool pompa=false;

bool kipas=false;



//============== MODE ===================

// true = AUTO
// false = MANUAL

bool autoPompa=true;

bool autoKipas=true;

//============== WEB SERVER =============

ESP8266WebServer server(80);

//============== DATA SENSOR ============

float suhu=0;

float humidity=0;

int soil=0;

unsigned long lastSensor=0;

unsigned long lastLCD=0;

unsigned long lastThingSpeak=0;

//=======================================
// UPDATE OUTPUT
//=======================================

void updateOutput(){


  // Relay aktif LOW

  digitalWrite(RELAY_POMPA,
               pompa ? LOW : HIGH);



  digitalWrite(RELAY_KIPAS,
               kipas ? LOW : HIGH);


}


//=======================================
// SETUP
//=======================================

void setup(){


Serial.begin(115200);



//============== I2C LCD ================

Wire.begin(SDA_PIN,SCL_PIN);



//============== SENSOR =================

dht.begin();


//============== LCD ====================

lcd.init();

lcd.backlight();


lcd.setCursor(0,0);

lcd.print("SMART AGRICULTURE");


lcd.setCursor(0,1);

lcd.print("WEMOS D1 R3");


delay(2000);


lcd.clear();


//============== RELAY ==================

pinMode(RELAY_POMPA,OUTPUT);

pinMode(RELAY_KIPAS,OUTPUT);



pompa=false;

kipas=false;


updateOutput();

//============== LITTLEFS ===============

if(!LittleFS.begin()){


Serial.println("LittleFS ERROR");


}

else{


Serial.println("LittleFS OK");


}

//============== WIFI ===================


WiFi.begin(ssid,password);


Serial.print("Connecting WiFi");



while(WiFi.status()!=WL_CONNECTED){


delay(500);

Serial.print(".");


}



Serial.println();

Serial.println("WiFi Connected");


Serial.print("IP : ");

Serial.println(WiFi.localIP());


//============== THINGSPEAK =============

ThingSpeak.begin(client);

//=======================================
// WEBSITE ROUTING
//=======================================


//============== INDEX ==================

server.on("/",HTTP_GET,[](){


File file = LittleFS.open("/index.html","r");


if(!file){

server.send(404,"text/plain","index missing");

return;

}


server.streamFile(file,"text/html");


file.close();


});






//============== CSS ====================

server.on("/style.css",HTTP_GET,[](){


File file = LittleFS.open("/style.css","r");


if(!file){

server.send(404,"text/plain","style missing");

return;

}


server.streamFile(file,"text/css");


file.close();


});


//============== JAVASCRIPT =============


server.on("/script.js",HTTP_GET,[](){


File file = LittleFS.open("/script.js","r");


if(!file){

server.send(404,"text/plain","script missing");

return;

}


server.streamFile(file,"application/javascript");


file.close();


});









//=======================================
// JSON DATA
//=======================================


server.on("/data",HTTP_GET,[](){



String json="{";


json+="\"suhu\":"+String(suhu)+",";


json+="\"humidity\":"+String(humidity)+",";


json+="\"soil\":"+String(soil)+",";




json+="\"pompa\":"+String(pompa?"true":"false")+",";


json+="\"kipas\":"+String(kipas?"true":"false")+",";




json+="\"modePompa\":"+String(autoPompa?"true":"false")+",";


json+="\"modeKipas\":"+String(autoKipas?"true":"false");



json+="}";



server.send(200,"application/json",json);



});


//=======================================
// MODE POMPA
//=======================================


server.on("/pompa/auto",HTTP_GET,[](){


autoPompa=true;


server.send(200,"text/plain","Pompa AUTO");


});



server.on("/pompa/manual",HTTP_GET,[](){


autoPompa=false;


server.send(200,"text/plain","Pompa MANUAL");


});


//=======================================
// MODE KIPAS
//=======================================


server.on("/kipas/auto",HTTP_GET,[](){


autoKipas=true;


server.send(200,"text/plain","Kipas AUTO");


});



server.on("/kipas/manual",HTTP_GET,[](){


autoKipas=false;


server.send(200,"text/plain","Kipas MANUAL");


});


//=======================================
// MANUAL POMPA
//=======================================


server.on("/pompa/on",HTTP_GET,[](){


if(!autoPompa){


pompa=true;


updateOutput();


}


server.send(200,"text/plain","Pompa ON");


});


server.on("/pompa/off",HTTP_GET,[](){


if(!autoPompa){


pompa=false;


updateOutput();


}


server.send(200,"text/plain","Pompa OFF");


});

//=======================================
// MANUAL KIPAS
//=======================================


server.on("/kipas/on",HTTP_GET,[](){


if(!autoKipas){


kipas=true;


updateOutput();


}


server.send(200,"text/plain","Kipas ON");


});

server.on("/kipas/off",HTTP_GET,[](){


if(!autoKipas){


kipas=false;


updateOutput();


}


server.send(200,"text/plain","Kipas OFF");


});


server.begin();


Serial.println("Web Server Started");


}

//=======================================
// LOOP
//=======================================

void loop(){


server.handleClient();




//=======================================
// BACA SENSOR ASLI
//=======================================


if(millis()-lastSensor > 3000){


lastSensor = millis();




//============== DHT21 ==================


float t = dht.readTemperature();

float h = dht.readHumidity();



if(!isnan(t)){

suhu = t;

}



if(!isnan(h)){

humidity = h;

}






//============== SOIL MOISTURE ==========


int adc = analogRead(SOIL_PIN);



Serial.print("ADC Soil : ");

Serial.println(adc);





// Sensor belum terpasang

if(adc < 100 || adc > 1000){


soil = 0;


}

else{


// Kalibrasi Wemos
// kering sekitar 850
// basah sekitar 350


soil = map(adc,850,350,0,100);


soil = constrain(soil,0,100);


}






Serial.println("======================");


Serial.print("Suhu : ");

Serial.print(suhu);

Serial.println(" C");



Serial.print("Humidity : ");

Serial.print(humidity);

Serial.println(" %");



Serial.print("Soil : ");

Serial.print(soil);

Serial.println(" %");



}








//=======================================
// AUTO POMPA
//=======================================


if(autoPompa){



if(soil < 40){


pompa = true;


}

else{


pompa = false;


}


updateOutput();


}









//=======================================
// AUTO KIPAS
//=======================================


if(autoKipas){



if(suhu > 30){


kipas = true;


}

else{


kipas = false;


}


updateOutput();


}


//=======================================
// LCD DISPLAY
//=======================================


if(millis()-lastLCD > 2000){


lastLCD = millis();

lcd.clear();

// BARIS 1

lcd.setCursor(0,0);

lcd.print("SMART AGRICULTURE");

// BARIS 2

lcd.setCursor(0,1);

lcd.print("T:");

lcd.print(suhu,1);

lcd.print("C ");

lcd.print("H:");

lcd.print(humidity,0);

lcd.print("%");

// BARIS 3

lcd.setCursor(0,2);


lcd.print("Soil:");

lcd.print(soil);

lcd.print("%");

// BARIS 4

lcd.setCursor(0,3);

lcd.print("P:");

lcd.print(pompa ? "ON " : "OFF");

lcd.print(" F:");

lcd.print(kipas ? "ON" : "OFF");

}

//=======================================
// SERIAL STATUS
//=======================================

Serial.println();

Serial.print("Mode Pompa : ");

Serial.println(autoPompa ? "AUTO":"MANUAL");

Serial.print("Pompa : ");

Serial.println(pompa ? "ON":"OFF");

Serial.print("Mode Kipas : ");

Serial.println(autoKipas ? "AUTO":"MANUAL");

Serial.print("Kipas : ");

Serial.println(kipas ? "ON":"OFF");

//=======================================
// THINGSPEAK
//=======================================

// Field:
// 1 = Suhu
// 2 = Kelembapan Udara
// 3 = Soil Moisture

if(millis()-lastThingSpeak > 15000){


lastThingSpeak = millis();


ThingSpeak.setField(1,suhu);


ThingSpeak.setField(2,humidity);


ThingSpeak.setField(3,soil);


int status = ThingSpeak.writeFields(channelID,writeAPI);


if(status == 200){


Serial.println("ThingSpeak Upload OK");


}

else{

Serial.print("ThingSpeak Error : ");

Serial.println(status);

}

}


}
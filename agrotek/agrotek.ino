#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#include <ThingSpeak.h>


//================ WIFI =================

const char* ssid = "R_2.4";
const char* password = "Ganes.115";


//============== THINGSPEAK =============

unsigned long channelID = 3441844;

const char* writeAPI = "TQO4792EXV0NDWQ6";

WiFiClient client;


//============== SENSOR =================

#define DHTPIN 4
#define DHTTYPE DHT21

DHT dht(DHTPIN, DHTTYPE);


#define SOIL_PIN 34



//============== RELAY ==================

#define RELAY_POMPA 25
#define RELAY_KIPAS 26


// Relay Active LOW

bool pompa = false;
bool kipas = false;


// Mode manual

bool manualPompa = false;
bool manualKipas = false;



//============== LCD ====================

LiquidCrystal_I2C lcd(0x27,20,4);



//============== WEB SERVER ==============

WebServer server(80);



//============== DATA ===================

float suhu = 0;
float humidity = 0;

int soil = 0;



unsigned long lastThingSpeak = 0;
unsigned long lastLCD = 0;





//=======================================
// SETUP
//=======================================

void setup(){


Serial.begin(115200);


// LCD

lcd.init();
lcd.backlight();

lcd.setCursor(0,0);
lcd.print("Smart Agriculture");

delay(1000);



// DHT

dht.begin();



// Relay

pinMode(RELAY_POMPA, OUTPUT);
pinMode(RELAY_KIPAS, OUTPUT);


digitalWrite(RELAY_POMPA,HIGH);
digitalWrite(RELAY_KIPAS,HIGH);



// LittleFS

if(!LittleFS.begin(true)){

Serial.println("LittleFS ERROR");

}

else{

Serial.println("LittleFS OK");

}



// WIFI

WiFi.begin(ssid,password);


Serial.print("Connecting");


while(WiFi.status()!=WL_CONNECTED){

delay(500);

Serial.print(".");

}


Serial.println();

Serial.println("WiFi Connected");

Serial.print("IP:");

Serial.println(WiFi.localIP());




// ThingSpeak

ThingSpeak.begin(client);





//================ ROUTING ===============


// halaman utama

server.on("/",HTTP_GET,[](){

File file = LittleFS.open("/index.html","r");

if(!file){

server.send(404,"text/plain","File Not Found");

return;

}


server.streamFile(file,"text/html");

file.close();

});




// CSS

server.on("/style.css",HTTP_GET,[](){

File file=LittleFS.open("/style.css","r");

server.streamFile(file,"text/css");

file.close();

});




// JS

server.on("/script.js",HTTP_GET,[](){

File file=LittleFS.open("/script.js","r");

server.streamFile(file,"application/javascript");

file.close();

});





// DATA JSON

server.on("/data",HTTP_GET,[](){


String json="{";


json += "\"suhu\":"+String(suhu)+",";

json += "\"humidity\":"+String(humidity)+",";

json += "\"soil\":"+String(soil)+",";

json += "\"pompa\":"+String(pompa)+",";

json += "\"kipas\":"+String(kipas);


json+="}";


server.send(200,"application/json",json);


});




//================ POMPA ================


server.on("/pompa/on",[](){

manualPompa=true;

pompa=true;

digitalWrite(RELAY_POMPA,LOW);

server.send(200,"text/plain","Pompa ON");

});



server.on("/pompa/off",[](){

manualPompa=true;

pompa=false;

digitalWrite(RELAY_POMPA,HIGH);

server.send(200,"text/plain","Pompa OFF");

});




//================ KIPAS ================


server.on("/kipas/on",[](){

manualKipas=true;

kipas=true;

digitalWrite(RELAY_KIPAS,LOW);

server.send(200,"text/plain","Kipas ON");

});



server.on("/kipas/off",[](){

manualKipas=true;

kipas=false;

digitalWrite(RELAY_KIPAS,HIGH);

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



//========== BACA SENSOR ===============


suhu = dht.readTemperature();

humidity = dht.readHumidity();



int adc = analogRead(SOIL_PIN);


// kalibrasi sensor tanah

soil = map(adc,4095,1200,0,100);

soil = constrain(soil,0,100);





//========== OTOMATISASI ===============



if(!manualPompa){


if(soil < 40){

pompa=true;

digitalWrite(RELAY_POMPA,LOW);

}

else{

pompa=false;

digitalWrite(RELAY_POMPA,HIGH);

}


}



if(!manualKipas){


if(suhu > 30){

kipas=true;

digitalWrite(RELAY_KIPAS,LOW);

}

else{

kipas=false;

digitalWrite(RELAY_KIPAS,HIGH);

}


}




//========== LCD ========================


if(millis()-lastLCD > 2000){


lastLCD=millis();


lcd.clear();


lcd.setCursor(0,0);

lcd.print("SMART FARM");


lcd.setCursor(0,1);

lcd.print("T:");

lcd.print(suhu);

lcd.print("C");


lcd.print(" H:");

lcd.print(humidity);



lcd.setCursor(0,2);

lcd.print("Soil:");

lcd.print(soil);

lcd.print("%");



lcd.setCursor(0,3);

lcd.print("P:");

lcd.print(pompa?"ON ":"OFF");


lcd.print(" F:");

lcd.print(kipas?"ON":"OFF");


}




//========== THINGSPEAK ================


if(millis()-lastThingSpeak > 15000){


lastThingSpeak=millis();



ThingSpeak.setField(1,suhu);

ThingSpeak.setField(2,humidity);

ThingSpeak.setField(3,soil);

ThingSpeak.setField(4,pompa);

ThingSpeak.setField(5,kipas);



int status = ThingSpeak.writeFields(channelID,writeAPI);



if(status==200){

Serial.println("ThingSpeak Upload OK");

}

else{

Serial.print("ThingSpeak Error : ");

Serial.println(status);

}


}



}
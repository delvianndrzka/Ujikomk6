#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ThingSpeak.h>


//================ WIFI =================

const char* ssid = "R_2.4";
const char* password = "Ganes.115";


//============== THINGSPEAK =============

unsigned long channelID = 3441844;

const char* writeAPI = "TQO4792EXV0NDWQ6";

WiFiClient client;


//============== OUTPUT LED =============

#define LED_POMPA 25
#define LED_KIPAS 27


// STATUS OUTPUT

bool pompa = false;
bool kipas = false;


// MODE CONTROL
// true = AUTO
// false = MANUAL

bool autoPompa = true;
bool autoKipas = true;



//============== WEB SERVER =============

WebServer server(80);



//============== SENSOR DATA ============

float suhu = 0;

float humidity = 0;

int soil = 0;



unsigned long lastSensor = 0;

unsigned long lastThingSpeak = 0;





//=======================================
// SETUP
//=======================================

void setup(){


Serial.begin(115200);


//================ LED ==================

pinMode(LED_POMPA, OUTPUT);

pinMode(LED_KIPAS, OUTPUT);


digitalWrite(LED_POMPA, LOW);

digitalWrite(LED_KIPAS, LOW);



//================ LITTLEFS =============

if(!LittleFS.begin(true)){

Serial.println("LittleFS ERROR");

}
else{

Serial.println("LittleFS OK");

}




//================ WIFI =================

WiFi.begin(ssid,password);


Serial.print("Connecting WiFi");


while(WiFi.status()!=WL_CONNECTED){

delay(500);

Serial.print(".");

}


Serial.println();

Serial.println("WiFi Connected");

Serial.print("IP Address : ");

Serial.println(WiFi.localIP());




//================ THINGSPEAK ===========

ThingSpeak.begin(client);





//=======================================
// WEBSITE ROUTING
//=======================================



//============== INDEX ==================

server.on("/",HTTP_GET,[](){

File file = LittleFS.open("/index.html","r");


if(!file){

server.send(404,"text/plain","index.html not found");

return;

}


server.streamFile(file,"text/html");

file.close();


});




//============== CSS ====================

server.on("/style.css",HTTP_GET,[](){

File file = LittleFS.open("/style.css","r");


if(!file){

server.send(404,"text/plain","style.css not found");

return;

}


server.streamFile(file,"text/css");

file.close();


});




//============== JAVASCRIPT =============

server.on("/script.js",HTTP_GET,[](){

File file = LittleFS.open("/script.js","r");


if(!file){

server.send(404,"text/plain","script.js not found");

return;

}


server.streamFile(file,"application/javascript");

file.close();


});





//============== DATA JSON ==============

server.on("/data",HTTP_GET,[](){


String json="{";


json += "\"suhu\":" + String(suhu) + ",";

json += "\"humidity\":" + String(humidity) + ",";

json += "\"soil\":" + String(soil) + ",";


json += "\"modePompa\":" + String(autoPompa ? "true" : "false") + ",";

json += "\"modeKipas\":" + String(autoKipas ? "true" : "false") + ",";


json += "\"pompa\":" + String(pompa ? "true" : "false") + ",";

json += "\"kipas\":" + String(kipas ? "true" : "false");


json+="}";


server.send(200,"application/json",json);


});





//=======================================
// MODE POMPA
//=======================================


server.on("/pompa/auto",HTTP_GET,[](){

autoPompa = true;

server.send(200,"text/plain","Pompa AUTO");

});



server.on("/pompa/manual",HTTP_GET,[](){

autoPompa = false;

server.send(200,"text/plain","Pompa MANUAL");

});





//=======================================
// MODE KIPAS
//=======================================


server.on("/kipas/auto",HTTP_GET,[](){

autoKipas = true;

server.send(200,"text/plain","Kipas AUTO");

});



server.on("/kipas/manual",HTTP_GET,[](){

autoKipas = false;

server.send(200,"text/plain","Kipas MANUAL");

});





//=======================================
// CONTROL MANUAL POMPA
//=======================================


server.on("/pompa/on",HTTP_GET,[](){


if(!autoPompa){


pompa = true;

digitalWrite(LED_POMPA,HIGH);


}


server.send(200,"text/plain","Pompa ON");


});





server.on("/pompa/off",HTTP_GET,[](){


if(!autoPompa){


pompa = false;

digitalWrite(LED_POMPA,LOW);


}


server.send(200,"text/plain","Pompa OFF");


});






//=======================================
// CONTROL MANUAL KIPAS
//=======================================


server.on("/kipas/on",HTTP_GET,[](){


if(!autoKipas){


kipas = true;

digitalWrite(LED_KIPAS,HIGH);


}


server.send(200,"text/plain","Kipas ON");


});





server.on("/kipas/off",HTTP_GET,[](){


if(!autoKipas){


kipas = false;

digitalWrite(LED_KIPAS,LOW);


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
// SIMULASI DATA SENSOR
//=======================================


if(millis()-lastSensor > 3000){


lastSensor = millis();


// random suhu 20 - 35 C

suhu = random(200,351) / 10.0;


// random kelembapan udara 40 - 90 %

humidity = random(400,901) / 10.0;


// random kelembapan tanah 20 - 80 %

soil = random(20,81);



Serial.println("====================");

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
// AUTO CONTROL POMPA
//=======================================


if(autoPompa){



    // tanah kering

    if(soil < 40){


        pompa = true;


        digitalWrite(LED_POMPA,HIGH);


    }


    else{


        pompa = false;


        digitalWrite(LED_POMPA,LOW);


    }


}





//=======================================
// AUTO CONTROL KIPAS
//=======================================


if(autoKipas){



    // suhu panas

    if(suhu > 30){


        kipas = true;


        digitalWrite(LED_KIPAS,HIGH);


    }


    else{


        kipas = false;


        digitalWrite(LED_KIPAS,LOW);


    }


}





//=======================================
// SERIAL MONITOR STATUS
//=======================================


Serial.print("Mode Pompa : ");

if(autoPompa)

Serial.println("AUTO");

else

Serial.println("MANUAL");



Serial.print("Pompa : ");

if(pompa)

Serial.println("ON");

else

Serial.println("OFF");




Serial.print("Mode Kipas : ");

if(autoKipas)

Serial.println("AUTO");

else

Serial.println("MANUAL");



Serial.print("Kipas : ");

if(kipas)

Serial.println("ON");

else

Serial.println("OFF");





//=======================================
// UPLOAD THINGSPEAK
// FIELD:
// 1 = Suhu
// 2 = Kelembapan Udara
// 3 = Kelembapan Tanah
//=======================================


if(millis()-lastThingSpeak > 15000){


lastThingSpeak = millis();



ThingSpeak.setField(1,suhu);


ThingSpeak.setField(2,humidity);


ThingSpeak.setField(3,soil);





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
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <HTTPClient.h>

#include <Arduino_JSON.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

//Variables de la bomba
int velocidad_Bomba = 10;
byte pin_bomba = 5;
int valor = 0;

//Variables de la Red Local
const char* ssid = "Pas";
const char* password = "123456789";

//Variables para Enviar datos
const char* serv = "http://ip:5000/historial";

//Variables para obtener lo valores del Servidor Python
int TemMax = 0;
int TemMin = 0;
int NivelHumedad = 0;
const char* NombreAsp = "";
const char* NombreP = "";

int NumSerie = 133412;

//Variables de Api OpenWhater
String contraApiOpen = "";
String city = "Ciudad Tula";
String countryCode = "MX";

unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

String jsonBuffer;

void setup() {
      Serial.begin(115200);
      
       pinMode(23, OUTPUT);
      digitalWrite(23, LOW);
        
       pinMode(pin_bomba,OUTPUT);   


      WiFi.begin(ssid, password);
      Serial.println("Conectando");
      while(WiFi.status() != WL_CONNECTED) {
         delay(800);
         Serial.print(".");
        digitalWrite(23, LOW);
        delay(1000);
        digitalWrite(23, HIGH);
      }
         Serial.println("");
         Serial.print("Connectado a la ip: ");
         Serial.println(WiFi.localIP());
       
  Serial.println("Bienvenido se ha conectado :)");
  }


int ObtenerDatos(){

  WiFiClient cliente2;

if(!cliente2.connect("ip",5000)){
       Serial.println("No se pudo conectar al servidor");
       return 0;      
    }

    cliente2.print(String("GET /values HTTP/1.1\r\n")+
                "Host: ip:5000\r\n" +
                "Connection: close\r\n\r\n");
    
    while(cliente2.connected()) {
    if (cliente2.available()) {
      String line = cliente2.readStringUntil('\r');
      //Serial.println(line);
      
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, line);
      
      NombreAsp = doc["NombreAsp"];
      NombreP = doc["NombreP"];
      TemMax = doc["TemMax"];
      TemMin = doc["TemMin"];
      NivelHumedad = doc["NivelHumedad"];
  }
}
cliente2.stop();

  delay(4000);

  return 1;
}




int EnviarHis(){
 
   WiFiClient his;

    if(!his.connect("ip",5000)){
      Serial.println("No se pudo conectar al servidor");
      return 0;
    }
     
      DynamicJsonDocument doc(1024);
      
      doc["NumSerie"] = NumSerie;
      
       String jsonString;
       serializeJson(doc, jsonString);
       
       Serial.println(jsonString);
       
       HTTPClient http;
       http.begin(his, serv);
       http.addHeader("Content-Type", "application/json");
       int httpResponseCode = http.POST(jsonString);

       if (httpResponseCode > 0) {
           String response = http.getString();
           Serial.println("Respuesta del servidor:");
           Serial.println(response);
       } else {
           Serial.print("Error en la solicitud HTTP: ");
           Serial.println(httpResponseCode);
       }

      // Liberar los recursos
       http.end();
       his.stop();

  return 1;
}




int encender_bombas(){
     
     int sensor_humedad = analogRead(39);
     int humedad = map(sensor_humedad,1865,4095,101,0);
     
     ObtenerDatos();
   //     int humedad = 40; 
     Serial.println(humedad);
                
        int NivMasUno = NivelHumedad + 1;
     Serial.println(NivMasUno);
      if(humedad >= 0 && humedad <= 5){
              Serial.println(" >> El sensor esta desconectado o fuera del suelo << ");
                 digitalWrite(pin_bomba, LOW);

                 valor = 0;
       }

      else if(humedad >= 1 && humedad < NivelHumedad){
              Serial.println(" >> El suelo esta seco << ");
                digitalWrite(pin_bomba, HIGH);
                digitalWrite(pin_bomba, velocidad_Bomba);
                EnviarHis();
                 valor = 1;

        }

        else if(humedad > NivelHumedad && humedad <= 100){
              Serial.println(" >> El suelo esta humedo << ");
                digitalWrite(pin_bomba, LOW);
                valor = 0; 
        }
        delay(1000);
    return valor;
}




void loop() {
  
   if((millis() - lastTime) > timerDelay){

      if(WiFi.status() == WL_CONNECTED){
         String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" +  contraApiOpen; 
         
         jsonBuffer = httpGETRequest(serverPath.c_str());
        
         // Serial.println(jsonBuffer);
         JSONVar myObject = JSON.parse(jsonBuffer);

           if (JSON.typeof(myObject) == "undefined"){
             Serial.println("Parsin input failed!");
             return;
           }
    
      //Serial.print("JSON object = ");
      //Serial.println(myObject);
            
      Serial.println();
      
      ObtenerDatos(); 

      Serial.print("Temperature: ");
      Serial.println(myObject["main"]["temp"]);
      
     int tempe = myObject["main"]["temp"];    

      float celcius=tempe - 273.15;
      Serial.print("Temperatura transformada:");
      Serial.print(tempe - 273.15);
      Serial.println(" °C");      
      
      Serial.println();
      Serial.print("El nombre del Aspersor: ");
      Serial.println(NombreAsp);
      Serial.print("El nombre de la planta: ");
      Serial.println(NombreP);      
      Serial.print("Temperatura máxima: ");
      Serial.println(TemMax);
      Serial.print("Temperatura Minima: ");
      Serial.println(TemMin);
      Serial.print("Nivel de humedad que requiere la planta: ");
      Serial.println(NivelHumedad);

      if(celcius >= TemMin && celcius <= TemMax ){
                     
              valor = encender_bombas();
              
              digitalWrite(23, LOW);
               
              Serial.println(valor);
              delay(2000);
            
      }else{
          digitalWrite(23, LOW);
             
      }
    }
    else {
      digitalWrite(pin_bomba, LOW);
      Serial.println("El WiFi esta desconectado");
      delay(500);
      digitalWrite(23, LOW);
        delay(800);
        digitalWrite(23, HIGH);
        delay(800);
        digitalWrite(23, LOW);
        delay(800);
        digitalWrite(23, HIGH);
    }
    lastTime = millis();
  }
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Su nombre de dominio con ruta URL o dirección IP con ruta
  http.begin(client, serverName);
  
  // Enviar solicitud HTTP POST
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    //Serial.print("HTTP Response code: ");
    //Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

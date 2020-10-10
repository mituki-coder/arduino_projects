#include<SoftwareSerial.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include<WiFiClient.h>

const char* ssid="virus";
const char* password="Abcd123456";
WiFiClient client;
unsigned long myChannelNumber = 678871; //Your channel Id
const char * myWriteAPIKey = "PN28YCSSQOUCSVEF"; 

int wificheck=1;

SoftwareSerial nodeMcu(D6,D5);
void setup() {
  // put your setup code here, to run once:
pinMode(1, OUTPUT);  
Serial.begin(9600);
nodeMcu.begin(9600);
WiFi.begin(ssid,password);
WiFi.mode(WIFI_STA);
delay(100);
//connecting to wifi
while(WiFi.status()!=WL_CONNECTED){
   delay(500);
   Serial.println(".");
   }
digitalWrite(1,HIGH);   
Serial.println("connected");

ThingSpeak.begin(client); 
}

void loop() {
  // put your main code here, to run repeatedly:
StaticJsonBuffer<2000> jsonBuffer;
JsonObject& root = jsonBuffer.parseObject(nodeMcu);
int temp1=root["temp1"];
int temp2=root["temp2"];
int ethanol=root["eth"];
int  CO2=root["CO2"];
int level=root["level"]; 



if((temp1==0)&&(temp2==0)&&(ethanol==0)&&(CO2==0)&&(level==0)){
  Serial.println("*");
  delay(100);
  }
  else{
ThingSpeak.setField(1, temp1=root["temp1"]);
ThingSpeak.setField(2, temp2=root["temp2"]);
ThingSpeak.setField(3, ethanol=root["eth"]);
ThingSpeak.setField(4, level);
ThingSpeak.setField(5, CO2);
  //updating channels
Serial.print("temperature1(Degree C):");     
Serial.print(temp1);
Serial.println();
Serial.print("temperature2(Degree C):");
Serial.print(temp2);
Serial.println();
Serial.print("Ethanol content(ppm):");
Serial.print(ethanol);
Serial.println();
Serial.print("CO2 content(ppm):");
Serial.print(CO2);
Serial.println();
Serial.print("Produce level:(inches)");
Serial.print(level);
Serial.println();
delay(1000);
//updating channels
  int x=ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  delay(5000);
  if (x==200){
    Serial.println("update successful");
    }
    else{
      Serial.println("update not successful");
    } 
    delay(100); 
  }
  }



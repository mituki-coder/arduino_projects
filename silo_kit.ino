#include<SoftwareSerial.h>
#include <Keypad.h>
#include<SPI.h>
#include<SD.h>
#include<DS3231.h>
#include<Wire.h>
#include<Adafruit_AM2315.h>
#include <ArduinoJson.h>
#include<LiquidCrystal_I2C.h>
//#include <NewPing.h>
LiquidCrystal_I2C lcd(0x27, 16, 4);



/*****************keypad*****************************/
char customKey;
long first = 0;

const long waitDelay = 6000;
unsigned long waitTime = 0;

const byte ROWS = 4;
const byte COLS = 4; 
char keypad_keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {22, 24, 26, 28}; 
byte colPins[COLS] = {30, 32, 34, 36};

Keypad customKeypad = Keypad( makeKeymap(keypad_keys), rowPins, colPins, ROWS, COLS);

/*********nodeMCU_communication*****************/
SoftwareSerial nodeMcu(5,6);
int wificheck=7;

/***********temperature************************/
Adafruit_AM2315 temp1,temp2;
int temperature_1,temperature_2;
float average_temp;
int Temp_setpoint = 0;
int Temp_lower;
int Temp_upper;
int external_temp;
int relay = 14;

/********codeflow_variables****************/
int control_modePin=18;
int mainMode_control=19;
int load_switch = 15;

/******************************************/

/********proximity sensor*****************/
int trig_pin=16;
int echo_pin=17;
float level_cm,level_inches;
long int duration;
/****************************************/
int analogPin=A0;
int analogPin1=A1;
int ethanol_content,CO2_content;

/**************ethanol and CO2********************/



/***********SD CARD and RTC module**********************/
int chip_selectPin=4;
//String header="ID:  DATE(year-month-date):  TIME(hours-minutes):  PRODUCE_LEVEL(inches):  TEMP_1(degree_celsious):  TEMP_AVG(degree_celsious):  ETH_LEVEL(ppm):     CO2(ppm):";
String labels = "ID\t   DATE\t\t    TIME\tPRODUCE_LEVEL\t\t     TEMP_OUT\t\t     TEMP_IN_AVG      ETHANOL\t\tCO2";
String units = "\t(YY/MM/DD)\t(Hour:Min:Sec)\t    (CM)\t         (degrees celcius)        (degrees celcius)    (PPM)\t       (PPM)";
File silo_card;
DS3231 rtc;
int id=0;
bool Century=false;
bool h12,PM;

StaticJsonBuffer<2000> jsonBuffer;
JsonObject& root =jsonBuffer.createObject();
void setup() {
  // put your setup code here, to run once:
pinMode(7,INPUT);
Serial.begin(9600);
lcd.begin();
lcd.clear();
lcd.print("   SILO KIT.");
nodeMcu.begin(9600);
StaticJsonBuffer<1000> jsonBuffer;
JsonObject& root =jsonBuffer.createObject();


temp1.begin();
temp2.begin();
pinMode(control_modePin,INPUT);
pinMode(mainMode_control,INPUT);
pinMode(load_switch, INPUT);
Wire.begin();
/********SDCARD AND RTC**************/
pinMode(15,OUTPUT);
pinMode(A0,INPUT);
pinMode(A1,INPUT);
pinMode(relay, OUTPUT);
SD.begin();
delay(50);

if(SD.begin(chip_selectPin)){
  Serial.println("SD card ready");
  lcd.setCursor(0,1);
  lcd.print("SDcard present");
  delay(10);
 }
else{
  Serial.println("SD card not ready");
  lcd.setCursor(0,1);
  lcd.print("SDcard error.");
  lcd.setCursor(-4,2);
  lcd.print("Insert SDcard.");
 }
Serial.println("creating a new file........");
silo_card = SD.open("silofile.txt", FILE_WRITE); //file name is SILO_FILE
if(silo_card){
  Serial.println("FILE_READY FOR WRITING");
  lcd.setCursor(-4,2);
  lcd.print("Preparing file...");
  delay(10);
  lcd.setCursor(-4,3);
  lcd.print("File ready.");
  silo_card.println(labels);
  silo_card.println(units);  
//  Serial.println(header);
  Serial.println("File closed");
  silo_card.close();
  }
  else{
    Serial.println("FILE NOT READY");
    lcd.setCursor(-4,3);
    lcd.print("File error.");
  }
  rtc.setClockMode(false); //24hr clockmode
/******************************************/

/*************proximity sensor**************************/
pinMode(trig_pin,OUTPUT);
pinMode(echo_pin,INPUT);
/********************************************************/
lcd.clear();
/*lcd.print("WiFi check......");
if(digitalRead(7)==1){
lcd.setCursor(0,1);
lcd.print("WiFi connected");
}
else{
  lcd.setCursor(0,1);
  lcd.print("Not connected");
  }*/
waitTime = millis() + waitDelay;
}



void sdcard_logging(){
  silo_card=SD.open("silofile.txt",FILE_WRITE);  //reopen file for writing data
  if(silo_card){
    Serial.println("Writing to file");
//    delay(2000);
    }
  else{
      Serial.println("file open error");
//      delay(2000);
    }
    silo_card.print(id);
    silo_card.print("\t ");
    silo_card.print("20");
    silo_card.print(rtc.getYear(), DEC);
    silo_card.print('-');
    silo_card.print(rtc.getMonth(Century), DEC);
    silo_card.print('-');
    silo_card.print(rtc.getDate(), DEC);
    silo_card.print("\t   ");
    silo_card.print(rtc.getHour(h12, PM), DEC);
    silo_card.print(':');
    silo_card.print(rtc.getMinute(), DEC);
    silo_card.print(':');
    silo_card.print(rtc.getSecond(), DEC);
    silo_card.print("\t   "); 
    silo_card.print(level_inches, 1);
    silo_card.print("    \t\t\t");
    silo_card.print(external_temp, DEC);
    silo_card.print("    \t\t\t");
    silo_card.print(average_temp, 1);
    silo_card.print("  \t\t");
    silo_card.print(ethanol_content);
    silo_card.print("\t\t");
    silo_card.print(CO2_content);
    silo_card.println();
    silo_card.close();
//    delay(3000);
}

void level_monitoring(){
  digitalWrite(trig_pin,LOW);
  delay(2);
  digitalWrite(trig_pin,HIGH);
  delay(2);
  digitalWrite(trig_pin,LOW);
  duration=pulseIn(echo_pin,HIGH);
  level_cm=(duration/2)/29.1;
  level_inches=(duration/2)/74;
  Serial.print("inches..");
  Serial.println(level_inches);
  Serial.print("cm.....");
  Serial.println(level_cm);
//  delay(1000);
}


void ethanol_monitoring(){
  ethanol_content=analogRead(A0);
  delay(10);
  }
  
void CO2_monitoring(){
  CO2_content=analogRead(A1);
  delay(10);
  }
  
void internal_temperature(){
  temperature_1=temp1.readTemperature();
  temperature_2=temp2.readTemperature();
  average_temp=((temperature_1+temperature_2)/2);
  } 

void external_temperature(){
 /* external_temp=(analogRead(A3)/9.31);*/
 external_temp=rtc.getTemperature();
  delay(10);
  }

void nodeMCU(){
   root["temp1"]=average_temp;
   root["temp2"]=external_temp;
   root["eth"]=ethanol_content;
   root["CO2"]=CO2_content;
   root["level"]=level_inches;
   root.printTo(nodeMcu);
  }

  void initialization1(){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("   SILO KIT.");
    lcd.setCursor(0,1);
    lcd.print("MONITORING MODE.");
    lcd.setCursor(-4,2);
    lcd.print("TIME: ");
    lcd.setCursor(2,2);
    lcd.print(rtc.getHour(h12, PM), DEC);
    lcd.print(':');
    lcd.print(rtc.getMinute(), DEC);
    lcd.print(':');
    lcd.print(rtc.getSecond(), DEC);
    lcd.setCursor(-4,3);
    lcd.print("PRESS:#>HOME");
    lcd.noCursor(); 
    lcd.noBlink();
    }

  void settings_menu()
  {
    lcd.setCursor(0,0);
    lcd.print("TEMP SETTINGS");
    lcd.setCursor(0,0);
    lcd.print("Enter the value");
    lcd.setCursor(0,1);
    lcd.print("and press A");
    lcd.setCursor(-4,3);
    lcd.print("INPUT: ");
    lcd.setCursor(3,3);
    lcd.cursor();
    lcd.blink();
  }

  void date_time()
  {
    lcd.setCursor(2,0);
    lcd.print("DATE AND TIME");
    lcd.setCursor(0,1);
    lcd.print("TIME: ");
    lcd.setCursor(6,1);
    lcd.print(rtc.getHour(h12, PM), DEC);
    lcd.print(':');
    lcd.print(rtc.getMinute(), DEC);
    lcd.print(':');
    lcd.print(rtc.getSecond(), DEC);
    lcd.setCursor(-4,2);
    lcd.print("DATE: ");
    lcd.setCursor(2,2);
    lcd.print("20");
    lcd.print(rtc.getYear(), DEC);
    lcd.print('-');
    lcd.print(rtc.getMonth(Century), DEC);
    lcd.print('-');
    lcd.print(rtc.getDate(), DEC);
  }
  
  void initialization2()
  {
    lcd.clear();
    lcd.setCursor(4,0);
    lcd.print("SILO KIT");
    lcd.setCursor(0,1);
    lcd.print("CONTROL>AUTO");
    lcd.setCursor(-4,2);
    lcd.print("PRESS:#>HOME");
    lcd.setCursor(2,3);
    lcd.print("*>RESET");
    lcd.noCursor(); 
    lcd.noBlink();
    }

  void initialization3()
  {
    lcd.clear();
    lcd.setCursor(4,0);
    lcd.print("SILO KIT");
    lcd.setCursor(0,1);
    lcd.print("CONTROL>MANUAL");
    lcd.setCursor(-4,2);
    lcd.print("PRESS:#>HOME");
    lcd.setCursor(2,3);
    lcd.print("*>STATUS");
    lcd.noCursor(); 
    lcd.noBlink();
    }
    
 void monitoring()
 {
 if(waitTime<millis())
  {
    initialization1();
    waitTime = millis() + waitDelay;
   }
}

void control1()
{
 if(waitTime<millis())
  {
    initialization2();
    waitTime = millis() + waitDelay;
   }
}

void control2()
{
 if(waitTime<millis())
  {
    initialization3();
    waitTime = millis() + waitDelay;
   }  
}

void monitoring_display()
{ 
  lcd.setCursor(0,0);
  lcd.print("T1: ");
  lcd.setCursor(4,0);
  lcd.print(average_temp, 1);
  
  lcd.setCursor(9,0);
  lcd.print("T2: ");
  lcd.setCursor(13,0);
  lcd.print(external_temp);

  lcd.setCursor(0,1);
  lcd.print("Ethanol: ");
  lcd.setCursor(9,1);
  lcd.print(ethanol_content);

  lcd.setCursor(-4,2);
  lcd.print("Level: ");
  lcd.setCursor(5,2);
  lcd.print(level_inches, 1);

  lcd.setCursor(-4,3);
  lcd.print("CO2: ");
  lcd.setCursor(5,3);
  lcd.print(CO2_content);

  lcd.noCursor();
  lcd.noBlink();
}

void status_check()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("LOAD STATUS");
  
  if(digitalRead(load_switch) == 1)
  {
    lcd.setCursor(0,1);
    lcd.print("FAN:   ON");
  }
  else
  {
    lcd.setCursor(0,1);
    lcd.print("FAN:   OFF");
  }
}
  
void loop() {
level_monitoring();
ethanol_monitoring();
CO2_monitoring();
internal_temperature();
external_temperature();
nodeMCU();
sdcard_logging();
id++;

customKey = customKeypad.getKey();
if (digitalRead(mainMode_control)== 0)
{
  //running in the monitoring mode only
  digitalWrite(relay,LOW); 
  //delay(1000);
  while(!customKey)
   {
   monitoring();
   break;
   }
   switch(customKey)
    {
      case '#':
      lcd.clear();
      monitoring_display();
      waitTime = millis() + waitDelay;
      break;

      case '*':
      lcd.clear();
      waitTime = millis() + waitDelay;
      break;

      case 'B':
      lcd.clear();
      date_time();
      waitTime = millis() + waitDelay;
      break;
    }
}


else if(digitalRead(mainMode_control)==1)
{
//........................Control-Mode Auto Mode......................  
  if (digitalRead(control_modePin)==1)
  {
  //running in control and monitoring
   while(!customKey)
   {
    control1();
    break;
   }

 switch(customKey)
  {
    case '0' ... '9':
    first = first*10 +(customKey - '0');
    lcd.setCursor(3,3);
    lcd.print(first);
    waitTime = millis() + waitDelay;
    break;

    case 'A':
    first = (Temp_setpoint !=0? Temp_setpoint:first);
    Temp_setpoint = first;
    Temp_lower = Temp_setpoint - 2;
    Temp_upper = Temp_setpoint + 2;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Temp_setpnt ");
    lcd.setCursor(12,0);
    lcd.print(Temp_setpoint);
    lcd.setCursor(0,1);
    lcd.print("Tolerance: +/-2");
    lcd.noCursor();
    lcd.noBlink();
    delay(3000);
    initialization2();
    waitTime = millis() + waitDelay;
    first = 0;
    break;

    case '*':
    Temp_setpoint = 0;
    lcd.clear();
    lcd.setCursor(3,0);
    lcd.print("TEMPERATURE");
    lcd.setCursor(1,1);
    lcd.print("RESET TO ZERO");
    lcd.setCursor(1,2);
    lcd.print("PLEASE");
    lcd.setCursor(-2,3);
    lcd.print("INPUT TO SET");
    delay(2000);
    lcd.clear();
    lcd.noCursor();
    lcd.noBlink();
    waitTime = millis() + waitDelay;
    settings_menu();
    break;

    case '#':
    lcd.clear();
    monitoring_display();
    waitTime = millis() + waitDelay;
    break;

    case 'B':
    lcd.clear();
    date_time();
    waitTime = millis() + waitDelay;
    break;
  }

  if(average_temp>Temp_upper)
  {
   digitalWrite(relay, HIGH);
   delay(50);
  }
  else
  {
   digitalWrite(relay, LOW);
   delay(50);
  }
  }
  
else 
 {
  while(!customKey)
   {
    control2();
    break;
   }

 switch(customKey)
 {
    case '*':
    lcd.clear();
    status_check();
    waitTime = millis() + waitDelay;
    break; 

    case '#':
    lcd.clear();
    monitoring_display();
    waitTime = millis() + waitDelay;
    break;

    case 'B':
    lcd.clear();
    date_time();
    waitTime = millis() + waitDelay;
    break;
 }
 
  if(digitalRead(load_switch) == 1)
  {
    digitalWrite(relay, HIGH);
    delay(50);
  }
  else
  {
    digitalWrite(relay, LOW);
    delay(50);
  }
 }
}
/****************notification mode***********************/

}



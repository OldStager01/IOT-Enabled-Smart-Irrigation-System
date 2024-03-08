#define BLYNK_TEMPLATE_ID "BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "BLYNK_TEMPLATE_NAME "
#define BLYNK_AUTH_TOKEN "BLYNK-AUTH-TOKEN"
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
//#include <LiquidCrystal_I2C.h>
//LiquidCrystal_I2C lcd(32,6,2);
char* ssid = "WIFI-NAME";//Wi-fi Name
char* pass = "Enter-Password";//Wi-fi Password

/*Blynk Virtual Pins:
  Mode Selection: V6
  Soil Moisture 1: V1
  Soil Moisture 2: V2
  Temperature: V3
  Hours (Section 1): V8
  Duration (Section 2): V7
  Hours (Section 2): V9
  Duration (Section 2): V10
  Valve 1 (Manual): V4
  Valve 2 (Manual): V5
*/

//Pins Allotment
int motor1=19,motor2=21;
int pump=18;
int soil1=34;
int soil2=35;

//Default
int motor1Stat=0, motor2Stat=0;
int pumpStat=0;
int baudRate=115200;
int mode;
int minMoistLevel1=50;
int minMoistLevel2=50; 
int min1=0,dur1=0,oldMin1=0,oldDur1=0; //For Time Interval of Motor 1
int min2=0,dur2=0,oldMin2=0,oldDur2=0; //For Time Interval of Motor 2
unsigned long timeWait1=0, timeIrrigate1=0, totalWait1=0, totalIrrigate1=0;
unsigned long timeWait2=0, timeIrrigate2=0, totalWait2=0, totalIrrigate2=0;
int ctr=0, updateDelay=2000,updateTime;

//Data Variables
int moistLevel1,moistLevel2;
int oldMode,isModeChanged;
int toggleManual1=0, toggleManual2=0;
int temp;
void setup()
{
  Serial.begin(baudRate);
  
  //Pin Setup
  pinMode(motor1,OUTPUT);
  pinMode(motor2,OUTPUT);
  pinMode(pump,OUTPUT);
  pinMode(soil1,INPUT);
  pinMode(soil2,INPUT);
  
  delay(1000);
  //Wifi Connecting
  WiFi.mode(WIFI_STA);//Station Mode
  WiFi.begin(ssid, pass);
  Serial.println("\nConnecting");

  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(100);
  }

  Serial.println("\nSuccessfully Connected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

//Read Data from Blynk Server

BLYNK_CONNECTED() {
   Blynk.syncAll();
}
BLYNK_WRITE(V6)//Mode Select
{
  mode=param.asInt();
}
BLYNK_WRITE(V8)
{
  min1=param.asInt();
}
BLYNK_WRITE(V7)
{
  dur1=param.asInt();
}
BLYNK_WRITE(V9)
{
  min2=param.asInt();
}
BLYNK_WRITE(V10)
{
  dur2=param.asInt();
}
BLYNK_WRITE(V4)
{
  toggleManual1=param.asInt();
}
BLYNK_WRITE(V5)
{
  toggleManual2=param.asInt();
}

void loop()
{
  Blynk.run();
  isModeChanged=0;
  if(oldMode!=mode)
  {
    isModeChanged=1;
  }
  oldMode=mode;
  if(ctr==0 || (millis()-updateTime>updateDelay)){
  getdata();
  updateTime=millis()+updateDelay;
  }
  //display();//Print on Serial Monitor
  switch(mode)
  {
    case 1:{
      Automatic();
    }break;
    case 2: {
      
        if(oldMin1!=min1 ||oldDur1!=dur1 ||ctr==0) 
        {
          setTime1();
          oldMin1=min1;
          oldDur1=dur1;
        }
        if(oldMin2!=min2 ||oldDur2!=dur2 ||ctr==0) 
        {
          setTime2();
          oldMin2=min2;
          oldDur2=dur2;
        }
      
      TimeInterval1();
      TimeInterval2();
    }break;
    case 3: {
      Manual();
    }break;
  }
  ctr=1;
  delay(300);
}


void getdata()
{
  moistLevel1=((1700-analogRead(soil1))*100/1700);
  Blynk.virtualWrite(V1,moistLevel1);
  moistLevel2=((2700-analogRead(soil2))*100/2700);
  Blynk.virtualWrite(V2,moistLevel2);
}

void pumpToggle(){
  if(motor1Stat==1 || motor2Stat==1)
  {
    digitalWrite(pump,LOW);
    Serial.println("Pump Started!");
  }
  else{
    digitalWrite(pump,HIGH);
    Serial.println("Pump Closed!");
  }
}

void motor1Toggle(int enable)
{
  if(enable==1 && motor1Stat==0)
  {
    digitalWrite(motor1, LOW);
    motor1Stat=1;
    Serial.println("Valve 1 Opened!");
    pumpToggle();
  }
  else if(enable==0 && motor1Stat==1)
  {
    digitalWrite(motor1, HIGH);
    motor1Stat=0;
    Serial.println("Valve 1 Closed!");
    pumpToggle();
  }
  
}
void motor2Toggle(int enable)
{
  if(enable==1 && motor2Stat==0)
  {
    digitalWrite(motor2, LOW);
    motor2Stat=1;
    Serial.println("Valve 2 Opened!");
    pumpToggle();

  }
  else if(enable==0 && motor2Stat==1)
  {
    digitalWrite(motor2, HIGH);
    motor2Stat=0;
    Serial.println("Valve 2 Closed!");
    pumpToggle();
  }
}

void Automatic()
{
  delay(200);
  //PART 1
  if(moistLevel1<=minMoistLevel1)
  {
    motor1Toggle(1);
  }
  else if(moistLevel1>minMoistLevel1)
  {
    motor1Toggle(0);
  }

  //PART 2
  if(moistLevel2<=minMoistLevel2)
  {
     motor2Toggle(1);
  }
  else if(moistLevel2>minMoistLevel2)
  {
     motor2Toggle(0);
  }
}

void setTime1()
{
  timeWait1=timeToMilli(min1);
  timeIrrigate1=timeToMilli(dur1);
  totalWait1=millis()+timeWait1;
  totalIrrigate1=totalWait1+timeIrrigate1;
}
void setTime2()
{
  timeWait2=timeToMilli(min2);
  timeIrrigate2=timeToMilli(dur2);
  totalWait2=millis()+timeWait2;
  totalIrrigate2=totalWait2+timeIrrigate2;
}

long timeToMilli(int min)
{
  return (min*1000);//Setting in Seconds for testing. Default: 60000
}
void TimeInterval1()
{
    if(millis()>=totalWait1)
    {
      if(millis()<=totalIrrigate1)
      {
        motor1Toggle(1);
      }else{
        motor1Toggle(0);
      }
    }
}
void TimeInterval2()
{
    if(millis()>=totalWait2)
    {
      if(millis()<=totalIrrigate2)
      {
        motor2Toggle(1);
      }else{
        motor2Toggle(0);
      }
    }
}
void Manual(){
    motor1Toggle(toggleManual1);
    motor2Toggle(toggleManual2);
}
void display()
{
  Serial.println("Moisture Levels:");
  Serial.println(moistLevel1);
  Serial.println(moistLevel2);
}

void reset()
{
  motor1Toggle(0);
  motor2Toggle(0);
}
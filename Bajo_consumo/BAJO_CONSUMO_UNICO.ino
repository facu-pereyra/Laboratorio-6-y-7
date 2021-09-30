/*
**********************************************************************************
* El Arduino EMPIEZA DORMIDO. SE PUEDE PONER UN SOLO PERÍODO DE GRABACIÓN AL DÍA *
**********************************************************************************

* Alarma_1 da el inicio de grabación cuya frecuencia de muestreo la da la variable "FreqMuestreo"

* Alarma_2 corta la grabación y duerme hasta la alarma 1

* Hay salto de fichero donde su duración está dada por la variable "Duracion"

* El nombre del archivo está compuesto por el día más la hora, así por ejemplo, si se graba un archivo el 24 a las 16:07:35, 
 entonces el archivo se llamará 24160735.wav
 
        ***********
        **WARNING**: EVITAR SACAR LA TARJETA MICRO SD DURANTE LA GRABACIÓN (SE PUEDE ROMPER).
        ***********

*/

#include <avr/sleep.h>

#include <SD.h>
#include <SPI.h>
#include <TMRpcm.h>
#include <DS3232RTC.h>   // https://github.com/JChristensen/DS3232RTC
#include "RTClib.h"    
#define interruptPin 2 //Pin we are going to use to wake up the Arduino
#define SD_ChipSelectPin 10  

TMRpcm audio;   
#define MIC A0

//==============================================================================
//Configurable

uint8_t Duracion = 15; //Duracion de los ficheros, esta en minutos
uint16_t FreqMuestreo = 22000; //Frecuencia de muestreo

uint8_t HoraInicio = 12; //Empieza a grabar a esta hora.
uint8_t MinInicio = 1;

uint8_t HoraDormir = 15; // Corta la grabación y se duerme hasta la HoraInicio.
uint8_t MinDormir = 1;
//==============================================================================
char filename[] = "00000000.wav"; 

uint8_t LED_Work = 4;
uint8_t LED_Error = 5;
uint8_t Transistor = 9;
uint8_t RST_PIN = 6; //VER si lo puso

volatile boolean Siguiente = false; //para ir al próximo fichero
unsigned long tiempo_fichero; //Variable del millis()
unsigned long SaltoFichero = Duracion * 60000; //60 seg * 1000 (mili) = 60000 milisegundos
uint8_t HORA;
uint8_t HORA_2;
uint8_t MIN;
uint8_t MIN_2;

//==============================================================================
  RTC_DS3231 rtc;
  
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();
  *date = FAT_DATE(now.year(), now.month(),  now.day());
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}
//==============================================================================
void getFileName(){
    time_t t; 
    t=RTC.get();
    sprintf(filename, "%02d%02d%02d%02d.wav", day(t), hour(t), minute(t), second(t));
}
//==============================================================================
void LedError() {
  digitalWrite(LED_Error, LOW);  
  digitalWrite(LED_Work,LOW);
  for (int j = 1; j < 6; j++) { 
    digitalWrite(LED_Error, HIGH);
    delay(1000);
    digitalWrite(LED_Error, LOW);
    delay(1000);      
  }
  //pinMode(RST_PIN,  OUTPUT);
  //digitalWrite(RST_PIN, LOW);
}
//==============================================================================
void wakeUp(){
  sleep_disable();
  detachInterrupt(0); 
}
//==============================================================================
void setup() {
  rtc.begin();
  DateTime now = rtc.now();
  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(LED_Work,OUTPUT);
  pinMode(LED_Error,OUTPUT);
  pinMode(MIC, INPUT);
  pinMode(Transistor, OUTPUT); 
  digitalWrite(Transistor,HIGH);
    
  audio.CSPin = SD_ChipSelectPin;  
  if (!SD.begin(SD_ChipSelectPin)) LedError();
//==========================
  audio.startRecording("Setup.wav",FreqMuestreo,MIC);
  delay(200);
  audio.stopRecording("Setup.wav"); 
  Siguiente=false;
//==========================  
    
  if (rtc.lostPower()) {
	rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	}
        
    
  RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, false);
  RTC.squareWave(SQWAVE_NONE);
    
  time_t t; 
  t=RTC.get();
  RTC.setAlarm(ALM1_MATCH_HOURS , 0, MinInicio, HoraInicio, 1);  //Se setea la Hora de inicio 
  RTC.setAlarm(ALM2_MATCH_HOURS, 0, MinDormir, HoraDormir, 1);  //Se pone la hora del primer corte de grabacion
  RTC.alarm(ALARM_1); // clear the alarm flag
  RTC.alarm(ALARM_2);                
  RTC.squareWave(SQWAVE_NONE); 
  RTC.alarmInterrupt(ALARM_1, true); // enable interrupt output for Alarm 1            
  RTC.alarmInterrupt(ALARM_2, true);

  digitalWrite(Transistor,LOW);     
  sleep_enable();
  attachInterrupt(0, wakeUp, LOW);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_cpu(); //Modo de bajo consumo
}

void loop() {
  time_t t = RTC.get();
  DateTime now = rtc.now();
// check to see if the INT/SQW pin is low, i.e. an alarm has occurred
  if (!digitalRead(interruptPin)){
    time_t t = RTC.get();
    if (RTC.alarm(ALARM_1)){             
      digitalWrite(Transistor,HIGH);
      delay(500);
      if (!SD.begin(SD_ChipSelectPin)) LedError();
	    getFileName();
	    SdFile::dateTimeCallback(dateTime); 
      audio.startRecording(filename,FreqMuestreo,MIC);
      digitalWrite(LED_Work,HIGH);
      Siguiente=false;
      t=RTC.get();
      tiempo_fichero = millis();
      RTC.setAlarm(ALM1_MATCH_HOURS , 0, MinInicio, HoraInicio, 1);  
      RTC.alarm(ALARM_1);
      RTC.alarmInterrupt(ALARM_1, true);             
    }        
    if (RTC.alarm(ALARM_2)){
      audio.stopRecording(filename); 
      Siguiente=false;
      Dormir_Alarma2();          
    }
  }
  if((millis() - tiempo_fichero > SaltoFichero)){  
    digitalWrite(LED_Work, LOW);
    audio.stopRecording(filename);
    Siguiente=true;
  }
  if(Siguiente==1){
    if (!SD.begin(SD_ChipSelectPin)) LedError();
    getFileName();
    SdFile::dateTimeCallback(dateTime); 
    audio.startRecording(filename,FreqMuestreo,MIC);
    digitalWrite(LED_Work,HIGH);
    Siguiente=false;
    t=RTC.get();
    tiempo_fichero = millis();
  }
}

//==============================================================================
void Dormir_Alarma2(){  
  sleep_enable();
  attachInterrupt(0, wakeUp, LOW);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  digitalWrite(LED_Work,LOW);
  time_t t;
  t=RTC.get();
  RTC.setAlarm(ALM2_MATCH_HOURS , 0, MinDormir, HoraDormir, 1);           
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_2, true);
  digitalWrite(Transistor,LOW); 
  delay(500); //wait a second to allow the led to be turned off before going to sleep
  sleep_cpu();
}
//==============================================================================

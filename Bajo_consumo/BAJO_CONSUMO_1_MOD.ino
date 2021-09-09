/*
*********************************
* El Arduino EMPIEZA DESPIERTO. *
*********************************
* Alarma 1 da el inicio de grabación y la alarma 2 da el corte
* Hay salto de fichero


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
uint8_t FreqMuestreo = 22000; //Frecuencia de muestreo

uint8_t HoraInicio = 8; //Empieza a grabar a esta hora y termina 2 horas despues. A las 7 horas se repite el proceso
uint8_t MinInicio = 1;

uint8_t HoraDormir = 21; //A partir de esta hora hasta la HoraInicio el Arduino está dormido
uint8_t MinDormir = 0;
//==============================================================================
char filename[] = "00000000.wav"; 
char timestamp[32]; //VER


uint8_t RST_PIN = 4;
uint8_t LED_Error = 6;
uint8_t LED_Work = 7;
//uint8_t LED_Control = 8;

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
  pinMode(RST_PIN,  OUTPUT);
  digitalWrite(RST_PIN, LOW);
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

    //Serial.begin(9600);
    pinMode(interruptPin, INPUT_PULLUP);
    pinMode(LED_Work,OUTPUT);
    pinMode(LED_Error,OUTPUT);
    //pinMode(LED_Control,OUTPUT);
    audio.CSPin = SD_ChipSelectPin; 
    pinMode(MIC, INPUT);  
    
  	if (!SD.begin(SD_ChipSelectPin)) LedError();

    /*
    //Poner en hora el reloj
    tmElements_t tm;
    tm.Hour = 15;               // set the RTC to an arbitrary time
    tm.Minute = 50;
    tm.Second = 20;
    tm.Day = 6;
    tm.Month = 9;
    tm.Year = 2021 - 1970;      // tmElements_t.Year is the offset from 1970
    RTC.write(tm);              // set the RTC from the tm structure
    */
    
    if (rtc.lostPower()) {
		rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
		// This line sets the RTC with an explicit date & time, for example to set
		// January 21, 2014 at 3am you would call:
		//rtc.adjust(DateTime(2021, 9, 6, 15, 34, 30));// Esta linea es para ajustar de forma manual el día y hora
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
    RTC.setAlarm(ALM1_MATCH_HOURS , 0, 30, 17, 0);  //Se setea la Hora de inicio  
    RTC.setAlarm(ALM2_MATCH_HOURS, 0,  30, 19, 0);  //Se pone la hora del primer corte de grabacion
    RTC.alarm(ALARM_1); // clear the alarm flag
    RTC.alarm(ALARM_2);                
    RTC.squareWave(SQWAVE_NONE); 
    RTC.alarmInterrupt(ALARM_1, true); // enable interrupt output for Alarm 1            
    RTC.alarmInterrupt(ALARM_2, true);

    //Serial.print("Hora de la libreria nueva: ");Serial.print(hour(t));Serial.print(":");Serial.println(minute(t));
    //Serial.print("Hora del RTClib: ");Serial.print(now.hour());Serial.print(":");Serial.println(now.minute());
    //delay(1000);
    
    //digitalWrite(LED_Control,HIGH);
    sleep_enable();
    attachInterrupt(0, wakeUp, LOW);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_cpu();
}

void loop() {
  time_t t = RTC.get();
  DateTime now = rtc.now();
// check to see if the INT/SQW pin is low, i.e. an alarm has occurred
  if (!digitalRead(interruptPin)){
    time_t t = RTC.get();
    if (RTC.alarm(ALARM_1)){             
      delay(500);
      if (!SD.begin(SD_ChipSelectPin)) LedError();
			getFileName();
			SdFile::dateTimeCallback(dateTime); 
      audio.startRecording(filename,FreqMuestreo,MIC);
      digitalWrite(LED_Work,HIGH);
      //digitalWrite(LED_Control,LOW);
      Siguiente=false;
      t=RTC.get();
      tiempo_fichero = millis();
      HORA=hour(t)+7;
      MIN=0;
      //Serial.print("HORA antes del primer if: "); Serial.println(HORA);
      if(HORA >= 24){
        HORA = HORA - 24;
      }
      //Serial.print("HORA despues del primer if: "); Serial.println(HORA);
      if (HORA >= HoraDormir || HORA < HoraInicio){
        HORA=HoraInicio;
        MIN=MinInicio;
      }
      //Serial.print("HORA de la alarma: "); Serial.println(HORA);
      RTC.setAlarm(ALM1_MATCH_HOURS , 0, MIN, HORA, 0);  
      RTC.alarm(ALARM_1);             
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
  //digitalWrite(LED_Control,HIGH);
  digitalWrite(LED_Work,LOW);
  time_t t;
  t=RTC.get();
  HORA_2=hour(t)+7;
  MIN_2=0;
  if(HORA_2 >= 24){
    HORA_2 = HORA_2 - 24;
  }
  if (HORA_2 >= HoraDormir || HORA_2 < HoraInicio){
    HORA_2=(HoraInicio + 2);
    MIN_2=MinInicio;
  }
  RTC.setAlarm(ALM2_MATCH_HOURS , 0, MIN_2, HORA_2, 0);           
  RTC.alarm(ALARM_2); 
  delay(500); //wait a second to allow the led to be turned off before going to sleep
  sleep_cpu();
}
//==============================================================================

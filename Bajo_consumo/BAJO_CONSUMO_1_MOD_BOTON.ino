/*
*********************************
* El Arduino EMPIEZA DORMIDO. *
*********************************

* Alarma 1 da el inicio de grabación cuya frecuencia de muestreo la da la variable "FreqMuestreo"

* alarma 2 corta la grabación y duerme hasta la alarma 1

* Hay 2 botones. Un botón despierta el arduino e inicia la grabación. 
  El BotonBreak corta la grabación y pone a dormir al arduino
  
* Hay salto de fichero donde su duración está dada por la variable "Duracion"

*El nombre del archivo está compuesto por el día más la hora, así por ejemplo, si se graba un archivo el 24 a las 16:07:35, 
 entonces el archivo se llamará 24160735.wav

* Cuando se toca el botón para grabar y no tiene la tarjeta SD salta error (titila 6 veces el led ROJO) y luego se reinicia 
 el Arduino.  
 
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
#define interruptPin 2 
#define SD_ChipSelectPin 10  

TMRpcm audio;   
#define MIC A0

//==============================================================================
//Configurable

uint8_t Duracion = 15; //Duracion de los ficheros, esta en minutos
uint16_t FreqMuestreo = 22000; //Frecuencia de muestreo

uint8_t HORA = 19; // Hora del primer inicio de la Alarma_1 (el primer inicio de la Alarma_2 está a HORA+3 ) 
uint8_t MIN = 42; // Minuto del primer inicio la Alarma_1 y Alarma_2

uint8_t HoraInicio = 11; //Empieza a grabar al siguiente día a esta hora y termina 3 horas despues. A las 10 horas se repite la alarma
uint8_t MinInicio = 1;

uint8_t HoraDormir = 17; //A partir de esta hora hasta la HoraInicio el Arduino está dormido
//==============================================================================
char filename[] = "00000000.wav"; 

uint8_t LED_Work = 4;
uint8_t LED_Error = 5;
uint8_t RST_PIN = 6; 
uint8_t BotonBreak = 7;
//uint8_t Transistor = 9;

volatile boolean Siguiente = false; //para ir al próximo fichero
volatile boolean Controlador_boton = false;
unsigned long tiempo_fichero; //Variable del millis()
unsigned long SaltoFichero = Duracion * 60000; //60 seg * 1000 (mili) = 60000 milisegundos
uint8_t HORA_2;
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
  pinMode(BotonBreak, INPUT_PULLUP);
  
  //pinMode(Transistor, OUTPUT); 
  //digitalWrite(Transistor,HIGH);
    
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
  RTC.setAlarm(ALM1_MATCH_HOURS , 0, MIN, HORA, 0);  //Se setea la Hora del primer inicio  
  RTC.setAlarm(ALM2_MATCH_HOURS, 0, MIN, HORA+3, 0);  //Se pone la hora del primer corte de grabacion
  RTC.alarm(ALARM_1); // clear the alarm flag
  RTC.alarm(ALARM_2);                
  RTC.squareWave(SQWAVE_NONE); 
  RTC.alarmInterrupt(ALARM_1, true); // enable interrupt output for Alarm 1            
  RTC.alarmInterrupt(ALARM_2, true);

  //digitalWrite(Transistor,LOW);     
  sleep_enable();
  attachInterrupt(0, wakeUp, LOW);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_cpu(); //Modo de bajo consumo
}

void loop() {
  time_t t = RTC.get();
  DateTime now = rtc.now();
  if (!digitalRead(interruptPin)){
    time_t t = RTC.get();
    if (RTC.alarm(ALARM_1)){             
      //digitalWrite(Transistor,HIGH);
      delay(500);
      if(Controlador_boton==0){
        if (!SD.begin(SD_ChipSelectPin)) LedError();
        getFileName();
        SdFile::dateTimeCallback(dateTime); 
        audio.startRecording(filename,FreqMuestreo,MIC);
        digitalWrite(LED_Work,HIGH);
      }
      Siguiente=false;
      Controlador_boton=true;
      t=RTC.get();
      tiempo_fichero = millis();
      HORA=hour(t)+10;
      MIN=1;
      if(HORA >= 24){
        HORA = HORA - 24;
      }
      if (HORA >= HoraDormir || HORA < HoraInicio){
        HORA=HoraInicio;
        MIN=MinInicio;
      }
      RTC.setAlarm(ALM1_MATCH_HOURS , 0, MIN, HORA, 0);  
      RTC.alarm(ALARM_1);             
    }        
    else if (RTC.alarm(ALARM_2)){
      audio.stopRecording(filename); 
      Siguiente=false;
      Dormir_Alarma2();          
    }
    else{ //Este else es para el botón
      //digitalWrite(Transistor,HIGH);
      delay(500);
      if (!SD.begin(SD_ChipSelectPin)) LedError();
      getFileName();
      SdFile::dateTimeCallback(dateTime); 
      audio.startRecording(filename,FreqMuestreo,MIC);
      digitalWrite(LED_Work,HIGH);
      Siguiente=false;
      Controlador_boton = true;
      tiempo_fichero = millis();
    }
  }
  if(!digitalRead(BotonBreak)){ 
    sleep_enable();
    attachInterrupt(0, wakeUp, LOW);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    audio.stopRecording(filename);
    digitalWrite(LED_Work,LOW);
    Controlador_boton = false;
    //digitalWrite(Transistor,LOW); 
    delay(500); 
    sleep_cpu();
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
  if(Controlador_boton==1){
    audio.stopRecording(filename);
    digitalWrite(LED_Work,LOW);
  }
  Controlador_boton=false;  
  time_t t;
  t=RTC.get();
  HORA_2=hour(t)+10;
  MIN_2=1;
  if(HORA_2 >= 24){
    HORA_2 = HORA_2 - 24;
  }
  if (HORA_2 >= HoraDormir || HORA_2 < HoraInicio){
    HORA_2=(HoraInicio + 3);
    MIN_2=MinInicio;
  }
  RTC.setAlarm(ALM2_MATCH_HOURS , 0, MIN_2, HORA_2, 0);           
  RTC.alarm(ALARM_2);
  //digitalWrite(Transistor,LOW); 
  delay(500); 
  sleep_cpu();
}
//==============================================================================

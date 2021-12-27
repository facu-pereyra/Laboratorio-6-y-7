/**
******************************
* El Arduino EMPIEZA DORMIDO *
******************************

* Boton start y Alarma_1 (configurada inicialmente mediante "Inicio_grabacion[]") da el inicio de grabación 
  cuya frecuencia de muestreo la da la variable "FreqMuestreo"

* Boton break y Alarma_2 (configurada inicialmente mediante "Final_grabacion[]") corta la grabación y duerme 
  hasta que suene la alarma 1

* Las alarmas pueden volver a sonar cada cierto tiempo (de forma cíclica), donde la hora y minuto que se 
  repita está dada por la variable "Ciclo_alarma[]". Ejemplo: si quiero que la alarma suene cada 3 horas
  y 15 minutos, entonces Ciclo_alarma[]={3,15}.

* Si la Alarma_1 y Alarma_2 debería sonar entre el "Tiempo_final[]" y "Inicio_grabacion[]", entonces 
  se reprograma las Alarmas para "Inicio_grabacion[]" (Alarma_1), y "Final_grabacion[]" (Alarma_2). 
    
* Hay salto de fichero donde su duración está dada por la variable "Duracion"

* El nombre del archivo está compuesto por el día más la hora, así por ejemplo, si se graba un archivo el 24 a las 16:07:35, 
  entonces el archivo se llamará 24160735.wav

* Cuando hay que grabar y no tiene la tarjeta SD salta error (titila 6 veces el led ROJO) y luego se reinicia 
  el Arduino.  

* Cuando se llega a "Tiempo_final[]" se duerme el arduino cortando la grabación. Se duerme hasta "Inicio_grabacion"
  o cuando suene la alarma
    
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
//==============================================================================
//Configurable

//Variables: 

uint8_t Duracion = 15; //Duracion de los ficheros, esta en minutos

char* Inicio_grabacion[] = {10,0}; //Hora y minuto de inicio de grabacion.
char* Final_grabacion[] = {12,0}; //Hora y minuto de finalización del primer período de grabación.
char* Ciclo_alarma[] = {1,0}; //Tiempo desde Inicio_grabacion para comenzar el 2do período 
char* Tiempo_final[] = {19,0}; //Hora y minuto de que finaliza el sistema y se duerme hasta Inicio_grabacion

uint16_t FreqMuestreo = 22000; //Frecuencia de muestreo de la grabación

//Conexiones:

// El boton_start se conecta en el pin interruptPin (D2)
uint8_t Transistor = 3;
uint8_t LED_Work = 4;
uint8_t LED_Error = 5;
uint8_t RST_PIN = 6; 
uint8_t BotonBreak = 7;

//==============================================================================
//==============================================================================
//NO CONFIGURABLE

char filename[] = "00000000.wav"; 

uint8_t HoraInicio=Inicio_grabacion[0];
uint8_t MinInicio=Inicio_grabacion[1];

uint8_t HoraFinal=Final_grabacion[0];
uint8_t MinFinal=Final_grabacion[1];

uint8_t HoraDormir=Tiempo_final[0];
uint8_t MinDormir=Tiempo_final[1];

uint8_t H_Alarma = Ciclo_alarma[0];
uint8_t M_Alarma = Ciclo_alarma[1];

volatile boolean Siguiente = false; //para ir al próximo fichero
volatile boolean Controlador_boton = false;

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
  //----------------------------------------------
  //Cambiar frecuencia del reloj
  //CLKPR = 0x80; 
  //CLKPR = 0x01; 
  //----------------------------------------------
  rtc.begin();
  DateTime now = rtc.now();
  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(LED_Work,OUTPUT);
  pinMode(LED_Error,OUTPUT);
  pinMode(MIC, INPUT);
  pinMode(BotonBreak, INPUT_PULLUP);
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
  RTC.setAlarm(ALM1_MATCH_HOURS , 0, MinInicio, HoraInicio, 1);  //Se setea la Hora del primer inicio  
  RTC.setAlarm(ALM2_MATCH_HOURS, 0, MinFinal, HoraFinal, 1);  //Se pone la hora del primer corte de grabacion
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
  if (!digitalRead(interruptPin)){
    time_t t = RTC.get();
    if (RTC.alarm(ALARM_1)){             
      if(Controlador_boton==0){
        digitalWrite(Transistor,HIGH);
        delay(500);
        if (!SD.begin(SD_ChipSelectPin)) LedError();
        getFileName();
        SdFile::dateTimeCallback(dateTime); 
        audio.startRecording(filename,FreqMuestreo,MIC);
        digitalWrite(LED_Work,HIGH);
        Siguiente=false;
        Controlador_boton=true;
        tiempo_fichero = millis();
      }            
      t=RTC.get();
      HORA=hour(t)+H_Alarma;
      MIN=minute(t)+M_Alarma;
      if(HORA >= 24){
        HORA = HORA - 24;
      }
      if(MIN >= 60){
        MIN = MIN - 60;
        HORA = HORA + 1;
      }
      if ((HORA >= HoraDormir && MIN >= MinDormir) || (HORA < HoraInicio && MIN < MinInicio) ){
        HORA=HoraInicio;
        MIN=MinInicio;
      }
      RTC.setAlarm(ALM1_MATCH_HOURS , 0, MIN, HORA, 1);  
      RTC.alarm(ALARM_1);
	  RTC.alarmInterrupt(ALARM_1, true);  
    }        
    else if (RTC.alarm(ALARM_2)){
      Siguiente=false;
      Dormir_Alarma2();          
    }
    else{ 									//Este else es para el botón start
      digitalWrite(Transistor,HIGH);
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
    digitalWrite(Transistor,LOW); 
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
  if(now.hour()==HoraDormir && now.minute()==MinDormir){
    audio.stopRecording(filename);
	  Controlador_boton=false; 
    digitalWrite(LED_Work,LOW);
    digitalWrite(Transistor,LOW);
    sleep_enable();
    attachInterrupt(0, wakeUp, LOW);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); 
    delay(500); 
    sleep_cpu();
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
  HORA_2=hour(t)+H_Alarma;  
  MIN_2=minute(t)+M_Alarma;
  if(HORA_2 >= 24){
    HORA_2 = HORA_2 - 24;
  }
  if(MIN_2 >= 60){
    MIN_2 = MIN_2 - 60;
    HORA_2 = HORA_2 + 1;
  }
  if ((HORA_2 >= HoraDormir && MIN_2 >= MinDormir) || (HORA_2 < HoraInicio && MIN_2 < MinInicio)){
    HORA_2=HoraFinal;
    MIN_2=MinFinal;
  }
  RTC.setAlarm(ALM2_MATCH_HOURS , 0, MIN_2, HORA_2, 1);           
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_2, true);
  digitalWrite(Transistor,LOW); 
  delay(500); 
  sleep_cpu();
}
//==============================================================================

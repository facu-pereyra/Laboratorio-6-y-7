/**
******************************
* El Arduino EMPIEZA DORMIDO *
******************************

* Alarma_1 da el inicio de grabación cuya frecuencia de muestreo la da la variable "FreqMuestreo".

* Alarma_2 corta la grabación y duerme hasta la alarma 1.

* Hay 2 botones. Un botón despierta el arduino e inicia la grabación. 
  El BotonBreak corta la grabación y pone a dormir al arduino
  
* Hay salto de fichero donde su duración está dada por la variable "Duracion"

* El nombre del archivo está compuesto por el día más la hora, así por ejemplo, si se graba un archivo el 24 a las 16:07:35, 
 entonces el archivo se llamará 24160735.wav

* Cuando se toca el botón para grabar y no tiene la tarjeta SD salta error (titila 6 veces el led ROJO) y luego se reinicia 
 el Arduino. 

* Manda el pin "Pin_Speaker" a LOW a tantos minutos (dado por "Able_Speaker")iniciada la grabación, luego 
  manda a HIGH a los x minutos dado por la variable "Disable_Speaker". Presionando el botón "BotonBreak",
  tambien manda a HIGH el pin "Pin_Speaker".
 
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
uint8_t Able_Speaker = 10; //Tiempo desde que empezó a grabar para activar el speaker
uint8_t Disable_Speaker = 15; //Duración de la reproduccion del archivo wav

uint16_t FreqMuestreo = 22000; //Frecuencia de muestreo en Hz

uint8_t HoraInicio = 10; //Empieza a grabar a esta hora. A las 10 horas se repite el proceso
uint8_t MinInicio = 5;

uint8_t HoraFinal = 12; //Termina de grabar a esta hora y se duerme. A las 10 horas se repite el proceso
uint8_t MinFinal = 5; 

//==============================================================================
char filename[] = "00000000.wav"; 

uint8_t Transistor = 3;
uint8_t LED_Work = 4;
uint8_t LED_Error = 5;
uint8_t RST_PIN = 6; 
uint8_t BotonBreak = 7;
uint8_t Pin_Speaker = 8;

volatile boolean Siguiente = false; //para ir al próximo fichero
volatile boolean Controlador_boton = false;
volatile boolean Grabando = false; //Para saber si graba así activa el pin para rerpoducir
volatile boolean Controlador = false; 


unsigned long tiempo_grabando; //Tiempo que se esta grabando
unsigned long tiempo_speaking; //Tiempo desde que se esta speakeando
unsigned long tiempo_fichero; //Variable del millis()
unsigned long Aviso_Speaker = Able_Speaker * 60000;
unsigned long Sacar_Speaker = Disable_Speaker * 60000;
unsigned long SaltoFichero = Duracion * 60000; //60 seg * 1000 (mili) = 60000 milisegundos

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
  
  //CLKPR = 0x80; // (1000 0000) enable change in clock frequency
  //CLKPR = 0x01; // (0000 0001) use clock division factor 2 to reduce the frequency from 16 MHz to 8 MHz
  //----------------------------------------------
  rtc.begin();
  DateTime now = rtc.now();
  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(LED_Work,OUTPUT);
  pinMode(LED_Error,OUTPUT);
  pinMode(MIC, INPUT);
  pinMode(Pin_Speaker,OUTPUT);
  pinMode(BotonBreak, INPUT_PULLUP);
  pinMode(Transistor, OUTPUT);

  digitalWrite(Pin_Speaker,HIGH); 
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
        Grabando = true;
        tiempo_grabando=millis();
        Siguiente=false;
        Controlador_boton=true;
        tiempo_fichero = millis();
      }            
      t=RTC.get();
      RTC.setAlarm(ALM1_MATCH_HOURS , 0, MinInicio, HoraInicio, 1);  
      RTC.alarm(ALARM_1);
	    RTC.alarmInterrupt(ALARM_1, true);	  
    }        
    else if (RTC.alarm(ALARM_2)){
      Siguiente=false;
      Dormir_Alarma2();          
    }
    else{ 									//Este else es para el botón
      digitalWrite(Transistor,HIGH);
      delay(500);
      if (!SD.begin(SD_ChipSelectPin)) LedError();
      getFileName();
      SdFile::dateTimeCallback(dateTime); 
      audio.startRecording(filename,FreqMuestreo,MIC);
      digitalWrite(LED_Work,HIGH);
      Siguiente=false;
      Controlador_boton = true;
      Grabando = true;
      tiempo_grabando=millis();
      tiempo_fichero = millis();
    }
  }
  if(!digitalRead(BotonBreak)){ 
    sleep_enable();
    attachInterrupt(0, wakeUp, LOW);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    audio.stopRecording(filename);
    digitalWrite(LED_Work,LOW);
    digitalWrite(Pin_Speaker,HIGH);
    Controlador_boton = false;
    Grabando = false; 
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
    if(Grabando==1 && (millis() - tiempo_grabando) > Aviso_Speaker){
      digitalWrite(Pin_Speaker,LOW);
      tiempo_speaking=millis();
      Grabando = false;
      Controlador=true;
    }
  if((millis() - tiempo_speaking) > Sacar_Speaker && Controlador==1){
      digitalWrite(Pin_Speaker,HIGH);
      Grabando = false;
      Controlador = false;
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
  Grabando = false;
  time_t t;
  t=RTC.get();
  RTC.setAlarm(ALM2_MATCH_HOURS , 0, MinFinal, HoraFinal, 1);           
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_2, true);
  digitalWrite(Transistor,LOW); 
  delay(500); 
  sleep_cpu();
}
//==============================================================================

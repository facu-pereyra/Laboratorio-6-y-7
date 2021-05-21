/*
Empieza a grabar cuando Botonstart==1 y se detiene cuando BotonBreak==1
Graba un solo archivo a la vez o no, depende de si es fecha u hora
*/

#include <SD.h>
#include <SPI.h>
#include <TMRpcm.h>
#include "RTClib.h"     // para el RTC

#define SD_ChipSelectPin 10  //example uses hardware SS pin 53 on Mega2560

TMRpcm audio;   // create an object for use in this sketch 
#define MIC A0

uint8_t BotonStart = 2; //Boton de Comienzo: Inicio de Grabación
uint8_t BotonBreak = 3; //Boton de Interrupción: Cortar Fichero  
uint8_t LED_Error = 4; //si parpadea hubo error, si está encendido es que se mandó a Detener
uint8_t LED_Work  = 7; //cuando está grabando está encendido

char filename[] = "_00000000_000000.wav"; 

//------------------------------------------------------------------------------------------
//==============================================================================
  RTC_DS3231 rtc;

void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();
  *date = FAT_DATE(now.year(), now.month(),  now.day());
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}
//==============================================================================
//------------------------------------------------------------------------------
void getFileName(){
    DateTime now = rtc.now();
    //sprintf(filename, "%%02d%02d%02d.wav", now.year(), now.month(), now.day());
    sprintf(filename, "%02d%02d%02d.wav", now.hour(), now.minute(), now.second());
}
//------------------------------------------------------------------------------------------
//==============================================================================
void LedFinal(){
  digitalWrite(LED_Work,   LOW);
  delay(500);
  digitalWrite(LED_Work,   HIGH);
  delay(500);
  digitalWrite(LED_Work,   LOW);
}
//==============================================================================

void setup() {
  
  //audio.speakerPin = 11; //5,6,11 or 46 on Mega, 9 on Uno, Nano, etc
  pinMode(12,OUTPUT);  //Pin pairs: 9,10 Mega: 5-2,6-7,11-12,46-45
//  Serial.begin(9600);
  
  rtc.begin();
   
  if (rtc.lostPower()) {
    // following line sets the RTC to the date & time this sketch was compiled (setea el dia y hora de la compu)
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0)); Esta linea es para ajustar de forma manual el día y hora
   } 
  
  if (!SD.begin(SD_ChipSelectPin)) {  
    return;
  }
  audio.CSPin = SD_ChipSelectPin;
  pinMode(MIC, INPUT);  // Microphone
  pinMode(LED_Error, OUTPUT);
  pinMode(LED_Work, OUTPUT); 
  pinMode(BotonStart, INPUT_PULLUP);
  pinMode(BotonBreak, INPUT_PULLUP);
}
void loop() {
  DateTime now = rtc.now();
  getFileName();
    
  delay(100);
  if (!digitalRead(BotonStart)) { 
    audio.startRecording(filename,16000,MIC);
    //digitalWrite(LED_Error, LOW);
    digitalWrite(LED_Work, HIGH);
  }
  else if(!digitalRead(BotonBreak)) {
      audio.stopRecording(filename);
      LedFinal();
  }
}

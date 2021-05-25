/*
Empieza a grabar cuando:
  Botonstart==1 
  Llega a la hora Inicial indicada
Se detiene la grabación cuando:
  BotonBreak==1
  Llega a la hora Final indicada
Graba un solo archivo a la vez o no, depende de si el filename es la fecha u hora
Entre el comienzo y el final de un nuevo archivo aparece un archivo de 1kb
  Importante:
Tengo que poner los segundos finales porque sino no me corta la grabación. Con los segundos puestos corta a la hora indicada

*/

#include <SD.h>
#include <SPI.h>
#include <TMRpcm.h>
#include "RTClib.h"     // para el RTC

#define SD_ChipSelectPin 10  //example uses hardware SS pin 53 on Mega2560

TMRpcm audio;   // create an object for use in this sketch 
#define MIC A0
//--------------------------------------------------------------------------------------------
//Defino variables o constantes

uint8_t HoraInicio = 14;
uint8_t MinInicio  = 18;
uint8_t SecInicio  = 0;

uint8_t HoraFinal = 14;
uint8_t MinFinal  = 20;
uint8_t SecFinal  = 0;

uint8_t BotonStart = 2; //Boton de Comienzo: Inicio de Grabación
uint8_t BotonBreak = 3; //Boton de Interrupción: Cortar Fichero  
uint8_t LED_Error = 4; //si parpadea hubo error, si está encendido es que se mandó a Detener
uint8_t LED_Work  = 7; //cuando está grabando está encendido
uint8_t RST_PIN = 6;

unsigned long t; //Variable del millis()
int SaltoFichero=6000; //Está en milisegundos
volatile boolean Siguiente = false; //para ir al próximo fichero

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
    sprintf(filename, "%02d%02d%02d.wav", now.hour(), now.minute(), now.second());
}
//--------------------------------------------------------------------------------
//==============================================================================
void LedError() {
  digitalWrite(LED_Work, LOW);  //apago LED de trabajo
  for (int i = 1; i < 6; i++) { // parpadea el de error
    digitalWrite(LED_Error, HIGH);
    delay(1000);
    digitalWrite(LED_Error, LOW);
    delay(1000);      
  }
  pinMode(RST_PIN,  OUTPUT);
  digitalWrite(RST_PIN, LOW);
}
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
    pinMode(12,OUTPUT);  
    rtc.begin();
    Serial.begin(9600);
  if (rtc.lostPower()) {
    // following line sets the RTC to the date & time this sketch was compiled (setea el dia y hora de la compu)
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0)); Esta linea es para ajustar de forma manual el día y hora
   } 
  
    if (!SD.begin(SD_ChipSelectPin)) LedError();
  
  // The audio library needs to know which CS pin to use for recording
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
  if (!digitalRead(BotonStart)||(Siguiente==1)||(now.hour()==HoraInicio && now.minute()==MinInicio && now.second()==SecInicio)) { 
    digitalWrite(LED_Error, LOW);
    digitalWrite(LED_Work, HIGH);
    Siguiente=false;
    audio.startRecording(filename,16000,MIC); //Serial.println("empezó");
    t=millis();
  }
  if ((millis() - t > SaltoFichero)&&(digitalRead(LED_Work))){        
      digitalWrite(LED_Work, LOW);
      //Serial.println("Estoy en el if");
      Siguiente=true;
      audio.stopRecording(filename);      
  }   
  else if (!digitalRead(BotonBreak)||(now.hour()==HoraFinal && now.minute()==MinFinal && now.second()==SecFinal)){      
       LedFinal();     
       audio.stopRecording(filename); //Serial.println("llegué");
       Siguiente=false;
       return; 
  }
} //cierra void loop

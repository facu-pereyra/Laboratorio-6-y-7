/*
*********************************
* El Arduino EMPIEZA DESPIERTO. *
*********************************
* Hay 2 alarmas, la Alarma_1 da el inicio de la grabación y la Alarma_2 da el corte. 
* Dada una hora (HoraDormir) el arduino duerme y despierta a la HoraInicio por medio de la Alarma_1.
* Hay salto de fichero dado por la variable SaltoFichero, donde la duración de cada fichero lo da la variable Duración.


*/

#include <avr/sleep.h>//this AVR library contains the methods that controls the sleep modes

#include <SD.h>
#include <SPI.h>
#include <TMRpcm.h>
#include <DS3232RTC.h>        // https://github.com/JChristensen/DS3232RTC
#define interruptPin 2 //Pin we are going to use to wake up the Arduino
#define SD_ChipSelectPin 10  //example uses hardware SS pin 53 on Mega2560

TMRpcm audio;   // create an object for use in this sketch 
#define MIC A0

const uint8_t RTC_INT_PIN(2);
char filename[] = "_00000000_000000.wav"; 
char timestamp[32];

#define BLOCK_COUNT = 1;
uint8_t RST_PIN = 4;
uint8_t LED_Error = 6;
uint8_t LED_Work = 7;
uint8_t LED_Control = 8;

volatile boolean Siguiente = false; //para ir al próximo fichero
unsigned long tiempo_fichero; //Variable del millis()
uint8_t Duracion = 1; //Duracion de los ficheros, esta en minutos
unsigned long SaltoFichero = Duracion * 60000; //60 seg * 1000 (mili) = 60000 milisegundos

//------------------------------------------
uint8_t HoraInicio;
uint8_t MinInicio;

uint8_t HoraDormir = 21;
uint8_t MinDormir = 0;
//-----------------------------------------
//------------------------------------------------------------------------------
void getFileName(){
    time_t t; //create a temporary time variable so we can set the time and read the time from the RTC
    t=RTC.get();//Gets the current time of the RTC
    sprintf(filename, "%02d%02d%02d%02d.wav", day(t), hour(t), minute(t), second(t));
}
//--------------------------------------------------------------------------------
void formatTime(char *buf, time_t t){
    char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)
    strcpy(m, monthShortStr(month(t)));
    sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d",
    hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t));
}
//------------------------------------------
void LedError() {
  digitalWrite(LED_Error, LOW);  //apago LED de trabajo
  digitalWrite(LED_Work,LOW);
  for (int j = 1; j < 6; j++) { // parpadea el de error
    digitalWrite(LED_Error, HIGH);
    delay(1000);
    digitalWrite(LED_Error, LOW);
    delay(1000);      
  }
  pinMode(RST_PIN,  OUTPUT);
  digitalWrite(RST_PIN, LOW);
}
//----------------------------------------------
void wakeUp(){
  sleep_disable();//Disable sleep mode
  detachInterrupt(0); //Removes the interrupt from pin 2; 
}
//---------------------------------------------
void setup() {
  // put your setup code here, to run once:

    Serial.begin(9600);
    //delay(1000);
    pinMode(RTC_INT_PIN, INPUT_PULLUP);
    pinMode(LED_Work,OUTPUT);
    pinMode(LED_Error,OUTPUT);
    pinMode(LED_Control,OUTPUT);
    pinMode(interruptPin,INPUT_PULLUP);//Set pin d2 to input using the buildin pullup resistor
    audio.CSPin = SD_ChipSelectPin; // The audio library needs to know which CS pin to use for recording
    pinMode(MIC, INPUT);  // Microphone
    
    /*
    //Poner en hora el reloj
    tmElements_t tm;
    tm.Hour = 18;               // set the RTC to an arbitrary time
    tm.Minute = 20;
    tm.Second = 45;
    tm.Day = 30;
    tm.Month = 8;
    tm.Year = 2021 - 1970;      // tmElements_t.Year is the offset from 1970
    RTC.write(tm);              // set the RTC from the tm structure
    */
    
    // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
    RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
    RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
    RTC.alarm(ALARM_1);
    RTC.alarm(ALARM_2);
    RTC.alarmInterrupt(ALARM_1, false);
    RTC.alarmInterrupt(ALARM_2, false);
    RTC.squareWave(SQWAVE_NONE);

    
    time_t t; //create a temporary time variable so we can set the time and read the time from the RTC
    t=RTC.get();//Gets the current time of the RTC
    RTC.setAlarm(ALM1_MATCH_HOURS , 0, 21, 18, 0);  //Se setea la Hora de inicio  
    RTC.setAlarm(ALM2_MATCH_HOURS, 0,  22, 18, 0);  //Se pone la hora del primer corte de grabacion
    RTC.alarm(ALARM_1); // clear the alarm flag
    RTC.alarm(ALARM_2);                 // ensure RTC interrupt flag is cleared
    RTC.squareWave(SQWAVE_NONE); // configure the INT/SQW pin for "interrupt" operation (disable square wave output)
    RTC.alarmInterrupt(ALARM_1, true); // enable interrupt output for Alarm 1            
    RTC.alarmInterrupt(ALARM_2, true);
    

}

void loop() {
time_t t = RTC.get();
// check to see if the INT/SQW pin is low, i.e. an alarm has occurred
    if (!digitalRead(RTC_INT_PIN)){
        if (RTC.alarm(ALARM_1)){             // resets the alarm flag if set. Acá va el || Siguente
            if ((hour(t) < HoraDormir)&&(hour(t) > HoraInicio)){            
              delay(500); // Para que cuando ocurra la transición dormir-grabar no haya ningún error
              if (!SD.begin(SD_ChipSelectPin)) LedError();
              getFileName();    
              audio.startRecording(filename,22000,MIC);
              digitalWrite(LED_Work,HIGH);
              digitalWrite(LED_Control,LOW);
              Siguiente=false;
              t=RTC.get();
              tiempo_fichero = millis();
              //HORA=hour(t)+10
              //if(HORA >= 24){
              //  HORA = HORA - 24
              //}
              //RTC.setAlarm(ALM1_MATCH_HOURS , 0, minute(t)+2, HORA, 0);  
              RTC.setAlarm(ALM1_MATCH_HOURS , 0, minute(t)+2, hour(t), 0); 
              RTC.alarm(ALARM_1); // clear the alarm flag
            }
            if((hour(t) >= HoraDormir)&&(hour(t) < HoraInicio)){
              Dormir_Alarma1();
            } 
        }        
        if (RTC.alarm(ALARM_2)){
          if((hour(t)<HoraDormir)&&(hour(t) > HoraInicio)){
            audio.stopRecording(filename); 
            Siguiente=false;
            Dormir_Alarma2();
          }
           else if((hour(t)>=HoraDormir)&&(hour(t) < HoraInicio)){
            Dormir_Alarma2_Absoluta();                     
           }
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
      audio.startRecording(filename,22000,MIC);
      digitalWrite(LED_Work,HIGH);
      Siguiente=false;
      t=RTC.get();
      tiempo_fichero = millis();
    }
}
//-------------------------------------------------------------------------
void Dormir_Alarma1(){
  sleep_enable();//Enabling sleep mode
  attachInterrupt(0, wakeUp, LOW);//attaching a interrupt to pin d2
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);//Setting the sleep mode, in our case full sleep
  digitalWrite(LED_Control,HIGH);
  digitalWrite(LED_Work,LOW);
  time_t t;
  t=RTC.get();
  RTC.setAlarm(ALM1_MATCH_HOURS , 0, MinInicio, HoraInicio, 0);              
  RTC.alarm(ALARM_1); // clear the alarm flag
  delay(500); //wait a second to allow the led to be turned off before going to sleep
  sleep_cpu();//activating sleep mode
}
//--------------------------------------------------------------------------
void Dormir_Alarma2(){
  sleep_enable();//Enabling sleep mode
  attachInterrupt(0, wakeUp, LOW);//attaching a interrupt to pin d2
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);//Setting the sleep mode, in our case full sleep
  digitalWrite(LED_Control,HIGH);
  digitalWrite(LED_Work,LOW);
  time_t t;
  t=RTC.get();
  RTC.setAlarm(ALM2_MATCH_HOURS , 0, minute(t)+2, hour(t), 0);           
  RTC.alarm(ALARM_2); // clear the alarm flag
  delay(500); //wait a second to allow the led to be turned off before going to sleep
  sleep_cpu();//activating sleep mode
}
//----------------------------------------------------------------------------
void Dormir_Alarma2_Absoluta(){
  sleep_enable();//Enabling sleep mode
  attachInterrupt(0, wakeUp, LOW);//attaching a interrupt to pin d2
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);//Setting the sleep mode, in our case full sleep
  digitalWrite(LED_Control,HIGH);
  digitalWrite(LED_Work,LOW);
  RTC.setAlarm(ALM2_MATCH_HOURS ,0, MinInicio , HoraInicio + 2, 0);           
  RTC.alarm(ALARM_2); // clear the alarm flag
  delay(500); //wait a second to allow the led to be turned off before going to sleep
  sleep_cpu();//activating sleep mode   
}
//-----------------------------------------------------------------------------

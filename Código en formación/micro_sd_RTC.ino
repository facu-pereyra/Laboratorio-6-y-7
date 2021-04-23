/*
Lo que quise pedir acá es que a partir de las 9 a las 21, el microfono grabe y que cuando sean las 9 aparezca una leyenda que diga
que comenzó a grabar, y a las 21 hs diga q dejó de grabar.
Me falta ver como lo grabo
Tira error, preguntar a Roberto

*/
// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"
//#include <Wire.h> Ver para que sirve esto 
RTC_DS3231 rtc; //Crea un objeto llamado rtc

bool grabar = true;    // variable de control con valor verdadero

#define ledsito 4
#define MIC A0
int sig=0

//Estas 2 de abajo son para la SD
#include <SPI.h>    // incluye libreria interfaz SPI
#include <SD.h>     // incluye libreria para tarjetas SD

//CS - pin 10

File Archivo;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(3000);
  pinMode(ledsito,OUTPUT); //Es el pin del led

  //Esta parte es de la SD
  Serial.print("Initializing SD card...");
  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  if (SD.exists("example.txt")) {
    Serial.println("example.txt exists.");
  } else {
    Serial.println("example.txt doesn't exist.");
  }
  //Serial.println("inicializacion correcta");  // texto de inicializacion correcta
  //archivo = SD.open("datos.txt", FILE_WRITE); // apertura para lectura/escritura de archivo datos.txt
  
  //Esta parte de abajo es para el RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled (setea el dia y hora de la compu)
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0)); Esta linea es para ajustar de forma manual el día y hora
  }
}

/*
  void led(){
  sig=analogRead(MIC);
  if (sig>250){digitalWrite(4,HIGH);} else{digitalWrite(4,LOW);} //En este caso estoy tomando q el valor de señal del pajaro es 250
}
*/
void loop() {
  // put your main code here, to run repeatedly:
  //led(); Tendría q poner eso si puse el void led
  sig=analogRead(MIC);
  
  DateTime fecha = rtc.now();
  if ( fecha.hour() == 9 && fecha.minute() == 0 ){  // si hora = 14 y minutos = 30
   if ( grabar == true ){        // si grabar tiene un valor verdadero
     Serial.println( "empiezo a grabar" );     // imprime en monitor serie texto Grabar
     grabar = false;         // carga valor falso en variable de control
    }             // para evitar ingresar mas de una vez
  }
  while (( fecha.hour() > 9 && fecha.minute() > 0 )&& ( fecha.hour() < 21 && fecha.minute() < 0 )){
   digitalWrite(4,HIGH);
   Serial.print(fecha.day());       // funcion que obtiene el dia de la fecha completa
   Serial.print("/");         // caracter barra como separador
   Serial.print(fecha.month());       // funcion que obtiene el mes de la fecha completa
   Serial.print("/");         // caracter barra como separador
   Serial.print(fecha.year());        // funcion que obtiene el año de la fecha completa
   Serial.print(" ");         // caracter espacio en blanco como separador
   Serial.print(fecha.hour());        // funcion que obtiene la hora de la fecha completa
   Serial.print(":");         // caracter dos puntos como separador
   Serial.print(fecha.minute());        // funcion que obtiene los minutos de la fecha completa
   Serial.print(":");         // caracter dos puntos como separador
   Serial.print(fecha.second());      // funcion que obtiene los segundos de la fecha completa
   Serial.print("; ");
   Serial.print(sig);   
   delay(1000);           // demora de 1 segundo
  }
  if ( fecha.hour() == 21 && fecha.minute() == 0 ){   // si hora = 21 y minutos = 0 restablece valor de
    digitalWrite(4,LOW);
    grabar = true;          // variable de control en verdadero 
    Serial.println( "dejo de grabar" ); //Quise poner q una vez que llegen las 21 hs el cosito deje de grabar
  }
}

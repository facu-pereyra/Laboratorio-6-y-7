// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"

RTC_DS3231 rtc; //Crea un objeto llamado rtc

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup () {

//Esta parte ni idea para q es
//#ifndef ESP8266
//  while (!Serial); // for Leonardo/Micro/Zero
//#endif

  Serial.begin(9600);

  delay(3000); // wait for console opening
//La parte de abajo es una verificación si hay comunicacion con el modulo
// rtc.begin: devuelve verdadero si la comunicacion es exitosa y falso en caso contrario
// ! rtc.begin: invierte el valor, por lo que si es falso, sigue el codigo y muestra en la pantalla "Couldn't find RTC" 
// El while es un bucle con la condición 1 q equivale a true por lo q es un bucle infinito
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

void loop () {
    DateTime fecha = rtc.now();

    Serial.print(fecha.year(), DEC);
    Serial.print('/');
    Serial.print(fecha.month(), DEC);
    Serial.print('/');
    Serial.print(fecha.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[fecha.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(fecha.hour(), DEC);
    Serial.print(':');
    Serial.print(fecha.minute(), DEC);
    Serial.print(':');
    Serial.print(fecha.second(), DEC);
    Serial.println();

    delay(1000);
//    Serial.print(" since midnight 1/1/1970 = ");
//    Serial.print(now.unixtime());
//    Serial.print("s = ");
//    Serial.print(now.unixtime() / 86400L);
//    Serial.println("d");
//
//    // calculate a date which is 7 days and 30 seconds into the future
//    DateTime future (now + TimeSpan(7,12,30,6));
//
//    Serial.print(" now + 7d + 30s: ");
//    Serial.print(future.year(), DEC);
//    Serial.print('/');
//    Serial.print(future.month(), DEC);
//    Serial.print('/');
//    Serial.print(future.day(), DEC);
//    Serial.print(' ');
//    Serial.print(future.hour(), DEC);
//    Serial.print(':');
//    Serial.print(future.minute(), DEC);
//    Serial.print(':');
//    Serial.print(future.second(), DEC);
//    Serial.println();
//
//    Serial.print("Temperature: ");
//    Serial.print(rtc.getTemperature());
//    Serial.println(" C");
//
//    Serial.println();
//    delay(3000);
}

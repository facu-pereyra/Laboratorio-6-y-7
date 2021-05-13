/*
Tiene errores, ver el chat con Roberto o sino la página https://arduino.stackexchange.com/questions/60046/trying-to-save-wav-files-with-new-name-every-loop

Cuando lo arregle lo modifico
*/

//////////////////////////////////////// SD CARD
#include <SD.h>
#include <SPI.h>
#include <TMRpcm.h>
#define SD_ChipSelectPin 10 //Corresponde al pin del CS
TMRpcm audio;
//////////////////////////////////////// SWITCH CASE
int audiofile = 0;     // # of recording
unsigned long i = 0;
bool recmode = 0;      // recording state
//////////////////////////////////////// SWITCH
int inPin = 2;         // input Switch Pin
int state = HIGH;      // the current state switch
int reading;           // the current reading from the switch

void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT);  // Microphone
  pinMode(inPin, INPUT_PULLUP); // Switch
  //////////////////////////////////////// SD CARD
  SD.begin(SD_ChipSelectPin);
  audio.CSPin = SD_ChipSelectPin;
}

void loop() {

  reading = digitalRead(inPin);
  ////////////////////////////////////////
  while (i < 300000) {
    i++;
  }
  i = 0;
  ////////////////////////////////////////
  if (reading == LOW) {
    if (recmode == 0) {
      recmode = 1;

      Serial.println("Recording");

      audiofile++; // To move case
      audio.startRecording("1.wav", 16000, A0);
    }
  }
  ////////////////////////////////////////
  else if (reading == HIGH) {
    recmode = 0;

    Serial.println("Hung-Up");
    audio.stopRecording("1.wav");
  }
}  

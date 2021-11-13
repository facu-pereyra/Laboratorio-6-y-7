/**
 * El código reproduce archivos de audio en formato wav de 8 bit, entre 8 a 32 kHz, en mono
 * 
 * Los archivos wav a reproducir se encuentran en la carpeta Audio
 * 
 * Los audios se reproducen de forma aleatoria mediante el nombre "ave_i" donde i=0, 1, 2, 3,...
 * 
 */
#include <SD.h>                   
#define SD_ChipSelectPin 10  
#include <TMRpcm.h>           
#include <SPI.h>

TMRpcm tmrpcm;   

//==============================================================================
//Variable a modificar

uint8_t num_cantos = 3; // Indica cuantos cantos hay en la carpeta de reproduccion

//==============================================================================

char fileName[20];    
uint8_t fileToPlay;
uint8_t LED_Error = 5;
uint8_t Speaker_pin = 6; //Pin de entrada que da el inicio o finalización del speaker
//==============================================================================
void LedError() {
  digitalWrite(LED_Error, LOW);  
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
void setup(){
  Serial.begin(9600);
  pinMode(Speaker_pin,INPUT);
  pinMode(LED_Error,OUTPUT);  
  tmrpcm.speakerPin = 9;   
  if (!SD.begin(SD_ChipSelectPin)) LedError(); 
  randomSeed(analogRead(A7)); 
}

void loop(){   
    while(digitalRead(Speaker_pin)==1){
      if(tmrpcm.isPlaying()==0){ 
        fileToPlay = random(0,num_cantos);
        sprintf(fileName, "Audio/ave_%d.wav", fileToPlay);  
        tmrpcm.play(fileName);
        Serial.println(fileName);
      }
    }
    if(digitalRead(Speaker_pin)==0){
      tmrpcm.stopPlayback();
      delay(2000); //Dejar esta línea mientras se prueba pq pasa rapido de 0 a 1
    }
}

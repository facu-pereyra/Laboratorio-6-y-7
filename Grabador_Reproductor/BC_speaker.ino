/**
 * El código reproduce archivos de audio en formato wav de 8 bit, entre 8 a 32 kHz, en mono
 * 
 * Los archivos wav a reproducir se encuentran en la carpeta Audio
 * 
 * Los audios se reproducen de forma aleatoria mediante el nombre "pajaro_i" donde i=0, 1, 2, 3,...
 * 
 * Con el botón "BotonBreak" cortas la reproducción y no se vuelve a reproducir a no ser que se reinicie el
 * arduino.
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
uint8_t Transistor = 3;
uint8_t Pin_Speaker = 4; //Pin de entrada que da el inicio o finalización del speaker
uint8_t LED_Error = 5;
uint8_t RST_PIN = 6;
uint8_t BotonBreak = 7; 

volatile boolean Controlador=true;
//==============================================================================
void LedError() {
  digitalWrite(LED_Error, LOW);  
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
void setup(){
  pinMode(Pin_Speaker,INPUT_PULLUP);
  pinMode(LED_Error,OUTPUT);
  pinMode(BotonBreak, INPUT_PULLUP);
  pinMode(Transistor, OUTPUT);
  
  tmrpcm.speakerPin = 9;
  digitalWrite(Transistor,HIGH);
  delay(500);   
  if (!SD.begin(SD_ChipSelectPin)) LedError(); 
  randomSeed(analogRead(A7));
  digitalWrite(Transistor,LOW); 
}

void loop(){   
    while(digitalRead(Pin_Speaker)==0){      
      digitalWrite(Transistor,HIGH);
      delay(500);
      if(Controlador==1 && tmrpcm.isPlaying()==0){
        delay(20000); 
        fileToPlay = random(0,num_cantos);
        sprintf(fileName, "Audio/pajaro_%d.wav", fileToPlay);  
        tmrpcm.play(fileName);
      }
      if(!digitalRead(BotonBreak)){
        tmrpcm.stopPlayback();
        Controlador=false;
      }
    }
    if(digitalRead(Pin_Speaker)==1){
      tmrpcm.stopPlayback();
      Controlador=true;
      digitalWrite(Transistor,LOW);
    }
}


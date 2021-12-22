/**
 * EMPIEZA DORMIDO
 * 
 * El código reproduce archivos de audio en formato wav de 8 bit, entre 8 a 32 kHz, en mono. 
   El nombre del archivo wav tiene que tener menos de 8 carácteres (sin considerar la extensión wav).
 * 
 * Los archivos wav a reproducir se encuentran en la carpeta Audio.
 * 
 * Los audios que reproducen son 2: "Real" y "Sint".
 * 
 * Se despierta a la hora dada por Comienza_reproduccion (Grabadora) por el pin 2 y se duerme a una hora dada por 
   Corta_reproduccion (Grabadora) mediante una señal del pin 8. 
 * 
 * En cada ciclo de reproducción se reproducen archivos wav distintos, primero el "Real" y luego el "Sint"
 * 
 * Hay un pin ("cable") determinado como output que hace marcar al grabador una especie de "delay"
   entre archivos wav a reproducir. Este delay se configura desde la GRABADORA. 
 * 
 * ****************************************************************************
 * SE UTILIZAN 3 PINES (+ GND interconectadas)  ENTRE GRABADORA Y REPRODUCTOR *
 * ****************************************************************************
 * 
 */

#include <avr/sleep.h>
#include <SD.h>            
#define SD_ChipSelectPin 10  
#include <TMRpcm.h>           
#include <SPI.h>
#define interruptPin 2 

TMRpcm audio;
//==============================================================================   
//==============================================================================
//Configurable

//Variables:

String wav_1 = "Audio/Real.wav"; // Nombre de los archivos wav a reproducir.
String wav_2 = "Audio/Sint.wav"; // El nombre tiene que tener menos de 8 carácteres (ej: Sint -> 4 carácteres)

//Conexiones:

uint8_t Transistor = 3;
uint8_t LED_Error = 5;
uint8_t RST_PIN = 6; // Se conecta al pin RST del arduino.
uint8_t Marca_delay = 7; 
uint8_t Duerme_speaker = 8;
uint8_t Speaker = 9; //Está conectado el Speaker, NO CAMBIAR PARA ARDUINO NANO O MINI.

//=================================================================================
//==============================================================================
//NO CONFIGURABLE

char fileName[20];    
String fileToPlay;

volatile boolean Controlador = false;
volatile boolean Aviso = false;
volatile boolean cont_delay =false;
volatile boolean Control_inicio = true;
volatile boolean Control_canto = false;

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
void wakeUp(){
  sleep_disable();
  detachInterrupt(0); 
}
//==============================================================================
void setup(){
  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(Duerme_speaker, INPUT_PULLUP);
  pinMode(Marca_delay, OUTPUT);
  pinMode(LED_Error,OUTPUT);
  pinMode(Transistor, OUTPUT);  
  audio.speakerPin = Speaker; 
  
  digitalWrite(Marca_delay,HIGH);
  
  delay(5000); //para que no se reproduzca el audio al inicio. 
  //----------------------------------
  digitalWrite(Transistor,LOW);     
  sleep_enable();
  attachInterrupt(0, wakeUp, LOW);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_cpu(); //Modo de bajo consumo
  //----------------------------------
}

void loop(){ 
  
  if(audio.isPlaying()==0 && Control_inicio==1){
    digitalWrite(Transistor,HIGH);
    digitalWrite(Speaker,HIGH);
    if(Aviso==0){
      fileToPlay=wav_1;
      Control_canto = true;
    }
    else if (Aviso==1){
      fileToPlay = wav_2;
      Control_canto = false;
    } 
    delay(100);
    if (!SD.begin(SD_ChipSelectPin)) LedError();
    delay(100);
    fileToPlay.toCharArray(fileName,20); // convierte un string en char
    audio.quality(1);
    audio.play(fileName);
    cont_delay=true;
    Control_inicio=false;    
  }
  
  if(audio.isPlaying()==0 && cont_delay==1){
    Control_inicio=true;
    cont_delay=false;
    digitalWrite(Marca_delay,LOW);
    delay(100);
    digitalWrite(Marca_delay,HIGH);
    digitalWrite(Speaker,LOW);
    //----------------------------------
    digitalWrite(Transistor,LOW);     
    sleep_enable();
    attachInterrupt(0, wakeUp, LOW);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    delay(500); 
    sleep_cpu(); //Modo de bajo consumo
    //----------------------------------
    
  }
 
 if (digitalRead(Duerme_speaker)==0){
    if(Control_canto==1){
      Aviso=true;
    }
    else if (Control_canto==0){
      Aviso=false;
    }
    Control_inicio=true;
    audio.stopPlayback();
    digitalWrite(Speaker,LOW); 
	//----------------------------------
    digitalWrite(Transistor,LOW);     
    sleep_enable();
    attachInterrupt(0, wakeUp, LOW);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    delay(500); 
    sleep_cpu(); //Modo de bajo consumo
    //----------------------------------
  }
}

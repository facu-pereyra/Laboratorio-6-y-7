/**
 *  							< Versión 12.2_b Incluido el ADC>
 *
 * Esta versión no incluye el reset por WDT por problemas con el bootloader viejo del Arduino Nano. Se elimina el watchdog
 *               
 * Los datos se graban a un fichero binario. Máximo probado con 10 bits es de 32 kHz.
 * El terminal de entrada habilitado por defecto es el A0
 * Para recuperar los datos ejecutar el programa de python
 * 
 * Guarda los datos si hay Error 
 * Crea ficheros uno detras del otro
 * Incluye el RTC para la fecha y hora
 * El nombre del fichero es Nombre + _ + año_mes_día + hh_mm_ss
 * No hay Puerto Serie
 * 
 * Inicia a una Hora (HH:MM) que se especifica.
 * Además Inicia al colocar el PinStart (2) a GND
 * Una vez que se llena un fichero se pasa automáticamente al próximo
 * 
 * Detiene a una Hora (HH:MM) que se especifica.
 * Si se Detiene por la Hora (HH:MM) que se especifica, entonces no se pasa a registrar en otro fichero.
 * Además Detiene al oprimirse el botón conectado al Pin 3. En este caso si se pasa automáticamente a otro fichero.
 * 
 * Con umbral para la grabación. Guarda los datos en el buffer si estos superan el umbral
 */

#ifdef __AVR__
#include <SPI.h>
#include <SdFat.h>
#include "C:/Users/facunorma/Documents/Arduino/libraries/SdFat/SdFatUtil.h"
#include "AnalogBinLogger.h"
//#include <avr/wdt.h> 	// para hacer Reset automático
#include "RTClib.h"     // para el RTC
//*****************************************
//Esto lo agregue yo
//#include <TMRpcm.h>
//TMRpcm audio;   // create an object for use in this sketch 
//*************************************************************
   	//int umbral = 256;
	
	/**  NO SE INCLUYE EL CERO A LA IZQUIERDA
    *   la fecha se pone en hora militar
    *   no incluye "0" como primer digito
    *   Ejemplo: 3:9, corresponde con 03:09 am
    *   3:16, corresponde con 03:16 am
    *   16:12, corresponde con 04:12 pm
    **/
   uint8_t HoraInicio = 18;
   uint8_t MinInicio  = 2;

   uint8_t HoraFinal = 18;
   uint8_t MinFinal  = 3;
   
   const uint32_t FILE_BLOCK_COUNT = 51200; //Aquí se modifica el tamaño del fichero
										  // con 256000 a 25 kHz son @ 4,33 minutos
   
	#define FILE_BASE_NAME "TEST" //4 caracteres ahora mismo
//*************************************************************
	const uint8_t PIN_LIST[] = {0}; 	// Terminal Ain, poner el numero del terminal
										// {0, 1, 2, 3, 4};
	const uint16_t  SAMPLE_RATE = 25000;  // Must be 0.25 or greater. (float)
	
	uint8_t LED_Error = 4; //si parpadea hubo error, si está encendido es que se mandó a Detener
	uint8_t LED_Work  = 7; //cuando está grabando está encendido
	uint8_t LED_Final = 5; //cuando escribió un fichero se enciende, luego se apaga
   
	uint8_t PinStart = 2; //Pin de Interrupción: Inicio de Grabación
	uint8_t PinBreak = 3; //Pin de Interrupción: Cortar Fichero	
	
	uint8_t RST_PIN = 6;
//==============================================================================
// End of configuration constants.
//==============================================================================
	
	volatile boolean Siguiente = false; //para ir al próximo fichero
	volatile boolean Detener   = false;	

    const float SAMPLE_INTERVAL = 1.0 / SAMPLE_RATE;
	
	#define ROUND_SAMPLE_INTERVAL 1
	#define ADC_PRESCALER 4 // F_CPU/16 1000 kHz on an Uno
	
	uint8_t const ADC_REF = (1 << REFS1) | (1 << REFS0);  // atMega328 es 1.1

//------------------------------------------------------------------------------
	const uint8_t BUFFER_BLOCK_COUNT = 1;
	// Dimension for queues of 512 byte SD blocks.
	const uint8_t QUEUE_DIM = 16;  // Must be a power of two!

// Temporary log file.  Will be deleted if a reset or power failure occurs.

	SdFat sd;
	SdBaseFile wavFile;

#define TMP_FILE_NAME "tmp.wav"

   const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
   
   char binName[24] = FILE_BASE_NAME "0000.wav";
   
// Number of analog pins to log.
   const uint8_t PIN_COUNT = sizeof(PIN_LIST) / sizeof(PIN_LIST[0]);

// Minimum ADC clock cycles per sample interval
   const uint16_t MIN_ADC_CYCLES = 15;

// Extra cpu cycles to setup ADC with more than one pin per sample.
   const uint16_t ISR_SETUP_ADC = 100; //100

// Maximum cycles for timer0 system interrupt, millis, micros.
   const uint16_t ISR_TIMER0 = 160;

//==============================================================================
// Set RECORD_EIGHT_BITS non-zero to record only the high 8-bits of the ADC.
	#define RECORD_EIGHT_BITS 0

#if RECORD_EIGHT_BITS
   const size_t SAMPLES_PER_BLOCK = DATA_DIM8 / PIN_COUNT;
typedef block8_t block_t;
#else  // RECORD_EIGHT_BITS
   const size_t SAMPLES_PER_BLOCK = DATA_DIM16 / PIN_COUNT;
typedef block16_t block_t;
#endif // RECORD_EIGHT_BITS

   block_t* emptyQueue[QUEUE_DIM];
   uint8_t emptyHead;
   uint8_t emptyTail;

   block_t* fullQueue[QUEUE_DIM];
   volatile uint8_t fullHead;  // volatile insures non-interrupt code sees changes.
   uint8_t fullTail;

// queueNext assumes QUEUE_DIM is a power of two
   inline uint8_t queueNext(uint8_t ht) {
   return (ht + 1) & (QUEUE_DIM - 1);
}

//==============================================================================
	RTC_DS1307 rtc;
	
void dateTime(uint16_t* date, uint16_t* time) {
	DateTime now = rtc.now();
	*date = FAT_DATE(now.year(), now.month(),  now.day());
	*time = FAT_TIME(now.hour(), now.minute(), now.second());
}
//==============================================================================
// Interrupt Service Routines

// Pointer to current buffer.
   block_t* isrBuf;

// Need new buffer if true.
   bool isrBufNeeded = true;

// overrun count
   uint16_t isrOver = 0;

// ADC configuration for each pin.
   uint8_t adcmux[PIN_COUNT];
   uint8_t adcsra[PIN_COUNT];
   uint8_t adcsrb[PIN_COUNT];
   uint8_t adcindex = 1;

// Insure no timer events are missed.
   volatile bool timerError = false;
   volatile bool timerFlag = false;
//------------------------------------------------------------------------------
// ADC done interrupt.

//uint16_t rampa = 0; 					// rampa de subida, pruebas de escritura

ISR(ADC_vect) {
  // Read ADC data.
#if RECORD_EIGHT_BITS
  uint8_t d = ADCH;
#else  // RECORD_EIGHT_BITS
  // This will access ADCL first.
  uint16_t d = ADC;
  //uint16_t d = rampa; //rampa de subida
  //rampa++; //rampa de subida
#endif  // RECORD_EIGHT_BITS
//------------------------------------------------------------------------------

  if (isrBufNeeded && emptyHead == emptyTail) {
    // no buffers - count overrun
    if (isrOver < 0XFFFF) {isrOver++;}

    // Avoid missed timer error.
    timerFlag = false;
    return;
  }
  // Start ADC
  if (PIN_COUNT > 1) {
    ADMUX = adcmux[adcindex];
    ADCSRB = adcsrb[adcindex];
    ADCSRA = adcsra[adcindex];
    if (adcindex == 0) {timerFlag = false;}
    adcindex =  adcindex < (PIN_COUNT - 1) ? adcindex + 1 : 0;
  } else {timerFlag = false;}
  
  // Check for buffer needed.
  if (isrBufNeeded) {
    // Remove buffer from empty queue.
    isrBuf = emptyQueue[emptyTail];
    emptyTail = queueNext(emptyTail);
    isrBuf->count = 0;
    isrBuf->overrun = isrOver;
    isrBufNeeded = false;
  }
  
  // Store ADC data.
  // Para el umbral de detección
  
  //if (d>umbral){
	  isrBuf->data[isrBuf->count++] = d;
	  
	  // Check for buffer full.
	  if (isrBuf->count >= PIN_COUNT * SAMPLES_PER_BLOCK) {
		  // Put buffer isrIn full queue.
		  uint8_t tmp = fullHead;  // Avoid extra fetch of volatile fullHead.
		  fullQueue[tmp] = (block_t*)isrBuf;
		  fullHead = queueNext(tmp);
		  // Set buffer needed and clear overruns.
		  isrBufNeeded = true;
		  isrOver = 0;
		}
	//}
  else {digitalWrite(LED_Work,   LOW);}	  
}
//------------------------------------------------------------------------------
// timer1 interrupt to clear OCF1B
ISR(TIMER1_COMPB_vect) {
  // Make sure ADC ISR responded to timer event.
  if (timerFlag) {
	timerError = true;
  }
  timerFlag = true;
}
//==============================================================================
void LedError() {
	digitalWrite(LED_Work, LOW);  //apago LED de trabajo
	for (int i = 1; i < 6; i++) { // parpadea el de error
		digitalWrite(LED_Error, HIGH);
		delay(1000);
		digitalWrite(LED_Error, LOW);
		delay(1000);			
	}
	pinMode(RST_PIN, 	OUTPUT);
	digitalWrite(RST_PIN, LOW);
}
//==============================================================================
#if ADPS0 != 0 || ADPS1 != 1 || ADPS2 != 2
#error unexpected ADC prescaler bits
#endif
//------------------------------------------------------------------------------
// initialize ADC and timer1
void adcInit(metadata_t* meta) {
  uint8_t adps;  // prescaler bits for ADCSRA
  uint32_t ticks = F_CPU * SAMPLE_INTERVAL + 0.5; // Sample interval cpu cycles.

  if (ADC_REF & ~((1 << REFS0) | (1 << REFS1)))	LedError(); //aquí nunca va a dar error
  
#ifdef ADC_PRESCALER
  if (ADC_PRESCALER > 7 || ADC_PRESCALER < 2)	LedError();	 //aquí nunca va a dar error
  
  adps = ADC_PRESCALER;
#else  // ADC_PRESCALER
  // Allow extra cpu cycles to change ADC settings if more than one pin.
  int32_t adcCycles = (ticks - ISR_TIMER0) / PIN_COUNT;
  - (PIN_COUNT > 1 ? ISR_SETUP_ADC : 0);

  for (adps = 7; adps > 0; adps--) {
    if (adcCycles >= (MIN_ADC_CYCLES << adps))	break;
  }
#endif  // ADC_PRESCALER
  meta->adcFrequency = F_CPU >> adps;
  if (meta->adcFrequency > (RECORD_EIGHT_BITS ? 2000000 : 1000000))	LedError(); // aquí nunca va a dar error
  
#if ROUND_SAMPLE_INTERVAL
  // Round so interval is multiple of ADC clock.
  ticks += 1 << (adps - 1);
  ticks >>= adps;
  ticks <<= adps;
#endif  // ROUND_SAMPLE_INTERVAL

  if (PIN_COUNT > sizeof(meta->pinNumber) / sizeof(meta->pinNumber[0]))	LedError(); //aquí nunca va a dar error

  meta->pinCount = PIN_COUNT;
  meta->recordEightBits = RECORD_EIGHT_BITS;

  for (int i = 0; i < PIN_COUNT; i++) {
    uint8_t pin = PIN_LIST[i];
    if (pin >= NUM_ANALOG_INPUTS)	LedError(); //aquí nunca va a dar error
	
    meta->pinNumber[i] = pin;

    // Set ADC reference and low three bits of analog pin number.
    adcmux[i] = (pin & 7) | ADC_REF;
    if (RECORD_EIGHT_BITS) {
      adcmux[i] |= 1 << ADLAR;
    }

    // If this is the first pin, trigger on timer/counter 1 compare match B.
    adcsrb[i] = i == 0 ? (1 << ADTS2) | (1 << ADTS0) : 0;
#ifdef MUX5
    if (pin > 7) {
      adcsrb[i] |= (1 << MUX5);
    }
#endif  // MUX5
    adcsra[i] = (1 << ADEN) | (1 << ADIE) | adps;
    adcsra[i] |= i == 0 ? 1 << ADATE : 1 << ADSC;
  }

  // Setup timer1
  TCCR1A = 0;
  uint8_t tshift;
  if (ticks < 0X10000) {
    // no prescale, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
    tshift = 0;
  } else if (ticks < 0X10000 * 8) {
    // prescale 8, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    tshift = 3;
  } else if (ticks < 0X10000 * 64) {
    // prescale 64, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11) | (1 << CS10);
    tshift = 6;
  } else if (ticks < 0X10000 * 256) {
    // prescale 256, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS12);
    tshift = 8;
  } else if (ticks < 0X10000 * 1024) {
    // prescale 1024, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS12) | (1 << CS10);
    tshift = 10;
  } else	LedError(); //aquí nunca va a dar error
  
  // divide by prescaler
  ticks >>= tshift;
  // set TOP for timer reset
  ICR1 = ticks - 1;
  // compare for ADC start
  OCR1B = 0;

  // multiply by prescaler
  ticks <<= tshift;

  // Sample interval in CPU clock ticks.
  meta->sampleInterval = ticks;
  meta->cpuFrequency = F_CPU;
  float sampleRate = (float)meta->cpuFrequency / meta->sampleInterval;
 }
//------------------------------------------------------------------------------
// enable ADC and timer1 interrupts
void adcStart() {
  // initialize ISR
  isrBufNeeded = true;
  isrOver = 0;
  adcindex = 1;

  // Clear any pending interrupt.
  ADCSRA |= 1 << ADIF;

  // Setup for first pin.
  ADMUX = adcmux[0];
  ADCSRB = adcsrb[0];
  ADCSRA = adcsra[0];

  // Enable timer1 interrupts.
  timerError = false;
  timerFlag = false;
  TCNT1 = 0;
  TIFR1 = 1 << OCF1B;
  TIMSK1 = 1 << OCIE1B;
}
//------------------------------------------------------------------------------
void adcStop() {
  TIMSK1 = 0;
  ADCSRA = 0;
}
//------------------------------------------------------------------------------
// log data
// max number of blocks to erase per erase call
uint32_t const ERASE_SIZE = 262144L;

void logData() {
  uint32_t bgnBlock, endBlock;

  // Allocate extra buffer space.
  block_t block[BUFFER_BLOCK_COUNT];

  // Initialize ADC and timer1.
  adcInit((metadata_t*) &block[0]);
//------------------------------------------------------------------------------
    DateTime now = rtc.now(); 
	binName[BASE_NAME_SIZE] = '_';
    
    binName[BASE_NAME_SIZE+1]  = (now.year()/1000)%10 + '0';
    binName[BASE_NAME_SIZE+2]  = (now.year()/100)%10 + '0'; 
    binName[BASE_NAME_SIZE+3]  = (now.year()/10)%10 + '0';
    binName[BASE_NAME_SIZE+4]  = now.year()%10 + '0';
   
    binName[BASE_NAME_SIZE+5]  = now.month()/10 + '0';
    binName[BASE_NAME_SIZE+6]  = now.month()%10 + '0';
    
    binName[BASE_NAME_SIZE+7]  = now.day()/10 + '0';
    binName[BASE_NAME_SIZE+8]  = now.day()%10 + '0';
    
    binName[BASE_NAME_SIZE+9] = '_';
   
    binName[BASE_NAME_SIZE+10] = now.hour()/10+ '0';
    binName[BASE_NAME_SIZE+11] = now.hour()%10 + '0';
    
    binName[BASE_NAME_SIZE+12] = now.minute()/10 + '0';
    binName[BASE_NAME_SIZE+13] = now.minute()%10 + '0';
    
    binName[BASE_NAME_SIZE+14] = now.second()/10 + '0';
    binName[BASE_NAME_SIZE+15] = now.second()%10 + '0';
//------------------------------------------------------------------------------
    
  // Delete old tmp file.
  if (sd.exists(TMP_FILE_NAME)) {
    if (!sd.remove(TMP_FILE_NAME))	LedError();
  }
  
  // Create new file.
  wavFile.close();
  
  //DateTime now = rtc.now(); 
  SdFile::dateTimeCallback(dateTime);
  
  if (!wavFile.createContiguous(sd.vwd(),
        TMP_FILE_NAME, 512 * FILE_BLOCK_COUNT))	LedError();
		
  // Get the address of the file on the SD.
  if (!wavFile.contiguousRange(&bgnBlock, &endBlock))	LedError();
  
  // Use SdFat's internal buffer.
  uint8_t* cache = (uint8_t*)sd.vol()->cacheClear();
  if (cache == 0)	LedError();

  // Flash erase all data in the file.
  uint32_t bgnErase = bgnBlock;
  uint32_t endErase;
  while (bgnErase < endBlock) {
    endErase = bgnErase + ERASE_SIZE;
    if (endErase > endBlock) {endErase = endBlock;}
    if (!sd.card()->erase(bgnErase, endErase))	LedError();
    bgnErase = endErase + 1;
  }
  
  // Start a multiple block write.
  if (!sd.card()->writeStart(bgnBlock, FILE_BLOCK_COUNT))	LedError();
  
  // Write metadata.
  if (!sd.card()->writeData((uint8_t*)&block[0]))	LedError();
  
  // Initialize queues.
  emptyHead = emptyTail = 0;
  fullHead = fullTail = 0;

  // Use SdFat buffer for one block.
  emptyQueue[emptyHead] = (block_t*)cache;
  emptyHead = queueNext(emptyHead);

  // Put rest of buffers in the empty queue.
  for (uint8_t i = 0; i < BUFFER_BLOCK_COUNT; i++) {
    emptyQueue[emptyHead] = &block[i];
    emptyHead = queueNext(emptyHead);
  }
  
  // Give SD time to prepare for big write.
  delay(1000);
  // Wait for Serial Idle.
  //Serial.flush();
  //delay(10);
  uint32_t bn = 1;
  uint32_t t0 = millis();
  uint32_t t1 = t0;
  uint32_t overruns = 0;
  uint32_t count = 0;
  uint32_t maxLatency = 0;

  // Start logging interrupts.
  adcStart();
  
  while (1) {
    digitalWrite(LED_Work,HIGH);
    
	if (fullHead != fullTail) {
      // Get address of block to write.
      block_t* pBlock = fullQueue[fullTail];

      // Write block to SD.
      uint32_t usec = micros();
      if (!sd.card()->writeData((uint8_t*)pBlock))	LedError();
	  
      usec = micros() - usec;
      t1 = millis();
      if (usec > maxLatency) {maxLatency = usec;}
	  
      count += pBlock->count;

      // Add overruns and possibly light LED.
      if (pBlock->overrun) {overruns += pBlock->overrun;}
		  //Aquí dentro del if puedo agregar LedError()
		  //Se reiniciaría si hay error de overrun
		
	  // Move block to empty queue.
      emptyQueue[emptyHead] = pBlock;
      emptyHead = queueNext(emptyHead);
      fullTail = queueNext(fullTail);
      bn++;
      if (bn == FILE_BLOCK_COUNT) { // File full so stop ISR calls.
        adcStop();
        break;
      }
    }
	
    if (timerError)	LedError();
//------------------------------------------------------------------------------
    //condición de parada de escritura!
	//if (!digitalRead(PinStart))  { //si PinStart=0, se detiene el programa
  DateTime now = rtc.now(); 
    if (!digitalRead(PinBreak) || (now.hour()== HoraFinal && now.minute()== MinFinal)) {
	//if ((now.hour()== HoraFinal && now.minute()== MinFinal)) {
	  // Stop ISR calls.
      adcStop();
      if (isrBuf != 0 && isrBuf->count >= PIN_COUNT) {
        // Truncate to last complete sample.
        isrBuf->count = PIN_COUNT * (isrBuf->count / PIN_COUNT);
        // Put buffer in full queue.
        fullQueue[fullHead] = isrBuf;
        fullHead = queueNext(fullHead);
        isrBuf = 0;
      }
      if (fullHead == fullTail)	break;
    }
   }

  if (!sd.card()->writeStop())	LedError();
  
  // Truncate file if recording stopped early.
  if (bn != FILE_BLOCK_COUNT) {
    if (!wavFile.truncate(512L * bn))	LedError();
  }
  
  if (!wavFile.rename(sd.vwd(), binName))	LedError();
  
  digitalWrite(LED_Error, HIGH); //se enciende el LED_Error fijo para indicar Detener
  digitalWrite(LED_Final, HIGH);
  digitalWrite(LED_Work , LOW);  //se apaga el LED_Work para indicar fin

  //para indicar si se empieza o no un nuevo fichero
  if (now.hour()== HoraFinal && now.minute()== MinFinal) {
    Siguiente = false;
    digitalWrite(LED_Error,LOW);
    digitalWrite(LED_Final,LOW);
  }
  else {Siguiente = true;} 
  
  delay(500);
  // while (!digitalRead(PinStart)) { //si vale "cero" me quedo aquí
    // SPI.end();
    // pinMode(10,INPUT); //CS
    // pinMode(11,INPUT); //MOSI
    // pinMode(12,INPUT); //MISO
    // pinMode(13,INPUT); //CLK
    // SPI.endTransaction();
    // } 
}
//------------------------------------------------------------------------------
void setup(void) {
	//wdt_disable();
	rtc.begin();
	//Serial.begin(9600);
	//Serial.println(FreeRam());
  
  if (! rtc.isrunning()) {
    // following line sets the RTC to the date & time this sketch was compiled
	  rtc.adjust(DateTime(__DATE__, __TIME__));
	  // January 21, 2014 at 3am you would call:
	  //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
	}
	
	analogRead(PIN_LIST[0]);
	
	pinMode(LED_Error, 	OUTPUT);
	pinMode(LED_Work, 	OUTPUT); 
	pinMode(LED_Final, 	OUTPUT);
	
	pinMode(PinStart, INPUT_PULLUP);
	pinMode(PinBreak, INPUT_PULLUP);
	
	// attachInterrupt(digitalPinToInterrupt(PinStart), SiguienteISR, RISING);
	// attachInterrupt(digitalPinToInterrupt(PinBreak), StopISR, CHANGE);
	
	//digitalWrite(PinStart, LOW); //garantizo un "cero" en el inicio 
                  				  //para que no inicie el sistema

	if (!sd.begin(10, SPI_FULL_SPEED)) LedError();
}

// void SiguienteISR() {
	// Siguiente=!Siguiente;
	// cli ();
	// reti();
// }

// void StopISR() {
	// Detener=!Detener;
// }	
	
//------------------------------------------------------------------------------
void loop(void) {
	DateTime now = rtc.now();
	// Serial.print(now.hour());
	// Serial.print(":");
	// Serial.print(now.minute());
	// Serial.println();
	delay(100); 

	if(!digitalRead(PinStart) || Siguiente == 1 || (now.hour()== HoraInicio && now.minute()== MinInicio)) {//
		digitalWrite(LED_Error,LOW);
		digitalWrite(LED_Final,LOW);
		logData();
	}
	else {
		digitalWrite(LED_Final,   HIGH);
		delay(1000);
		digitalWrite(LED_Final,   LOW);
	}
}
#else  // __AVR__
#error This program is only for AVR.
#endif  // __AVR__


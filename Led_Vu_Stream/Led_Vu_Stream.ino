#include "IRremote.h"
#include "module_colors.h"
#define TRUE 1;
#define FALSE 0;

//IR Remote
int receiver = 14;
IRrecv irrecv(receiver);           // create instance of 'irrecv'
decode_results results;            // create instance of 'decode_results'

//Led Stream
#include <SPI.h>

#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED_PIN  _BV(PORTB5)

static const uint8_t magic[] = {'A','d','a'};
#define MAGICSIZE  sizeof(magic)
#define HEADERSIZE (MAGICSIZE + 3)

#define MODE_HEADER 0
#define MODE_HOLD   1
#define MODE_DATA   2

uint8_t StreamLeds[] = { 0, 255, 0,     0, 255, 0,     0, 255, 0,     0, 255, 0,     0, 255, 0,     0, 255, 0,     0, 255, 0,     0, 255, 0,
												 0, 0, 0,     0, 0, 0,     0, 0, 0,     0, 0, 0,     0, 0, 0,     0, 0, 0,     0, 0, 0,
												 0, 0, 0,     0, 0, 0,     0, 0, 0,     0, 0, 0,     0, 0, 0,     0, 0, 0,     0, 0, 0,
												 0, 0, 0,     0, 0, 0,     0, 0, 0,     0, 0, 0,     0, 0, 0,     0, 0, 0,     0, 0, 0,     0, 0, 0};

static const unsigned long serialTimeout = 15000; // 15 seconds

//Led Controller by USB
uint8_t
buffer[256],
indexIn       = 0,
indexOut      = 0,
mode2          = MODE_HEADER,
hi, lo, chk, i, spiFlag;
int16_t
bytesBuffered = 0,
hold          = 0,
c;
int32_t
bytesRemaining;
unsigned long
startTime,
lastByteTime,
lastAckTime,
t;

//Pins Arduino
int audio = 5;
int button_mode = 2;
int num_leds = 9;

int rgb_red = 9;
int rgb_green = 5;
int rgb_blue = 6;

//Modo
boolean butt_pulse=0;
int mode = 1;
const int max_mod = 4;
/* Modos:
0-> None
1-> Led Stream By Serial (USB)
2-> Vu Meter + RGB + Effect Degree
3-> RGB + Efecct Degree
*/

int mode_rgb = 0;
/*Modo del rgb: 
0-> Disabled
1-> Activación por margen desde máximo local
2-> Activación entre media y maxímo volumen (75%)   (media + (vmax - media) / 2 )
*/

int mode_colors = 0; 
int mode_colors_max = 4;
/*Colours of VU meter
0->Progresive G to B
1->Constante G-Y-R
2->Constant V-B-G-Y-R-W
3->Rainbow R-G
4->Rainbow R-G-B (GAY!!!!)
*/

boolean MaxLed_on = TRUE; //Still on for a moment the led of last max.
boolean MaxLed_Degree_Vu = TRUE; //All Vu descend as do MaxLed
boolean VU_Meter_Static = FALSE; //Set 1 for turn on all leds of VU Meter

//RGB Variables

int VU_colors[3] = {0,0,0};


boolean MinToChange_On = FALSE;		//Activate if it's necesary a min. to change color or rgb
boolean already_activate_degrade = FALSE;	// If degrade with delay is already activate
boolean degree_delay_on = FALSE;	//Active delay that the rgb wait to degrade
boolean degree_on = FALSE;			//Modo degradado

//Intensidad del RGB
int vred = 0;
int vgreen = 0;
int vblue = 0;

const int MinToChange = 255; 	//Min value of all colors of RGB to can change color [Mode 3]
const int LedToOn = 5; 			//Number of leds that they are necesary to activate RGB [Mode 4]

//RGB Variables

//Valores de cambio y diferencia de lvl
int incremento = 4;	//Diferencias de lvl para subir de led
int amplitude = 2; 	//[RGB] Margen para cambiar color. Margen = incremento * amplitud;

//Maximos
int max_sound=0;	//Máximo alcanzado en unidad de tiempo
int vmax = 0;		//Audio mas alto local [Periodo corto timer_vmax_less]
int vmax_abs = 0;	//Audio mas alto cancion [Periodo largo timer_vmax_rst]
int max_led=0;  	//Led mas alto alcanzado 

//Media de audio
const int num_mediciones = 100;
int medicion_i = 0;
int mediciones[num_mediciones];
boolean mediciones_full = 0;

//Variables para temporizadores

	//Medida Audio
unsigned long time_sound;
const long timer_sound = 1;
	//Reducir maximo local
unsigned long time_vmax_less;
const long timer_vmax_less_first = 2500 ;	//Tiempo sin alcanzar el máximo para que empiece a restar
const long timer_vmax_less_cont = 1000;		//Cada cuanto tiempo se resta 1 al máximo
long timer_vmax_less = timer_vmax_less_first;
	//Reiniciar maximo local
unsigned long time_vmax_rst;
const long timer_vmax_rst = 5 * 1000;
	//Reiniciar maximo absoluto
unsigned long time_vmax_abs_rst;
const long timer_vmax_abs_rst = 10 * 1000;
	//Efecto Degradado
unsigned long time_degree;
const long timer_degree = 50;
const int value_degree = 8;
	//VU_Meter_Max
unsigned long time_max_led;
long timer_max_led = 50;
int timer_max_led_start = 100;
	//Calculo dinámico
unsigned long time_dinamic_calc;
long timer_dinamic_calc = 2500;
	//Activate Degree
unsigned long time_degree_delay;
const long timer_degree_delay = 500;


//Funciones
void print_int(int valor,String msj){
	Serial.print(msj);
	Serial.println(valor);
}

//RGB

void SetRgb(int ired, int igreen, int iblue){ //Establece leds RGB golpes de sonido

	int i;
	int div = 1;
	int leds_active = num_leds;

	if (mode==3)
		leds_active = 0;

	for (i = 0; i < 15-leds_active; i++){
		if (i) div = i * 10;
		SetStream(ired/div,igreen/div,iblue/div,14-i);
	}

	analogWrite(rgb_red, ired);
	analogWrite(rgb_green, igreen);
	analogWrite(rgb_blue, iblue);
}

boolean Rgb_Low_Values(int min){//If a necesary min to change color of leds (MinToChange_On need set 1)
	if(MinToChange_On == 0) return true;
	if(vred > min) return false;
	if(vgreen > min) return false;
	if(vblue > min) return false;

	return true;
}

void SetRgb_Max(int max_value){//Set A Rgb color by random number (0 to max_value)
	vred = random(max_value);
	vgreen = random(max_value);
	vblue = random(max_value);
	SetRgb(vred,vgreen,vblue);
}

int SetLeds(int lvl_audio, int lvl_min){ //VU Meter: Encendemos los leds segun el nivel de audio (lvl)

	int i;
	int leds_on = 0;
	int lvl_i = lvl_min;

	for(int i=0;i<num_leds;i++){ 

		ColorVU(VU_colors,mode_colors,i);

    //if(1){
		if( lvl_audio>=lvl_i || VU_Meter_Static){
			SetStream(VU_colors[0],VU_colors[1],VU_colors[2],i);
			leds_on++;
		} else{
			SetStream(0, 0, 0,i);
		}
		lvl_i+=incremento;
	}

	return leds_on;
}

void SetLeds_by_max(int max){
	int i;

	for(int i=0;i<=max;i++){ 
		ColorVU(VU_colors,mode_colors,i);
		SetStream(VU_colors[0],VU_colors[1],VU_colors[2],i);
	} 
}

void SetRgb_by_lvl(int lvl_audio){
//In function of max absolute calculate de intesity with the lvl of audio
	if ( Rgb_Low_Values(MinToChange) ){
		float max_value = (float) lvl_audio * ( ((float) vmax_abs)/255); 		if (max_value < 50.0) max_value = 50.0;
		SetRgb_Max ((int) max_value);
	}
}

//Efects
void Effect_Led_MaxToLvl(int leds_on, long time_now){ //Turn on one led that show the max of volume.
	
	if (max_led<=leds_on){
		max_led = leds_on;
		time_max_led = time_now;
		timer_max_led = timer_max_led_start;
	}

	if( (time_now <= (time_max_led+timer_max_led))){ //Hace descender el led despues de ser el máximo alcanzado hasta el lvl del volumen
		if (max_led <= 0) return;{
			SetStream(255,255,255,max_led);
			if(MaxLed_Degree_Vu) SetLeds_by_max(max_led);
		}
	} else {
		if (max_led <= 0) return;
		time_max_led=time_now;
		timer_max_led=50;
		SetStream(255,255,255,max_led);
		if(MaxLed_Degree_Vu) SetLeds_by_max(max_led);
		if (max_led > 0) max_led--;
	}
}

void Effect_Atenuado(long time_now){ //Atenua los leds de golpes de sonido
	if(time_now > (time_degree+timer_degree) ){
		if (vred >40)
			vred-=value_degree;
		else
			vred=0;
		if (vgreen >40)
			vgreen-=value_degree;
		else
			vgreen=0;
		if (vblue >40) 
			vblue-=value_degree;
		else
			vblue=0;

		SetRgb(vred,vgreen,vblue);

		time_degree = time_now;     
	}
}

void Activate_EfDegreat(long time_now){ //Retardo para empezar a atenuar los leds de golpes de sonido.
	if (already_activate_degrade == 0) {
		if (time_now > (time_degree_delay + timer_degree_delay) ){
			degree_on =  TRUE;
			already_activate_degrade = TRUE;
		}
	}
}


//Calculo dinámico de lvl
int SetMaxLocal(int lvl_audio, long time_now){
	if ( time_now > (time_vmax_less+timer_vmax_less) ){ //Reducimos el máximo local cada timer_vmax_less
		if (vmax > 0) vmax--;
		time_vmax_less=time_now;
		timer_vmax_less = timer_vmax_less_cont;

		//print_int(vmax,"Reducido a: ");

		if(time_now > (time_vmax_rst+timer_vmax_rst) ){ //Reseteamos el máxino local cada timer_vmax_rst
			vmax = 0;
			time_vmax_rst=time_now;
		}
	}
}

int SetMaxAbs(int lvl_audio, long time_now){
	if ( time_now > (time_vmax_abs_rst+timer_vmax_abs_rst) ){ //Reducimos el máximo absoluto cada timer_vmax_abs_rst

		print_int(vmax_abs,"Maximo absoluto reiniciado en: ");
		time_vmax_abs_rst=time_now;
		vmax_abs = vmax;

	}
}

void Dinamic_levels(long time_now){  //Recalculamos la diferencia de sonido para activar un led
	if (time_now > time_dinamic_calc + timer_dinamic_calc){
		incremento = vmax_abs / num_leds;
		time_dinamic_calc = time_now;
	}
}

int Media_lvl_audio(){  //Hace una media de las mediciones recogidas en un array.
	int sum = 0;
	for(int i = 0; i < num_mediciones; i++){
		sum += mediciones[i];
	}

	return sum / num_mediciones;
}

//Stream Leds Functions

void Duplicate(){ //Duplica el lado izquierdo de la malla led en el derecho
	int i;
	int j;

	for(i=0; i < 15; i++){
		for(j=0; j<3; j++){
			StreamLeds[(87 - 3*i) + j] = StreamLeds[i*3 + j];
		}
	}
}

void ResetStream(){ //Set off all leds
	int i;

	for (i=0;i<90;i++){
		StreamLeds[i] = 0;
	}

}
void SetStream(int r, int g, int b, int pos){ //Establece el color de un led
	pos = pos*3;

	if (r < 0) r = 0;
	if (g < 0) g = 0;
	if (b < 0) b = 0;

	StreamLeds[pos+0] = r;
	StreamLeds[pos+1] = g;
	StreamLeds[pos+2] = b;

}

void SendStream(){  //Envia el buffer de colores a los leds
	int i,j;

	Duplicate();

	for(j=0; j<30 ; j++){
		for(SPDR = StreamLeds[(j*3) + 1]; !(SPSR & _BV(SPIF)); );
		for(SPDR = StreamLeds[(j*3) + 2]; !(SPSR & _BV(SPIF)); );
		for(SPDR = StreamLeds[(j*3) + 0]; !(SPSR & _BV(SPIF)); );
	}
}


//IR Functions

void translateIR(){ // takes action based on IR code received
	int action = 0;
	int i;

	switch(results.value){

		case 0xFFA25D:  
		Serial.println("ON"); 
		break;

		case 0xFF629D:  
		Serial.println("Mode"); 
		break;

		case 0xFFE21D:  
		Serial.println("Mute"); 
		break;

		case 0xFF22DD:  
		Serial.println("PLAY");
		if(VU_Meter_Static)
			VU_Meter_Static--;
		else
			VU_Meter_Static++;
		break;

		case 0xFF02FD:  
		Serial.println("Pre");

		if(mode_colors)
			mode_colors--;
		else
			mode_colors = mode_colors_max;

		ResetStream();
		SetStream(0,0,255,mode_colors);
		SendStream();

		break;

		case 0xFFC23D:  
		Serial.println("Next"); 

		if(mode_colors>= mode_colors_max)
			mode_colors = 0;
		else
			mode_colors++;

		ResetStream();
		SetStream(0,0,255,mode_colors);
		SendStream();

		break;

		case 0xFFE01F:  
		Serial.println("EQ"); 
		break;

		case 0xFFA857:  
		Serial.println("VOL--");
		if (num_leds) num_leds--;
		ResetStream();
		for(i=15; i > num_leds; i--){
			SetStream(255,0,0,(i-1));
		}
		SendStream();
		break;

		case 0xFF906F:  
		Serial.println("Vol++");
		if (num_leds<15) num_leds++;
		ResetStream();
		for(i=15; i > num_leds; i--){
			SetStream(255,0,0,(i-1));
		}
		SendStream();
		break;

		case 0xFF6897:  
		Serial.println(" 0              ");
		mode = 0;
		ResetStream();
		SetStream(0,255,0,mode);
		SendStream();
		break;

		case 0xFF9867:  
		Serial.println("Change");
		SetRgb(random(255),random(255),random(255));
		break;

		case 0xFFB04F:  
		Serial.println("U/SD"); 
		if(MaxLed_Degree_Vu){
			MaxLed_Degree_Vu--;
			timer_max_led_start=250;
		}
		else {
			MaxLed_Degree_Vu++;
			timer_max_led_start=50;
		}
		break;

		case 0xFF30CF:  
		Serial.println(" 1              ");
		mode= 1;
		ResetStream();
		SetStream(0,255,0,mode);
		SendStream();
		break;

		case 0xFF18E7:  
		Serial.println(" 2              "); 
		mode = 2;
		ResetStream();
		SetStream(0,255,0,mode);
		SendStream();
		break;

		case 0xFF7A85:  
		Serial.println(" 3              ");
		mode = 3;
		ResetStream();
		SetStream(0,255,0,mode);
		SendStream();
		break;

		case 0xFF10EF:  
		Serial.println(" 4              ");
		break;

		case 0xFF38C7:  
		Serial.println(" 5              ");
		break;

		case 0xFF5AA5:  
		Serial.println(" 6              "); 
		break;

		case 0xFF42BD:  
		Serial.println(" 7              ");
		break;

		case 0xFF4AB5:  
		Serial.println(" 8              ");
		break;

		case 0xFF52AD:  
		Serial.println(" 9              "); 
		break;

		default: 
		Serial.println(" other button   ");
	}


	delay(350);
	ResetStream();
	SendStream();
}


//Main
void setup () {

  //LedStream

  LED_DDR  |=  LED_PIN; // Enable output for LED
  LED_PORT &= ~LED_PIN; // LED off

  
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV16); // 1 MHz max, else flicker
  
  //Other Params
  Serial.begin(115200); //Inicializo el puerto serial a 9600 baudios

  MaxLed_on = 1; mode_rgb = 2; degree_on = 0; MinToChange_On = 1;degree_delay_on=1;
  
  irrecv.enableIRIn();
  
  //Test GBR
  uint8_t testcolor[] = { 0, 0, 0, 255, 0, 0 };
  for(char n=3; n>=0; n--) {
  	for(c=0; c<30; c++) {
  		for(i=0; i<3; i++) {
  			for(SPDR = testcolor[n + i]; !(SPSR & _BV(SPIF)); );
  		}
  }
    delay(400); // One millisecond pause = latch
  }

  startTime    = micros();
  lastByteTime = lastAckTime = millis();
}

void loop2(){
}

void loop() {

	//SelectMode();
	if (irrecv.decode(&results)){ // have we received an IR signal?
		Serial.println(results.value, HEX); 
		translateIR();
		irrecv.resume();
	}

	if (mode > 1){
		unsigned long now = millis();

		int lvl = analogRead(audio);

		int leds_on = 0;
		int media = 0;

		if(lvl>max_sound){
			max_sound = lvl;
		}

		if(now > time_sound + timer_sound){

		//Creamos un array con las continuas mediciones
		mediciones[medicion_i] = max_sound;
		if (medicion_i < num_mediciones)
			medicion_i++;
		else{
			medicion_i=0;
			mediciones_full = 1;
			media = Media_lvl_audio();
			//print_int(media, "Nivel Medio: ");
		}


	//Si alcanzamos un máximo, lo establecemos y reiniciamos el reductor de máximo
		if(vmax <= max_sound && max_sound > 0){
			print_int(max_sound,"Nuevo max: ");
			vmax = max_sound;
			time_vmax_less = timer_vmax_less_first;
			time_vmax_rst=now;
			time_vmax_less=now;

			//Lo mismo pero con el máximo absoluto
			if(vmax_abs <= vmax && max_sound > 0){
				print_int(max_sound,"Nuevo max abs: ");
				vmax_abs =  max_sound;
				time_vmax_abs_rst = now;
			}
		}


		SetMaxLocal(max_sound,now);
		SetMaxAbs(max_sound,now);
		Dinamic_levels(now);

		
		
		if(mode!=3)
			leds_on = SetLeds(max_sound,12) - 1;

		if (MaxLed_on)
			Effect_Led_MaxToLvl(leds_on,now);

		//Activamos el RGB por golpes de sonido
		if (mode_rgb == 2){
			if ( max_sound > 5 && max_sound >= (vmax - incremento * amplitude) ){
				SetRgb_Max(100);
				if(degree_delay_on){
					degree_on = 0;
					already_activate_degrade = FALSE;
					time_degree_delay = now;
				}
			}

		}else if (mode_rgb = 1){
			if ( max_sound > 5 && 
				max_sound >= (media + (vmax - media) / 2 )  && 
				(+leds_on >= LedToOn || mode < 4) )
				SetRgb_by_lvl(max_sound);
		}

		if (degree_delay_on)
			Activate_EfDegreat(now);

		if (degree_on)
			Effect_Atenuado(now);

		SendStream();
		max_sound=0;
		time_sound = now;

		delay(1);
	}

} else if(mode==1) { //If Mode = 1, wait for stream in usb to set colors of leds

	// Implementation is a simple finite-state machine.
	// Regardless of mode, check for serial input each time:
	t = millis();
	if((bytesBuffered < 256) && ((c = Serial.read()) >= 0)) {
		buffer[indexIn++] = c;
		bytesBuffered++;
		lastByteTime = lastAckTime = t; // Reset timeout counters
	} else {
		// No data received.  If this persists, send an ACK packet
	// to host once every second to alert it to our presence.
	    	if((t - lastAckTime) > 1000) {
	        Serial.print("Ada\n"); // Send ACK string to host
	        lastAckTime = t; // Reset counter
	      }
	      // If no data received for an extended time, turn off all LEDs.
	      if((t - lastByteTime) > serialTimeout) {
	      	for(c=0; c<32767; c++) {
	      		for(SPDR=0; !(SPSR & _BV(SPIF)); );
	      	}
	        delay(1); // One millisecond pause = latch
	        lastByteTime = t; // Reset counter
	      }
	    }

	    switch(mode2) {

	    	case MODE_HEADER:

	      // In header-seeking mode.  Is there enough data to check?
	    	if(bytesBuffered >= HEADERSIZE) {
	        // Indeed.  Check for a 'magic word' match.
	    		for(i=0; (i<MAGICSIZE) && (buffer[indexOut++] == magic[i++]););
	    			if(i == MAGICSIZE) {
	          // Magic word matches.  Now how about the checksum?
	    				hi  = buffer[indexOut++];
	    				lo  = buffer[indexOut++];
	    				chk = buffer[indexOut++];
	    				if(chk == (hi ^ lo ^ 0x55)) {
	            // Checksum looks valid.  Get 16-bit LED count, add 1
	            // (# LEDs is always > 0) and multiply by 3 for R,G,B.
	    					bytesRemaining = 3L * (256L * (long)hi + (long)lo + 1L);
	    					bytesBuffered -= 3;
	            spiFlag        = 0;         // No data out yet
	            mode2           = MODE_HOLD; // Proceed to latch wait mode
	          } else {
	            // Checksum didn't match; search resumes after magic word.
	            indexOut  -= 3; // Rewind
	          }
	        } // else no header match.  Resume at first mismatched byte.
	        bytesBuffered -= i;
	      }
	      break;

	      case MODE_HOLD:

	      // Ostensibly "waiting for the latch from the prior frame
	      // to complete" mode, but may also revert to this mode when
	      // underrun prevention necessitates a delay.

	      if((micros() - startTime) < hold) break; // Still holding; keep buffering

	      // Latch/delay complete.  Advance to data-issuing mode...
	      LED_PORT &= ~LED_PIN;  // LED off
	      mode2      = MODE_DATA; // ...and fall through (no break):

	      case MODE_DATA:

	      while(spiFlag && !(SPSR & _BV(SPIF))); // Wait for prior byte
	      if(bytesRemaining > 0) {
	      	if(bytesBuffered > 0) {
	          SPDR = buffer[indexOut++];   // Issue next byte
	          bytesBuffered--;
	          bytesRemaining--;
	          spiFlag = 1;
	        }
	        // If serial buffer is threatening to underrun, start
	        // introducing progressively longer pauses to allow more
	        // data to arrive (up to a point).
	        if((bytesBuffered < 32) && (bytesRemaining > bytesBuffered)) {
	        	startTime = micros();
	        	hold      = 100 + (32 - bytesBuffered) * 10;
	        	mode2      = MODE_HOLD;
	        }
	      } else {
	        // End of data -- issue latch:
	      	startTime  = micros();
	        hold       = 1000;        // Latch duration = 1000 uS
	        LED_PORT  |= LED_PIN;     // LED on
	        mode2       = MODE_HEADER; // Begin next header search
	      }
	    } // end switch
  }
}

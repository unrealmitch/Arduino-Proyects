#include <Arduino.h>

//Modo 3 -> Efecto Arcoiris
unsigned long time_arco;
const long timer_arco = 100;
int arco_start = 0;
int arco_max = 11;
int arco_array [] = {255,0,0,
					 255,85,0,
					 255,170,0,
					 255,255,0,
					 170,255,0,
					 85,255,0,
					 0,255,0,
					 85,255,0,
					 170,255,0,
					 255,255,0,
					 255,170,0,
					 255,85,0};

int arco_array2 [] = {0,255,0,
					 128,255,0,
					 255,255,0,
					 255,128,0,
					 255,0,0,
					 255,0,128,
					 255,0,255,
					 128,0,255,
					 0,0,255,
					 0,128,255,
					 0,255,255,
					 0,255,128};
int counter_led;

void DynamicColor(int * VU_colors, int nled, int array[]){
	unsigned long now = millis();

	if (!nled){

		if( now > time_arco + timer_arco){
			time_arco = millis();
			if(arco_start >= arco_max)
				arco_start = 0;
			else
				arco_start++;
		}
		counter_led=arco_start;
	}

	VU_colors[0] = array[(counter_led) * 3];
	VU_colors[1] = array[(counter_led) * 3 + 1];
	VU_colors[2] = array[(counter_led) * 3 + 2];

	if (counter_led > 0)
		counter_led--;
	else
		counter_led=arco_max;
}
//Modos de color para leds VU_Meter
extern void ColorVU(int * VU_colors, int mode_led, int nled){

	switch (mode_led){
		case 0:
			switch (nled){
				case 0: case 1:
				VU_colors[0] = 0;
				VU_colors[1] = 255;
				VU_colors[2] = 0;
				break;

				case 2: case 3:  case 4: 
				VU_colors[0] = (nled - 1) * 85;
				VU_colors[1] = 255;
				VU_colors[2] = 0;
				break;

				case 5: case 6:  case 7: case 8:
				VU_colors[0] = 255;
				VU_colors[1] = 255 - ((nled - 4) * 63);
				VU_colors[2] = 0;
				break;

				case 9:  case 10: case 11:
				VU_colors[0] = 255;
				VU_colors[1] = 0;
				VU_colors[2] = (nled - 8) * 85;
				break;

				case 12:  case 13: case 14:
				VU_colors[0] = 255 - ((nled - 5) * 85);
				VU_colors[1] = 0;
				VU_colors[2] = 255;
				break;
			}
		break;



		case 1:

			switch (nled){
				case 0: case 1: case 2:
				VU_colors[0] = 0;
				VU_colors[1] = 255;
				VU_colors[2] = 0;
				break;

				case 3: case 4:  case 5: 
				VU_colors[0] = 255;
				VU_colors[1] = 255;
				VU_colors[2] = 0;
				break;

				case 6: case 7:  case 8:
				VU_colors[0] = 255;
				VU_colors[1] = 0;
				VU_colors[2] = 0;
				break;

				default:
				VU_colors[0] = 200;
				VU_colors[1] = 0;
				VU_colors[2] = 100;
				break;
			}
		break;


		case 2:
			switch (nled){
			case 0:
				VU_colors[0] = 200;
				VU_colors[1] = 30;
				VU_colors[2] = 200;
				break;

				case 1:
				VU_colors[0] = 0;
				VU_colors[1] = 0;
				VU_colors[2] = 255;
				break;

				case 2:
				VU_colors[0] = 0;
				VU_colors[1] = 255;
				VU_colors[2] = 255;
				break;

				case 3:
				VU_colors[0] = 0;
				VU_colors[1] = 255;
				VU_colors[2] = 0;
				break;

				case 4:
				VU_colors[0] = 255;
				VU_colors[1] = 255;
				VU_colors[2] = 0;
				break;

				case 5:
				VU_colors[0] = 255;
				VU_colors[1] = 0;
				VU_colors[2] = 0;
				break;

				case 6:
				VU_colors[0] = 255;
				VU_colors[1] = 0;
				VU_colors[2] = 255;
				break;


				default:
				VU_colors[0] = 255;
				VU_colors[1] = 255;
				VU_colors[2] = 255;
				break;
			}
		break;

		case 3:
		DynamicColor(VU_colors,nled,arco_array);
		break;

		case 4:
		DynamicColor(VU_colors,nled,arco_array2);
		break;
	}
}
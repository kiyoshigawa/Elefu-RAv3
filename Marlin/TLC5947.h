/*
This is the TLC5947 Library written by Elefu
It is written specifically for the RA board, and ignores the extra direct pin mappings.
It is also assuming you're using our TLC5947 RGB Add-On Board
*/
//include avr libs to make PORTs register
#include <avr/wdt.h>

//include arduino lib files (if using versions before Arduino 1.0, use WPhogram.h instead
#include <Arduino.h>

//allows offloading of constant variables to the flash memory
#include <avr/pgmspace.h>

//the TLC pins have been moved to the pins.h file for use with Marlin.

#define NUM_TLC5947_RGBS 8 //8 RGB LEDs per TLC5947 board
#define PWM_BITS 12 // number of bits per channel on the TLC5947
#define UPDATE_DELAY 0 //minimum number of ms between updates


//Pins A6 and A7 cannot be used, as they are analog inputs only.

//#define TRANS_ARRAY {0, 1, 2, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8} //forwards
#define TRANS_ARRAY {7, 6, 5, 4, 3, 2, 1, 0, 8, 9, 10, 11, 12, 13, 14, 15} //backwards

const int temp_array[NUM_TLCS*NUM_TLC5947_RGBS] = TRANS_ARRAY;

class TLC5947{
	private:
		int8_t clock, blank, xlat, data, clock_bit, blank_bit, xlat_bit, data_bit, num_tlcs;
		volatile uint8_t *clock_port, *blank_port, *xlat_port, *data_port;
		unsigned long last_update, current_time;
	public:
		long int r[NUM_TLCS*NUM_TLC5947_RGBS], g[NUM_TLCS*NUM_TLC5947_RGBS], b[NUM_TLCS*NUM_TLC5947_RGBS];
		TLC5947();
		void update();
		void blank_lights();
		void unblank_lights();
		void set_rgb(int, uint16_t, uint16_t, uint16_t);
		void set_all_rgbs(uint16_t, uint16_t, uint16_t);
		int translation_array[NUM_TLCS*NUM_TLC5947_RGBS];
};

//this function will send the current values in the red_array, green_array and blue_array out to the TLC5947 board(s)
//We'll be using direct bit writing on the 4 TLC pins to make everything work quickly and efficiently.
void TLC5947::update(){
	//for loop for all 8 RGB LEDs times the number of TLC5947 boards
	current_time = millis();
	if(current_time >= (last_update+UPDATE_DELAY)){
		for(int RGB_num=0; RGB_num < num_tlcs * NUM_TLC5947_RGBS; RGB_num++){
			//first you write the data bit of the most significant bit to the data pin
			//this goes Blue, Red, grene because that's how the board hardware is set up. Change the order if you want it to be different.
			for(int pwm_bit=(PWM_BITS-1); pwm_bit >= 0; pwm_bit--){//cycle through from the number of PWM bits down to 0 and write each bit
				//finally write the blue data
				bitWrite(*data_port, data_bit, (b[RGB_num] >> pwm_bit));
				//pulse the clock pin once each data bit is written
				bitSet(*clock_port, clock_bit);
				bitClear(*clock_port, clock_bit);
			}
			for(int pwm_bit=(PWM_BITS-1); pwm_bit >= 0; pwm_bit--){//cycle through from the number of PWM bits down to 0 and write each bit
				//then write the red data
				bitWrite(*data_port, data_bit, (r[RGB_num] >> pwm_bit));
				//pulse the clock pin once each data bit is written
				bitSet(*clock_port, clock_bit);
				bitClear(*clock_port, clock_bit);
			}
			for(int pwm_bit=(PWM_BITS-1); pwm_bit >= 0; pwm_bit--){//cycle through from the number of PWM bits down to 0 and write each bit
				//first write the green data
				bitWrite(*data_port, data_bit, (g[RGB_num] >> pwm_bit));
				//pulse the clock pin once each data bit is written
				bitSet(*clock_port, clock_bit);
				bitClear(*clock_port, clock_bit);
			}

		}
		//pulse the latch pin once the data bits have all been sent
		bitSet(*xlat_port, xlat_bit);
		bitClear(*xlat_port, xlat_bit);
		last_update = millis();
	}
	else{
		//do nothing
	}
}

//this makes the TLC5947 display no output until it has been unblanked by the unblank() function
void TLC5947::blank_lights(){
	bitSet(*blank_port, blank_bit);
}

//this makes the TLC5947 display the outputs found in its registers which are updated by the update() function
void TLC5947::unblank_lights(){
	bitClear(*blank_port, blank_bit);
}

//this sets up the TLC5947 object and sets the data ports to correspond to the appropriate registers on the 328p
//be sure to use the digital pin numbers only, not analog pin numbers
TLC5947::TLC5947(){
	//set the object variables to match the init variables
	clock = TLC_CLOCK_PIN;
	blank = TLC_BLANK_PIN;
	xlat = TLC_XLAT_PIN;
	data = TLC_DATA_PIN;
	num_tlcs = NUM_TLCS;
	//these four groups are setting the ports for clock, blank, xlat and data based on 328 port and bit numbers
	
	//initializes the red_array, green_array and blue_array variables based on num_tlcs
	for(int i=0; i<num_tlcs*8; i++){ r[i] = 0; g[i] = 0; b[i] = 0;}
	clock_port = TLC_CLOCK_PORT;
	clock_bit = TLC_CLOCK_BIT;
	
	blank_port = TLC_BLANK_PORT;
	blank_bit = TLC_BLANK_BIT;
	
	xlat_port = TLC_XLAT_PORT;
	xlat_bit = TLC_XLAT_BIT;
	
	data_port = TLC_DATA_PORT;
	data_bit = TLC_DATA_BIT;
	
	last_update = 0;
	
	//configure a default translation array for use with rearranging LEDs to fit actual builds.
	for(int i=0; i<num_tlcs*NUM_TLC5947_RGBS; i++){
		//default is a 1:1 translation, where the first LED is the first LED, 2nd is 2nd, etc.
		translation_array[i] = temp_array[i];
	}
}

//this sets an RGB in the TLC5947 arrays to be a certain color
void TLC5947::set_rgb(int index, uint16_t r_in, uint16_t g_in, uint16_t b_in){
	r[translation_array[index]] = r_in;
	g[translation_array[index]] = g_in;
	b[translation_array[index]] = b_in;
}

//this sets ALL rgbs in the TLC5947 arrays to be a certain color
void TLC5947::set_all_rgbs(uint16_t r_in, uint16_t g_in, uint16_t b_in){
	for(int i=0; i<num_tlcs*NUM_TLC5947_RGBS; i++){
		set_rgb(i, r_in, g_in, b_in);
	}
}

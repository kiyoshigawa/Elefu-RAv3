//this class controls animations using the colorfade class, keeping track of all RGB LED states
//it is designed to update variables that are compatible with the elefu TLC5947.h libraries and colorfade.h libraries.
//be sure to include these libraries before including this one or it will not compile properly

#include "TLC5947.h"
#include "colorfade.h"

#define MAX_NUM_COLORS 32//maximum number of colors that can be stepped through. Reduce this to reduce program size as needed.

#define MAX_NUM_FADERS 8//maximum number of colors that can be simultaneously displayed in any given animation. This is mostly limited due to SRAM available on the arduino, so lower it until it runs properly.

//animator types: add more as you add animation control functions to match them 
#define FADER 0
#define CHASER 1
#define RANDOM 2
#define RANDOM_FADER 3
#define RANDOM_CHASER 4//not implemented yet
#define STOP 254

//defines for default animation values in the absense of actual animations
#define DEFAULT_TYPE RANDOM_FADER
#define DEFAULT_HOLD 0
#define DEFAULT_DELAY 500
#define DEFAULT_POSITION 0
#define DEFAULT_COLOR color(0,0,0)
#define BLANK color(0,0,0);

class animator{
	private:
		int type; //the type of animation
		unsigned long delay, hold; //the delay time and hold time for animation steps
		color colors[MAX_NUM_COLORS]; //the colors the animation steps through
		int num_colors; //the length of the colors array
		bool has_changed;//indicated if the color has changed since last animator init/update functions were called
		bool is_fading; //used to tell if any of the colors in the animation are currently fading or if a new fade should begin
		uint8_t position; //this is the position in the colors array that is the current color
		colorfade faders[MAX_NUM_FADERS]; //the size of this depends on the animation type, so it is made in the animation's init files
		void update_position(int);//this cycles the position variable from 0 up to the max as defined in the argument and back to 0
		
		//below here are functions that control animations depending on the animation type. each one will animate in a different way
		void fader_init();
		void fader_control();
		void chaser_init();
		void chaser_control();
		void random_init();
		void random_control();
		void random_fader_init();
		void random_fader_control();
		
	public:
		TLC5947 tlc; //this is the TLC that controls the animations
		animator();
		void change();
		animator(int, unsigned long, unsigned long);
		void change(int, unsigned long, unsigned long);
		animator(int, unsigned long, unsigned long, const color[], int);
		void change(int, unsigned long, unsigned long, const color[], int);
		bool update();
		void init();
		
};

//this function will run the animations ad defined by the type, delay, hold, colors, and num_colors variables per the animation's initialization
//it will return true if any color in the r_array, g_array and b_arrays have changed while it runs, otherwise it will return false
bool animator::update(){
	if(type == FADER){
		if(is_fading){//if a fade is occuring, run the control function
			fader_control();
		}
		else{//if fading has stopped, run the init to start new fades as needed
			fader_init();
		}
	}
	if(type == CHASER){
		if(is_fading){//if a fade is occuring, run the control function
			chaser_control();
		}
		else{//if fading has stopped, run the init to start new fades as needed
			chaser_init();
		}
	}
	if(type == RANDOM){
		if(is_fading){//if a fade is occuring, run the control function
			random_control();
		}
		else{//if fading has stopped, run the init to start new fades as needed
			random_init();
		}
	}	
	if(type == RANDOM_FADER){
		if(is_fading){//if a fade is occuring, run the control function
			random_fader_control();
		}
		else{//if fading has stopped, run the init to start new fades as needed
			random_fader_init();
		}
	}
	if(type == STOP){
		tlc.set_all_rgbs(0, 0, 0);//turn them all off
	}
	//update the TLC5947 only if the color has changed
	if(has_changed){
		tlc.update();
	}
}

/************************************************************************************************/
//Below here are functions for the animations themselves, both init functions and control functions
//the init functions set up the colors array to have current faders for all channels in the animation
//the control functions update the r_array, g_array and b_array variables to match the faders per the animation

//The Fader type of animation fades all the RGB LEDs to the colors specified in the colors[] array one by one
void animator::fader_init(){
	//start a new fader to control the color change
	update_position(num_colors-1);
	faders[0].fade(colors[position], delay, hold);
	has_changed = faders[0].update();
	is_fading = faders[0].fading;
}
//this updates the fader to the current fading color transition levels
void animator::fader_control(){
	//update the current fader and set the TLC to match the fader's current color
	has_changed = faders[0].update();
	tlc.set_all_rgbs(faders[0].current.colors[RED], faders[0].current.colors[GREEN], faders[0].current.colors[BLUE]);
	is_fading = faders[0].fading;
}

//The Chaser type of animation will start with the first color in the colors[] array and fade it into RGB 1
//it will then fade the first color into RGB 2, and fade the second color in colors[] into RGB1
//after this it will continue to shift the existing colors down into the LEDs and loops back to the first
void animator::chaser_init(){
	has_changed = false;
	is_fading = false;
	//situation 1: num_colors == MAX_NUM_FADERS
	if(num_colors == MAX_NUM_FADERS){
		//can use either num_colors-1 or MAX_NUM_FADERS-1 as upper limit on position
		update_position(num_colors-1);
		//easiest case, just map colors to faders directly with position as offset
		for(int i=0; i<MAX_NUM_FADERS; i++){
			if(position-i >= 0){//when the position-i is positive, the fader at position-i is the color at i
				faders[position-i].fade(colors[i], delay, hold);
			}
			else{
				faders[MAX_NUM_FADERS+(position-i)].fade(colors[i], delay, hold);
			}
			has_changed = has_changed || faders[i].update();//if any fader has changed, update them all
			is_fading = is_fading || faders[i].fading;//if any fader has changed, update them all
		}
	}

	//situation 2: num_colors > MAX_NUM_FADERS
	if(num_colors > MAX_NUM_FADERS){
		//use num_colors as upper limit on position
		update_position(num_colors-1);
		//map the positionth color and the MAX_NUM_FADERS colors after it (looping back to 1 if needed) as the faders and apply them
		for(int i=0; i<MAX_NUM_FADERS; i++){
			//start at positionth color as fader 0, then go to positionth-1 color for fader 1, and on and on
			if(position-i >= 0){
				faders[i].fade(colors[position-i], delay, hold); 
			}
			else{
				faders[i].fade(colors[num_colors+(position-i)], delay, hold);
			}
			has_changed = has_changed || faders[i].update();//if any fader has changed, update them all
			is_fading = is_fading || faders[i].fading;//if any fader has changed, update them all
		}
	}
	//Situation 3: num_colors < MAX_NUM_FADERS
	if(num_colors < MAX_NUM_FADERS){
		//use MAX_NUM_FADERS-1 as upper limit on position
		update_position(MAX_NUM_FADERS-1);
		//send black to extra faders until they run out
		color black = BLANK;
		//map the positionth color until you run out of colors then make the faders go black
		for(int i=0; i<MAX_NUM_FADERS; i++){
			if(position-i >= 0){//when the position-i is positive, the fader at position-i is the color at i
				if(i <= num_colors){
					faders[position-i].fade(colors[i], delay, hold);
				}
				else{
					faders[position-i].fade(black, delay, hold);
				}
			}
			else{
				if(i <= num_colors){
					faders[MAX_NUM_FADERS+(position-i)].fade(colors[i], delay, hold);
				}
				else{
					faders[MAX_NUM_FADERS+(position-i)].fade(black, delay, hold);
				}
			}
			has_changed = has_changed || faders[i].update();//if any fader has changed, update them all
			is_fading = is_fading || faders[i].fading;//if any fader has changed, update them all
		}
	}
}
//this updates all the chaser channels and sends sends the faders to the correct RGBs based on position
void animator::chaser_control(){
	//update the current faders and set the TLC to match the faders' current colors
	has_changed = false;
	is_fading = false;
	for(int i=0; i<NUM_TLCS*NUM_TLC5947_RGBS; i++){
		has_changed = faders[i % MAX_NUM_FADERS].update() || has_changed;//if any fader has changed, update them all
		tlc.set_rgb(i, faders[i % MAX_NUM_FADERS].current.colors[RED], faders[i % MAX_NUM_FADERS].current.colors[GREEN], faders[i % MAX_NUM_FADERS].current.colors[BLUE]);
		is_fading = is_fading || faders[i % MAX_NUM_FADERS].fading;//if any fader has changed, update them all
	}
}

//The Random type of animation makes a random color for every RGB on the TLCs and fades to a new random color every cycle
void animator::random_init(){
	//start MAX_FADERS new faders to control the color changes
	has_changed = false;
	is_fading = false;
	for(int i=0; i<MAX_NUM_FADERS; i++){
		colors[i].randomizer();
		faders[i].fade(colors[i], delay, hold);
		has_changed = has_changed || faders[i].update();//if any fader has changed, update them all
		is_fading = is_fading || faders[i].fading;//if any fader has changed, update them all
	}
}
//the random control updates all the fader chanels and sends them to the RGBs looping over again when it runs out of channels and has more LEDS to fill
void animator::random_control(){
	//set states to default values and let function below fix them as needed
	has_changed = false;
	is_fading = false;
	position = 0;
	//update the current faders and set the TLC to match the faders' current colors
	for(int i=0; i<NUM_TLCS*NUM_TLC5947_RGBS; i++){
		has_changed = faders[position].update() || has_changed;//if any fader has changed, update them all
		tlc.set_rgb(i, faders[position].current.colors[RED], faders[position].current.colors[GREEN], faders[position].current.colors[BLUE]);
		is_fading = is_fading || faders[position].fading;//if any fader has changed, update them all
		update_position(MAX_NUM_FADERS-1);
	}
}

//The Random Fader type of animation makes a random color for All RGBs and fades them all to this color
void animator::random_fader_init(){
	//start a new fader to control the color change
	colors[0].randomizer();
	faders[0].fade(colors[0], delay, hold);
	has_changed = faders[0].update();
	is_fading = faders[0].fading;
}
//updates all RGBs to the current state of the fading from random colors
void animator::random_fader_control(){
	//update the current fader and set the TLC to match the fader's current color
	has_changed = faders[0].update();
	tlc.set_all_rgbs(faders[0].current.colors[RED], faders[0].current.colors[GREEN], faders[0].current.colors[BLUE]);
	is_fading = faders[0].fading;
}


/************************************************************************************************/

//call this function in the arduino setup function to initialize the animation
void animator::init(){

	//declare pins as inpus/outputs
	pinMode(TLC_CLOCK_PIN, OUTPUT);
	pinMode(TLC_BLANK_PIN, OUTPUT);
	pinMode(TLC_XLAT_PIN, OUTPUT);
	pinMode(TLC_DATA_PIN, OUTPUT);
	
	//blank the TLC chip until the initial colors are set
	tlc.blank_lights();
	
	//set initial colors for all LEDs to be off
	tlc.set_all_rgbs(colors[0].colors[RED], colors[0].colors[GREEN], colors[0].colors[BLUE]);
	
	//update the TLC to send the color data to the registers
	tlc.update();
	
	//unblank the TLC to display the colors in the registers
	tlc.unblank_lights();
}

//this increments the position variable until it is at the max value and then sets it to 0
void animator::update_position(int max){
	if(position < max){
		position++;
	}
	else{
		position = 0;
	}
}

/****************************************************************************************/
//Below here are the change and constructor functions that define the type and duration of animations


//this is the default constructor. 
//It will create an animation of the default type with default settings
animator::animator(){
	change();
}

// this constructor is for types that are randomly generated to determine their colors
animator::animator(int type_input, unsigned long delay_input, unsigned long hold_input){
	change(type_input, delay_input, hold_input);
}

//this constructor is for types that are not randomly generated and use external color arrays to determine their colors
animator::animator(int type_input, unsigned long delay_input, unsigned long hold_input, const color colors_input[MAX_NUM_COLORS], int num_colors_input){
	change(type_input, delay_input, hold_input, colors_input, num_colors_input);
}

//these functions below allow you to change an animator object to a different animation easily, and are called when the animator is initialized
//they can also be called at any point inside a program to change the animation to another
void animator::change(){
	//make a new TLC5947 object to control the TLC5947 boards.
	tlc = TLC5947();
	//set the object type variable that control the animations
	type = DEFAULT_TYPE;
	//set delay and hold times to match setup
	delay = DEFAULT_DELAY;
	hold = DEFAULT_HOLD;
	num_colors = MAX_NUM_COLORS; //the length of the colors array
	for(int i=0; i<num_colors; i++){
		colors[i] = DEFAULT_COLOR; //the colors the animation steps through
	}
	has_changed = true;;//indicated if the color has changed since last animator init/update functions were called
	for(int i=0; i<MAX_NUM_FADERS; i++){
		faders[i] = colorfade(DEFAULT_COLOR); //the colors the animation steps through
	}
	is_fading = false;; //used to tell if any of the colors in the animation are currently fading or if a new fade should begin
	position = DEFAULT_POSITION; //this is the position in the colors array that is the current color
}

void animator::change(int type_input, unsigned long delay_input, unsigned long hold_input){
	//set the object type variable that control the animations
	type = type_input;
	//make a new TLC5947 object to control the TLC5947 boards.
	tlc = TLC5947();
	//set delay and hold times to match setup
	delay = delay_input;
	hold = hold_input;
	num_colors = MAX_NUM_COLORS; //the length of the colors array
	for(int i=0; i<num_colors; i++){
		colors[i] = DEFAULT_COLOR; //the colors the animation steps through
	}
	has_changed = true;;//indicated if the color has changed since last animator init/update functions were called
	for(int i=0; i<MAX_NUM_FADERS; i++){
		faders[i] = colorfade(DEFAULT_COLOR); //the colors the animation steps through
	}
	is_fading = false;; //used to tell if any of the colors in the animation are currently fading or if a new fade should begin
	position = DEFAULT_POSITION; //this is the position in the colors array that is the current color
}
void animator::change(int type_input, unsigned long delay_input, unsigned long hold_input, const color colors_input[MAX_NUM_COLORS], int num_colors_input){
		//make a new TLC5947 object to control the TLC5947 boards.
	tlc = TLC5947();
	//set the object type variable that control the animations
	type = type_input;
	//set delay and hold times to match setup
	delay = delay_input;
	hold = hold_input;
	num_colors = num_colors_input; //the length of the colors array
	for(int i=0; i<num_colors; i++){
		colors[i] = colors_input[i]; //the colors the animation steps through
	}
	has_changed = true;;//indicated if the color has changed since last animator init/update functions were called
	for(int i=0; i<MAX_NUM_FADERS; i++){
		faders[i] = colorfade(DEFAULT_COLOR); //the colors the animation steps through
	}
	is_fading = false;; //used to tell if any of the colors in the animation are currently fading or if a new fade should begin
	position = DEFAULT_POSITION; //this is the position in the colors array that is the current color
}

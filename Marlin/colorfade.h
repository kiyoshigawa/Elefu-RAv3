//this colorfade.h file allows you to take an RGB color and fade it over time, then hold it for a certain time after
//it is designed to work with the TLC5947 libraries from elefu.com
//be sure to include this librariy before including colorfade.h one or it may not compile properly

#define MIN_FADE 10//to prevent division by 0 on the slope
#define MIN_HOLD 1//to prevent division by 0 on the slope

//define color position on color object arrays
#define RED 0
#define GREEN 1
#define BLUE 2

#define MAX_COLORS 3 //the maximum number of colors in a color object

//color object to hold color values
//it can hold as many different color channels as you like, though it is currently configured for 3-channel RGB LEDs
class color{
	private:
		//none
	public:
		long int colors[MAX_COLORS]; //this is the array of values to make the color
		color() { colors[RED] = 0; colors[GREEN] = 0; colors[BLUE] = 0; };
		color(long int, long int, long int);
		void change(long int, long int, long int);
		void randomizer();
};

//this will control fading of a color from one color to another over tiem with a hold time on the new color. For every simultaneous color fade, you will need one colorfade object.
//it will work with any color object and fade from the current color to a desired color over any length of time
class colorfade{
	private:
		unsigned long end_time, end_hold_time, last_update_time;//time variables for calculations
		color final;//the final color the fader is heading for
		float slope[MAX_COLORS];//the variable for determining change in color over time
		bool updated;//bools that update is a color has changed during an update
	public:
		colorfade() { fading = false; };
		colorfade(color current_init) { current = current_init; fading = false; };
		void fade(color, unsigned long, unsigned long);
		bool update();
		color current;//the current state of the fading color
		bool fading;//state variable to tell if the color is changing or not
};

//start with a color, fade to another and keep the current color up to date according to timing guidelines above
void colorfade::fade(color final_color, unsigned long fade_time, unsigned long hold_time){
	//when initializing the fade, set your start times, end times, and hold times, as well as the initial color to your current variable, then set slopes for each color.
	fading = true;//start fading sequence
	unsigned long start_time = millis();
	end_time = start_time + fade_time+MIN_FADE;
	end_hold_time = end_time + hold_time+MIN_HOLD;
	color init = current;
	final = final_color;
	current = current;
	for(int i=0; i<MAX_COLORS; i++){
		slope[i] = (float)((long)final.colors[i]-(long)init.colors[i])/(float)((unsigned long)end_time-(unsigned long)start_time);
	}
	last_update_time = start_time;
}

//this checks to see what time section the fade is in and updated current accordingly
bool colorfade::update(){
	if(fading){
		//first check to see which time zone we're in.
		unsigned long time = millis();
		if(time < end_time){//fading zone
			unsigned long x = (time-last_update_time);
			//check slope of light change over time, add slope times the time since the fade began to the initial channel color, and update the current color to be correct based on this.
			for(int i=0; i<MAX_COLORS; i++){
				int new_color = current.colors[i] + slope[i]*x;
				if(current.colors[i] != new_color){
					current.colors[i] = new_color;//set it to final if it's hold time
					updated = true;
				}
			}
			last_update_time = time; //update the last update time to 
		}
		else if (time < end_hold_time){//hold zone
			for(int i=0; i<MAX_COLORS; i++){
				if(current.colors[i] != final.colors[i]){
					current.colors[i] = final.colors[i];//set it to final if it's hold time
					updated = false;
				}
			}
			current = final;//hold final color
		}
		else if (time >= end_hold_time){//time is outside fading zones, start a new fade
			fading = false;
		}
	}
	//check for out of bounds values before submitting to prevent sudden change from off to full on
	for(int i=0; i<MAX_COLORS; i++){
		if(current.colors[i] > 4095){
			current.colors[i] = 4095;
		}
		if(current.colors[i] < 0){
			current.colors[i] = 0;
		}
	}
	if(updated){
		return true;
	}
	else{
		return false;
	}
}

//initializes an RGB color object with colors as shown
color::color(long int r, long int g, long int b){
	colors[RED] = r;
	colors[GREEN] = g;
	colors[BLUE] = b;
}
//changes an RGB color object to different colors
void color::change(long int r, long int g, long int b){
	colors[RED] = r;
	colors[GREEN] = g;
	colors[BLUE] = b;
}
//makes a color object store a random color
void color::randomizer(){
	colors[RED] = random(0, 4095);
	colors[GREEN] = random(0, 4095);
	colors[BLUE] = random(0, 4095);
}
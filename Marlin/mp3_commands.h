//This file has an object that controls the mp3 player from Elefu.com
//it uses the wire.h library to communicate over TWI.

//include important libraries:
#include "Wire.h"
#include <Arduino.h>

//Defines for default options
#define DEFAULT_TRACK STARTUP_TRACK //must be between 0 and 255
#define DEFAULT_VOLUME 0 //must be between 0 and 255, 0 being loudest
#define DEFAULT_REPEAT false //toggles individual track repeat
#define MP3_ADDRESS 11 //the mp3 module board's default address
#define MP3_STARTUP_DELAY 1000 //give the mp3 board time in ms to start up before sending configuration commands
#define CHECK_IS_PLAYING_INTERVAL 100 //how often between updating the is_playing variable


//track numbers for functions:
#define NO_SOUND 1000 //set tracks below to NO_SOUND is you want them not to play

#define STARTUP_TRACK 1
#define PRINT_STARTED_TRACK 2
#define PRCT10_TRACK 3
#define PRCT20_TRACK 4
#define PRCT30_TRACK 5
#define PRCT40_TRACK 6
#define PRCT50_TRACK 7
#define PRCT60_TRACK 8
#define PRCT70_TRACK 9
#define PRCT80_TRACK 10
#define PRCT90_TRACK 11
#define PRINT_COMPLETE_TRACK 12
#define ERROR_TRACK 13

#define SILLY_TRACK 253 //for use with being silly


//generic mp3 settings:
#define TRANSMISSION_DELAY 1 //the delay between sending a TWI command and asking for a response and checking to see if the response is in.

#define MP3_STARTUP_DELAY 1000 //delay for the mp3 to finish starting and be ready to receive commands

//Define mp3 commands:
//these are variables to be sent over TWI representing typical values that can't be used due to characters 0x00 and 0xFF being reserved for end transmission and no data received respectively
#define TWI_TRUE 'T'
#define TWI_FALSE 'F'
#define TWI_BLANK '_'

//#defines that control the size of commands and other constants regardless of command.
#define TWI_COMMAND_LENGTH 8 //8 bytes per TWO command
//Slaves will echo back only the command as it was received upon receipt of a command. if the command does not require a response
//commands that require responses have the last byte set to TWI_TRUE so slaves will know to send a different response then the original command
//note that the master never echos received responses back to the slaves, it just accepts their data and parses it accordingly

//The #defines at the top level are all command types. Below them are sub-commands and data bytes for the specific command

#define CANCEL_COMMAND 'X'
	//this is to cancel long TWI operations at will
	//example command: {CANCEL_COMMAND, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE}
#define UNKNOWN_COMMAND '?'
	//this is sent as a response if the command is not known to the device who received it.
	//example response: {UNKNOWN_COMMAND, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE}
#define MP3_COMMAND 'B'
	//These are mp3 actions for byte 1 of MP3_COMMAND TWI messagescontrolling the mp3 module remotely with TWI
	#define TWI_RWND 'A' //rewind
		//All data bytes 2-6 are Null for this action
		//example command: {MP3_COMMAND, RWND, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE}
	#define TWI_PREV 'B' //Previous track
		//All data bytes 2-6 are Null for this action
		//example command: {MP3_COMMAND, PREV, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE}
	#define TWI_PLAY_PAUSE 'C' //Play/Pause
		//All data bytes 2-6 are Null for this action
		//example command: {MP3_COMMAND, PLAY_PAUSE, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE}
	#define TWI_STOP 'D' //Stop
		//All data bytes 2-6 are Null for this action
		//example command: {MP3_COMMAND, STOP, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE}
	#define TWI_NEXT 'E' //next Track
		//All data bytes 2-6 are Null for this action
		//example command: {MP3_COMMAND, NEXT, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE}
	#define TWI_FFWD 'F' //Fast Forward
		//All data bytes 2-6 are Null for this action
		//example command: {MP3_COMMAND, FFWD, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE}
	#define TWI_VOL_UP 'G' //Volume Up
		//All data bytes 2-6 are Null for this action
		//example command: {MP3_COMMAND, VOL_UP, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE}
	#define TWI_VOL_DN 'H' //Volume Down
		//All data bytes 2-6 are Null for this action
		//example command: {MP3_COMMAND, VOL_DN, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE}
	#define TWI_PLAY_TRACK 'I' //Skip to a specific track
		//data bytes 2-6 are the track number in ascii numbers.
		//example command: play track 172: {MP3_COMMAND, TWI_PLAY_TRACK, '0', '0', '1', '7', '2', TWI_FALSE}
	#define TWI_IS_PLAYING 'J' //check to see if a track is currently playing
		//all daya bytes are null for this command
		//example command: {MP3_COMMAND, IS_PLAYING, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_TRUE}
		//command expects a boolean response on byte 2 of response to master
		//example response: something is playing: {MP3_COMMAND, IS_PLAYING, true, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE}
	#define TWI_SET_VOL 'K' //Volume Down
		//All data bytes 2-6 are are a specific volume level to set the mp3 to
		//example command: Set volume to -15db: {MP3_COMMAND, VOL_DN, '0', '0', '0', '1', '5' TWI_FALSE}
		
//mp3 object class: This class sends the TWI commands to the mp3 and has several functions for ease of use.
	//put a Wire.begin() in the init function of the mp3 class.
	

	
class mp3{
	private:
		//Variables:
		//these are commands for the TWI connection to the mp3.

			char is_playing[TWI_COMMAND_LENGTH];		
			char last_TWI_response[TWI_COMMAND_LENGTH];//variable for storing responses from the mp3
		
		//these are state variables for various mp3 functions
			bool is_playing_state;
			bool repeat_track; //toggles individual track repeat
			int current_track; //sets default track as current track
			unsigned long last_request_time; //the last time a response was requested
			unsigned long last_play_check_time; //the last time a response was requested
			bool awaiting_response; //if the response has been requested but not received, this will be true
			
		//private functions called by the class:
		void send_command(char* command, int length);//this sends one of the above commands to the mp3 board over TWI
		
	public:
		
		mp3(); //constructor to initialize the mp3 and its variables. Run in setup
		
		void init();
		void update();
		
		void cmd_set_vol(int vol);//sets the volume from 0 to 255. 0db is highest, and larger numbers represent negative db from 0, for instance set_vol(40) sets the volume to -40db
		void cmd_play_track(int track);//plays a specific track from 1-255. Send it NO_SOUND to play nothing and stop all sounds currently playing		
		void cmd_prev();
		void cmd_play_pause();
		void cmd_stop();
		void cmd_next();
		void cmd_vol_up();
		void cmd_vol_dn();
		bool cmd_is_playing();
		bool playing(); //returns true if it is playing, false if it is not.
		bool repeat(); //returns true if the player is set to repeat one track, false if it is set to stop playing after one track
		void toggle_repeat(); //returns true if the player is set to repeat one track, false if it is set to stop playing after one track
};
//functions to do things:

//this sets the volume to a specific number
void mp3::cmd_play_track(int track){
	char temp_cmd[TWI_COMMAND_LENGTH];
	if(track != NO_SOUND){
		if(track < 255 && track > 0){
			sprintf(temp_cmd, "%c%c%5d%c", MP3_COMMAND, TWI_PLAY_TRACK, track, TWI_FALSE);//set to specified volume
		}
		else{
			sprintf(temp_cmd, "%c%c%5d%c", MP3_COMMAND, TWI_PLAY_TRACK, DEFAULT_TRACK, TWI_FALSE);//set to default because specified volume is out of range
		}
		send_command(temp_cmd, TWI_COMMAND_LENGTH);
	}
	else{
		cmd_stop();
	}
}

//this sets the volume to a specific number
void mp3::cmd_set_vol(int vol){
	char temp_cmd[TWI_COMMAND_LENGTH];
	if(vol < 255 && vol >= 0){
		sprintf(temp_cmd, "%c%c%5d%c", MP3_COMMAND, TWI_SET_VOL, vol, TWI_FALSE);//set to specified volume
	}
	else{
		sprintf(temp_cmd, "%c%c%5d%c", MP3_COMMAND, TWI_SET_VOL, DEFAULT_VOLUME, TWI_FALSE);//set to default because specified volume is out of range
	}
	send_command(temp_cmd, TWI_COMMAND_LENGTH);
}

void mp3::cmd_prev(){
	char prev[TWI_COMMAND_LENGTH];
	sprintf(prev, "%c%c%c%c%c%c%c%c", MP3_COMMAND, TWI_PREV, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE);//previous track command
	send_command(prev, TWI_COMMAND_LENGTH);
}

void mp3::cmd_play_pause(){
	char play_pause[TWI_COMMAND_LENGTH];
	sprintf(play_pause, "%c%c%c%c%c%c%c%c", MP3_COMMAND, TWI_PLAY_PAUSE, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE);//play/pause command
	send_command(play_pause, TWI_COMMAND_LENGTH);
}

void mp3::cmd_stop(){
	char stop[TWI_COMMAND_LENGTH];
	sprintf(stop, "%c%c%c%c%c%c%c%c", MP3_COMMAND, TWI_STOP, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE);//stop command
	send_command(stop, TWI_COMMAND_LENGTH);
}

void mp3::cmd_next(){
	char next[TWI_COMMAND_LENGTH];
	sprintf(next, "%c%c%c%c%c%c%c%c", MP3_COMMAND, TWI_NEXT, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE);//next track command
	send_command(next, TWI_COMMAND_LENGTH);
}

void mp3::cmd_vol_up(){
	char vol_up[TWI_COMMAND_LENGTH];
	sprintf(vol_up, "%c%c%c%c%c%c%c%c", MP3_COMMAND, TWI_VOL_UP, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE);//turn volume up by default amount (3db)
	send_command(vol_up, TWI_COMMAND_LENGTH);
}

void mp3::cmd_vol_dn(){
	char vol_dn[TWI_COMMAND_LENGTH];
	sprintf(vol_dn, "%c%c%c%c%c%c%c%c", MP3_COMMAND, TWI_VOL_DN, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_FALSE);//turn volume down by default amount (3db)
	send_command(vol_dn, TWI_COMMAND_LENGTH);
}


bool mp3::playing(){
	return is_playing_state;
}

bool mp3::repeat(){
	return repeat_track;
}

void mp3::toggle_repeat(){
	repeat_track = !repeat_track;
}

//this sets default values for all the important functions
mp3::mp3(){
	//these are commands for the TWI connection to the mp3.
	sprintf(is_playing, "%c%c%c%c%c%c%c%c", MP3_COMMAND, TWI_IS_PLAYING, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_TRUE);//this one asks if a song is currently playing and the mp3 sends back a response
	
	sprintf(last_TWI_response, "%c%c%c%c%c%c%c%c", MP3_COMMAND, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK, TWI_BLANK);//variable for storing responses from the mp3
	
	repeat_track = DEFAULT_REPEAT; //toggles individual track repeat
	current_track = DEFAULT_TRACK; //sets default track as current track
	is_playing_state = false;//assumes nothing is playing. Run the is_playing() public function to update
	last_request_time = 0;
	last_play_check_time = 0;
	awaiting_response = false;
}

void mp3::init(){
	//delay for a time so the mp3 can startup
	delay(MP3_STARTUP_DELAY);
	cmd_set_vol(DEFAULT_VOLUME);
	cmd_play_track(STARTUP_TRACK);
}

//this updated the mp3 variables and checks for responses as needed
void mp3::update(){
		unsigned long current_time = millis();
		
		//if you've been waiting for a response and enough time has passed
		if(awaiting_response && ((last_request_time + TRANSMISSION_DELAY) < current_time)){
			//Serial.println(F("mp3 Echo:"));//debug serial
			int i = 0;
			while(Wire.available()){//read the returned data, be it a direct echo or a data response and save it to last_TWI_response for use in other functions
				char c = Wire.read();
				last_TWI_response[i] = c;
				i++;
			}
			//Serial.println(last_TWI_response);//debug serial
			awaiting_response == false;
			is_playing_state = last_TWI_response[2];
		}
		
		//if enough time has passed since the last check of is_playing, check again
		if((last_play_check_time + CHECK_IS_PLAYING_INTERVAL) < current_time){
			last_play_check_time = current_time;
		}
		
		//if is_playing is false and the player is set to repeat, then restart the current track
		if(!is_playing && repeat_track){
			cmd_play_pause();
		}
}

//this sends the command string to the TWI and stores the response in the last_TWI_response
void mp3::send_command(char* command, int length){
	if(!awaiting_response){
		int null_char = 0;
		Wire.beginTransmission(MP3_ADDRESS);//send the command indicated
		for(int i=0; i<TWI_COMMAND_LENGTH; i++){
			Wire.write(command+i);
		}
		Wire.endTransmission();
		if(command[TWI_COMMAND_LENGTH-1] == TWI_TRUE){
			Wire.requestFrom(MP3_ADDRESS, TWI_COMMAND_LENGTH);//receive echo or return data
			awaiting_response = true; //set flag to hold new commands until it gets a response
			last_request_time = millis();
		}
	}
	else{
		//sends fail if it happens while the program is still awaiting a response from the previous command
	}
}
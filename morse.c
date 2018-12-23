// ------------------------------------------------------------------------------
// Copyright 2018 Stephen Stebbing. telecnatron.com
//
//    Licensed under the Telecnatron License, Version 1.0 (the “License”);
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        https://telecnatron.com/software/licenses/
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an “AS IS” BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
// ------------------------------------------------------------------------------#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "morse.h"
#include "stdlib.h"
#include <ctype.h>

// structure representing a morse character
typedef struct {
    char ch; // ascii representation of character 
    unsigned char len; // total number of dits and dahs in character. 1-8
    unsigned char code; // binary of morse. 0 is dit, 1 is dah. msb first. eg 'C' 0b10100000
} morse_char_t;

// forward function declerations
const morse_char_t* morse_lookup_char(char c);
void morse_send_bit(morse_state_t* morse_state);
void morse_start_next_char(morse_state_t* morse_state);
char morse_get_current_char(morse_state_t* morse_state);

// States
#define MORSE_STATE_IDLE         0
// sending dot
#define MORSE_STATE_DOT          1
// sending dash
#define MORSE_STATE_DASH         2
// inter-character gap
#define MORSE_STATE_ICHAR_GAP    3
// gap between letters
#define MORSE_STATE_LETTER_GAP   4
// gap between words
#define MORSE_STATE_WORD_GAP     5

// Morse character element periods in terms of dot period
// Dash is 3 dots in period
#define MORSE_PERIOD_DASH 3
#define MORSE_PERIOD_LETTER_GAP 3
#define MORSE_PERIOD_WORD_GAP 7

// Definintions of where the string being sent is being retrieved from.
// As used by morse_state_t.text_location.
#define MORSE_LOCATION_RAM       0
#define MORSE_LOCATION_PROGMEM   1
#define MORSE_LOCATION_EEPROM    2

// Globals
// Morse table - contains dot and dash info for each sendable character
static const morse_char_t morse_table[] PROGMEM = {
    {'A',2,0b01000000},
    {'B',4,0b10000000},
    {'C',4,0b10100000},
    {'D',3,0b10000000},
    {'E',1,0b00000000},
    {'F',4,0b00100000},
    {'G',3,0b11000000},
    {'H',4,0b00000000},
    {'I',2,0b00000000},
    {'J',4,0b01110000},
    {'K',3,0b10100000},
    {'L',4,0b01000000},
    {'M',2,0b11000000},
    {'N',2,0b10000000},
    {'O',3,0b11100000},
    {'P',4,0b01100000},
    {'Q',4,0b11010000},
    {'R',3,0b01000000},
    {'S',3,0b00000000},
    {'T',1,0b10000000},
    {'U',3,0b00100000},
    {'V',4,0b00010000},
    {'W',3,0b01100000},
    {'X',4,0b10010000},
    {'Y',4,0b10110000},
    {'Z',4,0b11000000},
    {'0',5,0b11111000},
    {'1',5,0b01111000},
    {'2',5,0b00111000},
    {'3',5,0b00011000},
    {'4',5,0b00001000},
    {'5',5,0b00000000},
    {'6',5,0b10000000},
    {'7',5,0b11000000},
    {'8',5,0b11100000},
    {'9',5,0b11110000},
    {'.',6,0b01010100},
    {',',6,0b11001100},
    {'?',6,0b00110000},
    {'\'',6,0b01111000},
    {'!',6,0b10101100},
    {'/',5,0b10010000},
    {'(',5,0b10110000},
    {')',5,0b10110100},
    {'&',5,0b01000000},
    {':',6,0b11100000},
    {';',6,0b10101000},
    {'=',5,0b10001000},
    {'+',5,0b01010000},
    {'-',6,0b10000100},
    {'_',6,0b00110100},
    {'"',6,0b01001000},
    {'$',7,0b00100100},
    {'@',6,0b01101000},
    {'\x0',0,0} // EOF marker - this must always be the last element in this array
};


void morse_update(morse_state_t* morse_state)
{
    if (morse_state->state == MORSE_STATE_IDLE)
	return; // nothing to be done.

    // count ticks
    morse_state->dot_tick_count++;

    if( (morse_state->state==MORSE_STATE_DOT)  || 
	(morse_state->state==MORSE_STATE_DASH && morse_state->dot_tick_count==MORSE_PERIOD_DASH) ) {
	// dot or dash has been sent
	morse_state->morse_keyer(0); // key up
	//
	if(--morse_state->ddr){
	    // more to send - shift next bit to front
	    morse_state->code =morse_state->code << 1;
	    morse_state->state=MORSE_STATE_ICHAR_GAP;
	}else{
	    // char has been sent - move on to next
	    morse_state->dot_tick_count=0;
	    ++morse_state->text;
	    char ch=morse_get_current_char(morse_state);
	    if( ch == ' ' || ch == '\x0'){
		// end of word
		if( ch == ' ')
		    ++morse_state->text; // skip space
		morse_state->state=MORSE_STATE_WORD_GAP;
	    }else{
		// next character
		morse_state->state=MORSE_STATE_LETTER_GAP;
	    }
	}
    }else if(morse_state->state==MORSE_STATE_ICHAR_GAP){
	// inter character gap has elapsed
	morse_send_bit(morse_state);
    }else if(morse_state->state==MORSE_STATE_LETTER_GAP && morse_state->dot_tick_count == MORSE_PERIOD_LETTER_GAP){
	// letter gap has elapsed
	morse_start_next_char(morse_state);
    }else if(morse_state->state==MORSE_STATE_WORD_GAP && morse_state->dot_tick_count == MORSE_PERIOD_WORD_GAP){
	// word gap has elapsed
	if( morse_get_current_char(morse_state)=='\x0'){
	    // done - nothing left to send
	    morse_state->state=MORSE_STATE_IDLE;
	}else{
	    morse_start_next_char(morse_state);
	}
    }
}

void morse_start_next_char(morse_state_t* morse_state)
{
    morse_state->dot_tick_count=0;
    const morse_char_t* mc=morse_lookup_char(morse_get_current_char(morse_state)); // note mc points to program mem
    if(mc){
	// character was found in table.
	// get number of dit and dah elements
	morse_state->ddr = pgm_read_byte( &(mc->len));
	morse_state->code =  pgm_read_byte( &(mc->code));
	morse_send_bit(morse_state);
    }else{
	// invalid character - do nothing, ie ignore 
    }
}

//! Returns the character current being, or about to be sent.
//! This is read from either RAM, PROGMEM, or EEPROM depending of setting of morse_state->text_location
char morse_get_current_char(morse_state_t* morse_state)
{
    if(morse_state->text_location==MORSE_LOCATION_PROGMEM){
	return pgm_read_byte(morse_state->text);
    }else if(morse_state->text_location==MORSE_LOCATION_EEPROM){
	return (char)eeprom_read_byte((uint8_t *)morse_state->text);
    }else{
	// ram
	return *(morse_state->text);
    }
}

//! Start the next dot or dash of character currently being sent.
void morse_send_bit(morse_state_t* morse_state)
{
    morse_state->morse_keyer(1); // key down
    morse_state->dot_tick_count=0;
    if(morse_state->code & 0x80){ // MSB is the bit to be sent
	// dash
	morse_state->state=MORSE_STATE_DASH;
    }else{
	// dot
	morse_state->state=MORSE_STATE_DOT;
    }
}

//! Lookup up passed character in the morse_table array and return a pointer
//! to the corresponding morse_char_t element. Return NULL if not found.
const morse_char_t* morse_lookup_char(char c)
{
    unsigned char i=0;
    c=toupper(c); // always compare on uppercase
    while(1){
	char ch = pgm_read_byte(&(morse_table[i].ch));
	if(ch==c){
	    // found it
	    return  &(morse_table[i]);
	}
	if(ch=='\x0')
	    break;
	i++;
    }
    // not found
    return NULL;
}

void morse_send_string(morse_state_t* morse_state,char* text)
{
    morse_state->text_location = MORSE_LOCATION_RAM;
    morse_state->text=text;
    morse_start_next_char(morse_state);
}

void morse_send_string_P(morse_state_t* morse_state, const char* text)
{
    morse_state->text_location = MORSE_LOCATION_PROGMEM;
    morse_state->text=(char *) text;
    morse_start_next_char(morse_state);
}

void morse_send_string_E(morse_state_t* morse_state,char* text)
{
    morse_state->text_location = MORSE_LOCATION_EEPROM;
    morse_state->text=text;
    morse_start_next_char(morse_state);
}

unsigned char morse_is_idle(morse_state_t* morse_state)
{
    return morse_state->state == MORSE_STATE_IDLE;
}

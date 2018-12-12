// -----------------------------------------------------------------------------
// Copyright Stephen Stebbing 2015.
// $Id: main.c 198 2015-01-30 02:50:54Z steves $
//
// Demo of morse code library on atmega8 processor.
// Sends morse code message by flashing a LED
//
// -----------------------------------------------------------------------------
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "morse.h"


// words per minute to use
#define WPM 10
// number of seconds to wait before sending the message again
#define MSG_GAP_SEC 4

// pin that led is attached to.
// Here D4, pin 6 on dip package
#define LED_PORT PORTD
#define LED_PIN  PIN4
#define LED_DDR  DDRD

// Globals
// elapsed milliseconds: this is incremented every 1ms by timer ISR
volatile unsigned long ms_ticks;
// variable to hold morse state information
morse_state_t morse_state;
// morse text that gets sent
const char morse_text_P[] PROGMEM ="CQ CQ DE VK2AAV";
                
// Forward declarations
void key_down(uint8_t down);
void init_timer();

// Convienence macros
// enable TIMER2 compare-match interrupt
#define  T2_CMINT_E()  TIMSK |= _BV(OCIE2)
// disable TIMER2 compare-match interrupt
#define  T2_CMINT_D()  TIMSK &=~ _BV(OCIE2)
// set ms_ticks global variable to zero safely, ie with timer interrupt disabled.
#define MS_TICKS_RESET() T2_CMINT_D(); ms_ticks=0; T2_CMINT_E()
// LED control - note: active low
#define LED_ON()  LED_PORT &=~ _BV(LED_PIN)
#define	LED_OFF() LED_PORT |= _BV(LED_PIN)


int main(void)
{

    //  init LED pin
    LED_DDR |= _BV(LED_PIN); // set as output
    LED_OFF();  // turn it off

    // init the keyer function
    MORSE_INIT_KEYER(&morse_state, key_down);

    // init timer for 1ms interrupt
    init_timer();

    // enable interrupts
    sei();

    // start morse message
    morse_send_string_P(&morse_state, morse_text_P);

    // main loop
    for(;;){

	// do the morse processing here
	if(morse_is_idle(&morse_state)){
       	    // message is not being sent
	    if(ms_ticks >= MSG_GAP_SEC * 1000 ){
		// it's time to start sending the message again.
		morse_send_string_P(&morse_state, morse_text_P);
		// reset timer to zero
		MS_TICKS_RESET();
	    }
	}else{
	    // message is being sent.
	    if(ms_ticks >= MORSE_WPM_DOT_MS(WPM)){
		// dot time has expired, it's time to call the state machine again
		morse_update(&morse_state);
		// reset timer to zero
		MS_TICKS_RESET();
	    }
	}

	// XXX other processing goes here
    }
    
}

// TIMER2 compare-match interrupt handler.
// This is called every 1ms
ISR(TIMER2_COMP_vect)
{
    ms_ticks++;
}

// init TIMER2 for 1ms interrupt using 16MHz cpu clock
void init_timer()
{
    // config CTC mode
    TCCR2 |= _BV(WGM21);
    // select clock source and prescaller: divide by 64. Ticks at 250kHz, 4us
    TCCR2 |= _BV(CS22);
    // set terminal count: 250 x 4us = 1ms
    OCR2 = 250;
    // enable compare-match interrupt
    T2_CMINT_E();
}

// Function used by morse library to implement morse-key up/down
// Turn LED on when down, off otherwise.
void key_down(uint8_t down)
{
    if(down){
	LED_ON();
    }else{
	LED_OFF();
    }
}




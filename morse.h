#ifndef MORSE_H
#define MORSE_H
// -----------------------------------------------------------------------------
// Copyright Stephen Stebbing. 2015.
// $Id: morse.h 198 2015-01-30 02:50:54Z steves $
// -----------------------------------------------------------------------------
// Structures
// structure containing state information
typedef struct {
    // Pointer to the string that is currently being sent - this increments as each character is sent.
    char* text;
    // Current state
    unsigned char state;
    // number of dits and dahs in character that remain to be sent
    unsigned char ddr;
    // the binary representation of the dots and dashes in character being sent. - ie the morse_char_t struct's 'code' member
    unsigned char code;
    // dot tick counter
    unsigned char dot_tick_count;
    // where text is located. This is set to one of the MORSE_LOCATION_XXX defines
    unsigned char text_location;
    // pointer to the keyer function
    void (*morse_keyer)(unsigned char down);
}morse_state_t;


// Macros
//! Set the keyer function. msp is pointer to morse_state_t, fn is pointer to the keyer function:
//!  void keyer_fn(unsigned char down)
#define MORSE_INIT_KEYER(msp, fn) (msp)->morse_keyer=fn
//! Calculate the dot time in ms for the passed words-per-minute rate
#define MORSE_WPM_DOT_MS(wpm) 1200/wpm

// function declarations
//! This should be called periodically at the dit_ms interval
void morse_update(morse_state_t* morse_state);

//! Send the passed string which is contained in RAM
void morse_send_string(morse_state_t* morse_state,char* text);

//! Send the passed string which is contained in PROGMEM
void morse_send_string_P(morse_state_t* morse_state, const char* text);

//! Send the passed string which is contained in EEPROM
void morse_send_string_E(morse_state_t* morse_state,char* text);

//! Return false if a string is in the process of being sent, true otherwise
unsigned char morse_is_idle(morse_state_t* morse_state);

#endif

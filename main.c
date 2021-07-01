/************** ECE2049 LAB 3 ******************/
/**************  14 May 2020   ******************/
/*************by Megan Aloise************/
/***************************************************/

#include <msp430.h>

/* Peripherals.c and .h are where the functions that implement
 * the LEDs and keypad, etc are. It is often useful to organize
 * your code by putting like functions together in files.
 * You include the header associated with that file(s)
 * into the main file of your project. */
#include "peripherals.h"

// Function Prototypes
void swDelay(char numLoops);
void configUserLED(char inbits);
void configTimer();
void setSong();
void updateCurr();
void playNote();

typedef enum // state machine for generate state of music box
{
    WELCOME, SONGLIST, COUNTDOWN, PLAYINGSONG, DONOTHING1,
} MusicState;

typedef enum // state machine for state of countdown
{
    DONOTHING, PRINT1, PRINT2, PRINT3, PRINTGO, UPDATE,
} CountState;

typedef enum // state machine for playing a song
{
    STARTBUZZER, DURATION, DONOTHING2,
} NoteState;

MusicState state = WELCOME;
CountState stateC = DONOTHING;
NoteState stateN = DONOTHING2;

// setting constants that help with execution

unsigned int song = 0;
bool countdown_complete = false;
unsigned char currKey = 0;
unsigned int current_note = 0;
volatile unsigned long timer = 0;
unsigned int song_length = 0;
unsigned long curr_time = 0;
bool execute_once = true;
unsigned int current_length;
unsigned int temp_note = 0;
volatile unsigned int k = 30;

unsigned int toxic_notes[] = { 523, 587, 622, 587, 523, 0, 523, 622, 523, 622, // pitch frequencies to be played by buzzer
                               0, 523, 622, 587, 523, 0, 523, 523, 523, 622,
                               523, 622, 0, 466, 587, 523, 466, 0, 784, 494,
                               784, 494, 0, 784, 880, 0, 932, 1397, 880, 784, 0,
                               784, 784, 784, 880, 880, 880, 932, 1397, 880,
                               784, 0, 698, 784, 932, 784, 698 };
unsigned int toxic_duration[] = { 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 4, 1, 1, 1, 1, // ratio of notes in comparison to the shortest note used in song
                                  2, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 2, 2, 1,
                                  1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1,
                                  1, 2, 1, 1, 2, 2, 2, 2, 8, 2, 1, 1, 1, 1, 2 };
unsigned int happy_birthday_notes[] = { 349, 349, 392, 329, 494, 440, 349, 349, // a very out of tune rendition of happy birthday
                                        392, 349, 523, 494, 349, 349, 659,
                                        587, 494, 466, 392, 659, 659, 587,
                                        494, 523, 494 };
unsigned int happy_birthday_duration[] =
        { 3, 1, 4, 4, 4, 8, 3, 1, 4, 4, 4, 8, 3, 1, 4, 4, 4, 4, 4, 3, 1, 4, 4,
          4, 8 };
// Main
void main(void)

{

    WDTCTL = WDTPW | WDTHOLD; // turning off watchdog timer b/c it's bad for this

    // Useful code starts here
    initLeds();
    configDisplay();
    configKeypad();
    configTimer();
    BuzzerOn(); // using this to configure buzzer, does not play a pitch
    _BIS_SR(GIE); // global interrupts enabled

    // *** Intro Screen ***
    Graphics_clearDisplay(&g_sContext); // Clear the display

    // Draw a box around everything because it looks nice
    Graphics_Rectangle box = { .xMin = 3, .xMax = 125, .yMin = 3, .yMax = 125 };
    Graphics_drawRectangle(&g_sContext, &box);

    while (1)    // Forever loop
    {
        currKey = getKey(); // checking keypad input for functionalities of music box below switch statements

        switch (state)
        {
        case WELCOME:
            // reset constants that may be altered before returning to WELCOME for proper functionality
            current_note = 0;
            song = 0;
            execute_once = true;
            countdown_complete = false;
                
            // printing welcome screen
                
            Graphics_drawStringCentered(&g_sContext, "HELLO",
            AUTO_STRING_LENGTH,
                                        64, 40,
                                        TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "WELCOME TO MY",
            AUTO_STRING_LENGTH,
                                        64, 50,
                                        TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "MUSIC BOX",
            AUTO_STRING_LENGTH,
                                        64, 60,
                                        TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "SELECT",
            AUTO_STRING_LENGTH,
                                        64, 80,
                                        TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "(*)",
            AUTO_STRING_LENGTH,
                                        64, 90,
                                        TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "TO BEGIN",
            AUTO_STRING_LENGTH,
                                        64, 100,
                                        TRANSPARENT_TEXT);

            Graphics_flushBuffer(&g_sContext); // so we can actually see the text

            currKey = getKey(); // just in case
            if (currKey == '*')
            {
                state = SONGLIST;
                P1OUT |= BIT0;      // Set the P1.0 as 1 (High), LED on
                Graphics_clearDisplay(&g_sContext); // clear before next state
            }
            break;

        case SONGLIST:

            Graphics_drawStringCentered(&g_sContext, "SELECT KEYS TO PLAY",
            AUTO_STRING_LENGTH,
                                        64, 40,
                                        TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "(1) TOXIC",
            AUTO_STRING_LENGTH,
                                        64, 50,
                                        TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "by BRITTANY SPEARS",
            AUTO_STRING_LENGTH,
                                        64, 60,
                                        TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "(2) HAPPY BIRTHDAY",
            AUTO_STRING_LENGTH,
                                        64, 80,
                                        TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "by PATTY HILL",
            AUTO_STRING_LENGTH,
                                        64, 90,
                                        TRANSPARENT_TEXT);

            Graphics_flushBuffer(&g_sContext);
            currKey = getKey();
            if (currKey == '1' || currKey == '2') // if either of two valid buttons pressed
            {
                setSong();
                Graphics_clearDisplay(&g_sContext);
            }

            break;

        case COUNTDOWN:
//            if (execute_once)
//            {
            updateCurr(); // keeps record of current time
            state = DONOTHING1; // handing off to another state machine for countdown
            stateC = PRINT1;
//            }
            break;
        case PLAYINGSONG:
            if (current_note + 1 < song_length) // ensures song stops playing when end of song is reached
            {
                stateN = STARTBUZZER; // shifting control to other state machine
                state = DONOTHING1;
            }
            else
            {
                state = WELCOME; // return to WELCOME when song complete
            }
            break;
        case DONOTHING1:
            break;

        }

        switch (stateC)
        {
        case DONOTHING:
            break;
        case PRINT1:
            Graphics_drawStringCentered(&g_sContext, "3",
            AUTO_STRING_LENGTH,
                                        64, 60,
                                        TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);
            P4OUT |= BIT7; // flashing respective LEDs for countdown aesthetic
            if (curr_time + 100 < timer) // rough estimate of passage of time
            {
                P4OUT &= ~BIT7; // have to turn off LEDs before next state for blinking in-time effect
                Graphics_clearDisplay(&g_sContext);
                stateC = PRINT2;
                curr_time = timer; // reset global timekeeping variable
            }
            break;
        case PRINT2:
            Graphics_drawStringCentered(&g_sContext, "2",
            AUTO_STRING_LENGTH,
                                        64, 60,
                                        TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);
            P1OUT |= BIT1; // flashing respective LEDs for countdown aesthetic
            if (curr_time + 100 < timer)
            {
                P1OUT &= ~BIT0;
                Graphics_clearDisplay(&g_sContext);
                stateC = PRINT3;
                curr_time = timer;
            }
            break;
        case PRINT3:
            Graphics_drawStringCentered(&g_sContext, "1",
            AUTO_STRING_LENGTH,
                                        64, 60,
                                        TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);
            P4OUT |= BIT7; // flashing respective LEDs for countdown aesthetic
            if (curr_time + 100 < timer)
            {
                P4OUT &= ~BIT7;
                Graphics_clearDisplay(&g_sContext);
                curr_time = timer;
                stateC = PRINTGO;
            }
            break;
        case PRINTGO:
            Graphics_drawStringCentered(&g_sContext, "GO!",
            AUTO_STRING_LENGTH,
                                        64, 60,
                                        TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);
            P4OUT |= BIT7; // flashing respective LEDs for countdown aesthetic
            P1OUT |= BIT0;
            if (curr_time + 200 < timer)
            {
                P4OUT &= ~BIT7;
                P1OUT &= ~BIT0;
                Graphics_clearDisplay(&g_sContext);
                stateC = UPDATE;
            }
            break;
        case UPDATE:

            stateC = DONOTHING; // shifting control back to original state machine 
            state = PLAYINGSONG;

            break;
        }

        switch (stateN)
        {
        case DONOTHING2:
            break;
        case STARTBUZZER:
            // change
            if (song == 1)
            {
                temp_note = *(toxic_notes + current_note); // iterating through array for frequency of current note
                current_length = *(toxic_duration + current_note); // iterating through array for duration of current note
                updateCurr(); // update global timekeeping variable
                stateN = DURATION;

            }
            else if (song == 2)
            {
                temp_note = *(happy_birthday_notes + current_note);
                current_length = *(happy_birthday_duration + current_note);
                updateCurr();
                stateN = DURATION;
            }
            break;
        case DURATION:
            if ((curr_time + (current_length*k)) < timer)  // multiplying duration of note by some value k, k can be adjusted to alter speed of song
            {   // ensures note plays to fuull duration and then stops
                BuzzerOff();
                current_note++;
                state = PLAYINGSONG; // shifting back control to original state machine -- loops until song is finished
                stateN = DONOTHING2;
            }
            else
            {
                setBuzzer(temp_note);
            }
            break;
        }

        if (currKey == '#') // at any point, '#' key returns user to WELCOME state/screen
        {
            state = WELCOME;
            stateN = DONOTHING2;
            stateC = DONOTHING;
            BuzzerOff();
            P4OUT |= BIT7;      // Set the P4.7 as 1 (High)
        }

        if (currKey == '8'){
            state = DONOTHING1; // at any point of play, '8' key pauses song play
            stateC = DONOTHING;
            stateN = DONOTHING2;
            P4OUT &= ~BIT7;
            P1OUT |= BIT0;
            BuzzerOff();
        }
        if (currKey == '9'){ // at any point of play, '9' key resumes a song from paused 
            state = PLAYINGSONG;
            P1OUT &= ~BIT0;
            P4OUT |= BIT7;
        }

        if(currKey == '3'){ // the following keys change the value that determines speed of song at any point in play
            k = 20; // fastest setting
        }
        if(currKey == '4'){
            k = 25;
        }
        if(currKey == '5'){
            k = 30; // default setting
        }
        if(currKey == '6'){
            k = 35;
        }
        if(currKey == '7'){
            k = 40; // slowest setting
        }

    }  // end while (1)
}

void swDelay(char numLoops) // unused because of blocking
{

    volatile unsigned int i, j;

    for (j = 0; j < numLoops; j++)
    {
        i = 50000;					// SW Delay
        while (i > 0)
            i--;
    }
}

void configUserLED(char inbits)
{  
    P4SEL &= ~BIT7; // configure P4.7 (LED2) for Digital I/O
    P4DIR |= BIT7; // As output
    P4OUT |= inbits & BIT7; 

    P1SEL &= ~BIT0; // configure P1.0 (LED1) for Digital I/O
    P1DIR |= BIT0;
    P1OUT |= (inbits & BIT0) << 7;
}

void setSong()
{
    currKey = getKey(); // just in case
    if (currKey == 0x31) // reading in using hex value of ASCII
    {
        // setting constants based on input from keypad (song choice)
        song = 1;
        song_length = 57;
        state = COUNTDOWN;
    }
    else if (currKey == 0x32)
    {
        song = 2;
        song_length = 25;
        state = COUNTDOWN;
    }
    else
    {
        Graphics_clearDisplay(&g_sContext);
        Graphics_drawStringCentered(&g_sContext, "PLEASE SELECT A VALID KEY",
        AUTO_STRING_LENGTH,
                                    64, 60,
                                    TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
        state = SONGLIST;
    }
}
void updateCurr() // I dont really need this upon reflection
{
    curr_time = timer;
}

void configTimer()
{
    TA2CTL = TASSEL_1 + ID_0 + MC_1; //ACLK, DIV = 0, Up-Mode
    TA2CCR0 = 327; // every 0.01 second, timer interrupt
    TA2CCTL0 = CCIE;
}

#pragma vector = TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void)
{
    timer++;
}


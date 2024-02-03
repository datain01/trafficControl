#include <stdint.h>
#include "SysTick.h"
#include "msp432p401r.h"

#define NORTH_SENSOR (P5->IN & 0x02) // P5.1: North sensor 0000 0010
#define EAST_SENSOR  (P5->IN & 0x01) // P5.0: East sensor 0000 0001
#define WALK_SENSOR (!(P1->IN & 0x02)) //P1.1: Walk sensor 0000 0010

const struct State {
    uint32_t Out;
    uint32_t Time;
    uint32_t Next[7];
};
typedef const struct State STyp;

#define goN     0
#define waitN   1
#define goE     2
#define waitE   3
#define walkWait 4
#define walkGo   5
#define hurryUp  6

STyp FSM[7] = {
    {0x21, 300, {goN, waitN, goN, waitN}}, //goN
    {0x22, 50, {goE, goE, goE, goE}}, //waitN
    {0x0C, 300, {goE, goE, waitE, waitE}}, //goE
    {0x14, 50, {goN, goN, goN, goN}}, //waitE
    {0x24, 200, {walkGo, walkGo, walkGo, walkGo}}, //  All car lights red, 2s
    {0x24, 500, {hurryUp, hurryUp, hurryUp, hurryUp}}, // walkGo All car lights red, 5s
    {0x24, 50, {hurryUp, goN, hurryUp, goN}} // hurryUp All car lights red, 5s
};

void main(void){
    uint32_t ind;
    uint32_t Input;
    uint32_t walkInput;
    uint32_t blinkCounter = 0;


    ind = goN; // Initial state

    SysTick_Init();

    P4SEL0 &= ~0x3F; // Set P4.5-P4.0 as output 0011 1111
    P4SEL1 &= ~0x3F;
    P4DIR |= 0x3F;

    P5SEL0 &= ~0x03; // Set P5.1-P5.0 as input 0000 0000
    P5SEL1 &= ~0x03;
    P5DIR &= ~0x03;

    P1SEL0 &= ~0x02; // Set P1.1 as input 0000 0010 Walk Sensor
    P1SEL1 &= ~0x02;
    P1DIR &= ~0x02;
    P1->REN |= 0x02;  // Enable resistor for P1.1
    P1->OUT |= 0x02;  // Set resistor to pull-up for P1.1


    P2SEL0 &= ~0x03; // Set P2.1-P2.0 as output 0000 0011 RG
    P2SEL1 &= ~0X03;
    P2DIR |= 0X03;

    while (1) {
        P4->OUT = FSM[ind].Out; // Set lights
        SysTick_Wait10ms(FSM[ind].Time); // Wait for a time duration
        Input = (P5->IN & 0x03); // Get sensor input

        // Check for walk sensor input
        walkInput = WALK_SENSOR;


        if(walkInput && ind != walkWait && ind != walkGo && ind != hurryUp) {
            ind = walkWait; // Transition to walkWait state if walk sensor is pressed
        } else {
            if(ind != hurryUp) { // Prevent state change during hurryUp blinking
                ind = FSM[ind].Next[Input]; // Go to next state
            }
        }

        // Handle walk light
        if(ind == walkGo) {
            P2->OUT &= ~0x01;
            P2->OUT |= 0x02; // Turn on P2.1 (green walk light)
        } else if(ind == hurryUp) {
            blinkCounter++;
            P2->OUT &= ~0x01;
            if(blinkCounter <= 10) { // Blink for 10 times (0.5s on, 0.5s off)
                P2->OUT ^= 0x02; // Toggle P2.1
            } else {
                blinkCounter = 0; // Reset counter
                P2->OUT &= ~0x02;
                P2->OUT |= 0x01;
                // Check the EAST_SENSOR after the blinking is finished
                            if (EAST_SENSOR) {
                    ind = goE;
                } else {
                    ind = goN;
                }
            }
        } else {
            P2->OUT &= ~0x02; // Turn off P2.1
            P2->OUT |= 0x01; // Turn on P2.0 (red walk light)
        }
    }

}

#include <stdint.h>
#include "SysTick.h"
#include "msp432p401r.h"

#define NORTH_SENSOR (P5->IN & 0x02) // P5.1: North sensor 0000 0010
#define EAST_SENSOR  (P5->IN & 0x01) // P5.0: East sensor 0000 0001

const struct State {
    uint32_t Out;
    uint32_t Time;
    uint32_t Next[4];
};
typedef const struct State STyp;

#define goN     0
#define waitN   1
#define goE     2
#define waitE   3

STyp FSM[4] = {
    {0x21, 300, {goN, waitN, goN, waitN}}, //goN RYGRYG
    {0x22, 50, {goE, goE, goE, goE}}, //waitN
    {0x0C, 300, {goE, goE, waitE, waitE}}, //goE
    {0x14, 50, {goN, goN, goN, goN}} //waitE
};

void main(void){
    uint32_t ind;
    uint32_t Input;

    ind = goN; // Initial state

    SysTick_Init();
    P4SEL0 &= ~0x3F; // Set P4.5-P4.0 as output 0011 1111
    P4SEL1 &= ~0x3F;
    P4DIR |= 0x3F;

    P5SEL0 &= ~0x03; //P5->DIR = 0x00; // Set P5.1-P5.0 as input 0000 0000
    P5SEL1 &= ~0x03;
    P5DIR &= ~0x03;

    while (1) {
            P4->OUT = FSM[ind].Out; // Set lights
            SysTick_Wait10ms(FSM[ind].Time); // Wait for a time duration
            Input = (P5->IN & 0x03); // Get sensor input
            ind = FSM[ind].Next[Input]; // Go to next state
        }
}

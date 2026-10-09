/*
 * Lab06.c
 *
 * Created: 10/4/2022 9:16:57 AM
 * Initial Coder: jfhutton
 * Current Coder: Jake Matthews
 * Modified:      10/6/2026
 *
 * This lab uses hardware LEDs wired to PORTA, an interrupt from the joystick center
 * button, and a timer interrupt to build a game loop program that can display 
 * different patterns on the LEDs.
 *
 * While there are many "down and dirty" ways to get this coding done, try to 
 * remember your coding and data structures classes.  Things like ENUM, Arrays, Functions
 * could help make for more elegant coding.
 *
 */ 

#include <avr/io.h>         // Needed for AVR IO defines
#include <avr/interrupt.h>  // Needed for AVR interupt devines

#define LEDS            PORTA    // alias PORTA

// Four LED patterns required by the lab
typedef enum {
    MODE_LOW_TO_HIGH = 0,    // A0 to A7
    MODE_HIGH_TO_LOW = 1,    // A7 to A0
    MODE_BACK_AND_FORTH = 2, // A0 to A7 to A0
    MODE_CUSTOM = 3          // Bounce off middle
} PatternMode;

// Global variables 
const unsigned char TCNT0_COUNT_SET = 0x8E; // Count for 1ms loop 

volatile unsigned int Tick = 0;               // Incremented every 1ms in Timer0 ISR
volatile PatternMode Mode = MODE_LOW_TO_HIGH; // Current pattern mode
volatile unsigned char modeChanged = 0;        // Flag set when joystick button updates Mode

// Function defaults for pattern execution
void update_low_to_high(void);
void update_high_to_low(void);
void update_back_and_forth(void);
void update_custom_pattern(void);

int main(void){
    // State machine initialization
    Mode = MODE_LOW_TO_HIGH;
    Tick = 0;
    modeChanged = 0;
    
    // Initialization for LEDs
    DDRA = 0xFF;  // Set the Direction for all PORTA pins to be outputs
    LEDS = 0xFF;  // Set PORTA pins to high 
    
    // Initialization for Timer Interrupt
    TCCR0 = (1 << CS02);
    TCNT0 = TCNT0_COUNT_SET;
    TIMSK |= (1 << TOIE0); // Enable Timer0 overflow interrupt
    
    // Port Initialization for Joystick Center Switch
    // Configure INT0 pin (PD0) as input with pull-up resistor enabled
    DDRD &= ~(1 << DDD0);   // PD0 as input
    PORTD |= (1 << PORTD0); // Enable internal pull-up on PD0
    
    // Interrupt Enable Block
    EICRA = (1 << ISC01);
    EIMSK = (1 << INT0);    // Enable External Interrupt 0
    
    // Enable Global Interrupts
    sei();
    

    while (1) {
        // Reset state if joystick pressed to clear output immediately
        if (modeChanged) {
            modeChanged = 0;
            Tick = 0;
            LEDS = 0xFF; // Turn off all LEDs when switching modes
        }

        // Wait for Tick timer to reach 50 ms
        if (Tick >= 50) {
            Tick = 0; // Reset tick counter

            // Select active LED pattern
            switch (Mode) {
                case MODE_LOW_TO_HIGH:
                    update_low_to_high();
                    break;
                case MODE_HIGH_TO_LOW:
                    update_high_to_low();
                    break;
                case MODE_BACK_AND_FORTH:
                    update_back_and_forth();
                    break;
                case MODE_CUSTOM:
                    update_custom_pattern();
                    break;
                default:
                    Mode = MODE_LOW_TO_HIGH;
                    break;
            }
        }
    }
}

// ISR routine for Timer0 Overflow (fires every 1 ms)
ISR(TIMER0_OVF_vect) {
    TCNT0 = TCNT0_COUNT_SET; // Reload initial count value to maintain 1ms period
    Tick++;                  // Increment tick counter
}

// ISR routine for Hardware Pin Interrupt 0 (PD0 Center Joystick Switch)
ISR(INT0_vect) {
    // Cycle modes: 0 -> 1 -> 2 -> 3 -> 0
    switch (Mode) {
        case MODE_LOW_TO_HIGH:
            Mode = MODE_HIGH_TO_LOW;
            break;
        case MODE_HIGH_TO_LOW:
            Mode = MODE_BACK_AND_FORTH;
            break;
        case MODE_BACK_AND_FORTH:
            Mode = MODE_CUSTOM;
            break;
        case MODE_CUSTOM:
            Mode = MODE_LOW_TO_HIGH;
            break;
        default:
            Mode = MODE_LOW_TO_HIGH;
            break;
    }
    modeChanged = 1; // Signal main game loop that mode changed
}

// Pattern 1: Low to High (PA0 to PA7)
void update_low_to_high(void) {
    static unsigned char index = 0;
    LEDS = ~(1 << index); // Active low: bit 0 lights up LED
    index = (index + 1) % 8;
}

// Pattern 2: High to Low (PA7 to PA0)
void update_high_to_low(void) {
    static unsigned char index = 7;
    LEDS = ~(1 << index);
    if (index == 0) {
        index = 7;
    } else {
        index--;
    }
}

// Pattern 3: Back and Forth (PA0 to PA7 to PA0)
void update_back_and_forth(void) {
    static unsigned char index = 0;
    static int direction = 1; // 1 = upward, -1 = downward

    LEDS = ~(1 << index);

    if (direction == 1) {
        if (index == 7) {
            direction = -1;
            index = 6; // Reverse immediately without staying double duration on ends
        } else {
            index++;
        }
    } else {
        if (index == 0) {
            direction = 1;
            index = 1; // Reverse immediately without staying double duration on ends
        } else {
            index--;
        }
    }
}

// Pattern 4: bounce off middle
void update_custom_pattern(void) {
    static unsigned char step = 0;
    const unsigned char patterns[] = {
        ~0x81, // PA7 & PA0
        ~0x42, // PA6 & PA1
        ~0x24, // PA5 & PA2
        ~0x18, // PA4 & PA3
        ~0x24, 
        ~0x42
    };

    LEDS = patterns[step];
    step = (step + 1) % (sizeof(patterns) / sizeof(patterns[0]));
}
#define ADC_CHANNELS                        3
#define ADC_OVERSAMPLING                    4
#define ADC_VALUE_RANGE                    (1024*ADC_OVERSAMPLING)

#define SEQUENCER_STEPS_RANGE               32
#define SEQUENCER_DEADBAND_THRESHOLD        (ADC_VALUE_RANGE/SEQUENCER_STEPS_RANGE/4)

#define SEQUENCER_ROTATE_CONTROL            0
#define SEQUENCER_FILL_CONTROL              1
#define SEQUENCER_STEP_CONTROL              2

#define SEQUENCER_CLOCK_DDR                 DDRD
#define SEQUENCER_CLOCK_PORT                PORTD
#define SEQUENCER_CLOCK_PINS                PIND
#define SEQUENCER_CLOCK_PIN                 PORTD3

#define SEQUENCER_RESET_DDR                 DDRD
#define SEQUENCER_RESET_PORT                PORTD
#define SEQUENCER_RESET_PINS                PIND
#define SEQUENCER_RESET_PIN                 PORTD2

#define SEQUENCER_OUTPUT_DDR                DDRB
#define SEQUENCER_OUTPUT_PORT               PORTB
#define SEQUENCER_OUTPUT_PINS               PINB
#define SEQUENCER_OUTPUT_PIN_A              PORTB0

#define SEQUENCER_TRIGGER_SWITCH_DDR        DDRD
#define SEQUENCER_TRIGGER_SWITCH_PORT       PORTD
#define SEQUENCER_TRIGGER_SWITCH_PINS       PIND
#define SEQUENCER_TRIGGER_SWITCH_PIN_A      PORTD7

#define SEQUENCER_ALTERNATE_SWITCH_DDR      DDRD
#define SEQUENCER_ALTERNATE_SWITCH_PORT     PORTD
#define SEQUENCER_ALTERNATE_SWITCH_PINS     PIND
#define SEQUENCER_ALTERNATE_SWITCH_PIN_A    PORTD6

#define SEQUENCER_LEDS_DDR                  DDRB
#define SEQUENCER_LEDS_PORT                 PORTB
#define SEQUENCER_LEDS_PINS                 PINB
#define SEQUENCER_LED_A_PIN                 PORTB4
#define SEQUENCER_LED_C_PIN                 PORTB3

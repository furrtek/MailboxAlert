// Receiver (in home) - v0.1b
// For ATTiny45, internal RC oscillator & CKDIV8, disable BOD
// L:0x52 H:0xDF X:0xFF

#define F_CPU 1000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>

// Pin definitions
#define RADIO	PB0
#define BUTTON	PB1
#define LED		PB2
#define VBATT	PB3
#define VDIV	PB4

// Hamming(4,2):
const uint8_t hamming[8] = { 0, 3, 5, 6, 9, 10, 12, 15 };

uint16_t volatile sr = 0;
uint8_t volatile flag_sense = 0;
uint8_t volatile flag_remote_batt = 0;
uint8_t volatile can_sleep = 1;

ISR(WDT_vect) {
	if (can_sleep) {
		WDTCR = _BV(WDIE) | _BV(WDE) | _BV(WDP2) | _BV(WDP1);	// Set WDIE back ! 1s @ 5V
		can_sleep = 0;
	} else {
		WDTCR |= _BV(WDIE) | _BV(WDE);	// Set WDIE back !
		can_sleep = 1;
	}
}

ISR(TIM0_OVF_vect) {
	sr = 0;
}

ISR(TIM0_COMPA_vect) {
	uint8_t c;

	sr = sr << 1;
	if (PINB & 0b00000100) sr |= 1;
	if ((sr & 0xFF0) == 0xF20) {
		// Check Hamming

		sr &= 0x00F;

		for (c = 0; c < 8; c++)
			if (hamming[c] == sr) break;

		flag_sense = c & 4;
		flag_remote_batt = c & 1;
	}
}

ISR(INT0_vect) {
	TCNT0 = 0;
	TCCR0A = 0x00;
	OCR0A = 0x28;			// Critical ! Between 2 symbols
	TCCR0B = 0b00000010;
	TIMSK = 0b00010010;		// Overflow interrupt + Compare A
}

uint16_t get_adc() {
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADCW;
}

int main(void) {
	uint8_t flag_local_batt = 0;
	uint16_t batt_test_tmr = 0;
	uint16_t v;

	DDRB = 0b00010000;
	PORTB = 0x00;

	ACSR = _BV(ACD);		// Disable comparator
	//DIDR0 = _BV(VBATT);		// Disable digital input on VBATT sense pin

	//_delay_ms(500);

	sei();

	for (;;) {

		wdt_reset();
 		// Enable Watchdog interrupt (no reset) 8s @ 5V
		MCUSR = 0;
		WDTCR = _BV(WDIE) | _BV(WDE) | _BV(WDP3) | _BV(WDP0);
		// Sleepy time u_u
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_enable();
		sei();
		sleep_cpu();
		sleep_disable();
		cli();
		can_sleep = 0;

		/*if (!batt_test_tmr) {
			ADCSRA = _BV(ADEN) | _BV(ADPS1) | _BV(ADPS0);	// Enable ADC
			ADMUX = _BV(REFS1) | _BV(MUX1) | _BV(MUX0);		// 1.1V ref 125kHz

			DDRB |= _BV(VDIV);			// Enable voltage divider
			_delay_ms(1);
			v = get_adc();
			DDRB &= ~_BV(VDIV);			// Disable voltage divider

			ADCSRA = 0x00;				// Disable ADC

			if (v < 635) {				// 7.5V*10k/(100k+10k)*(1024/1.1V)
				flag_local_batt = 1;	// Set low battery flag
			} else {
				flag_local_batt = 0;
			}

			batt_test_tmr = 14400;		// ~1 hour (60*60/0.25s)
		} else {
			batt_test_tmr--;
		}*/

		//PORTB |= _BV(POWER);	// Enable receiver
		//_delay_us(20);

		GIFR = 0b01000000;
		MCUCR = 0b00000011;		// INT0 on rising edge of PB0
		GIMSK = 0b01000000;		// Enable INT0

		sei();

		//PORTB &= ~_BV(POWER);	// Disable receiver

		while (!can_sleep) {}

		cli();

		MCUCR = 0b00000000;
		GIMSK = 0b00000000;

		if (flag_sense)
			PORTB = 0b00010000;
		else
			PORTB = 0b00000000;


	}
}

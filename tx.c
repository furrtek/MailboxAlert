// Transmitter (in mailbox) - v0.1
// For ATTiny45, internal RC oscillator & CKDIV8, disable BOD
// L:0x52 H:0xDF X:0xFF

#define F_CPU 1000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

// Pin definitions
#define RADIO	PB0
#define SENSE	PB1
#define POWER	PB2
#define VBATT	PB3
#define VDIV	PB4

#define REPEATS	4

// Hamming(4,2):
const uint8_t hamming[8] = { 0, 3, 5, 6, 9, 10, 12, 15 };

// In seconds:
const uint16_t repeat_periods[REPEATS] = { 8, 22, 58, 157 };

ISR(WDT_vect) {
	WDTCR = _BV(WDIE) | _BV(WDE) | _BV(WDP2);	// Set WDIE back !
}

uint16_t get_adc() {
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADCW;
}

void tx_message(uint16_t message) {
	uint8_t c;

	for (c = 0; c < 12; c++) {
		if ((message << c) & 0x800) {
			PORTB |= _BV(RADIO);		// 1: ""__
			_delay_us(500);
			PORTB &= ~_BV(RADIO);
			_delay_us(500);
		} else {
			PORTB |= _BV(RADIO);		// 0: "___
			_delay_us(250);
			PORTB &= ~_BV(RADIO);
			_delay_us(750);
		}
	}
}

int main(void) {
	uint8_t flag_sense = 0;
	uint8_t repeat_idx = 0;
	uint8_t flag_battery = 0;
	uint16_t tx_tmr = 0;
	uint16_t batt_test_tmr = 0;
	uint16_t message;
	uint16_t v;
 
 	// Enable Watchdog interrupt (no reset) 250ms @ 5V
	MCUSR = 0;
	WDTCR = _BV(WDIE) | _BV(WDE) | _BV(WDP2);

	DDRB = _BV(RADIO) | _BV(POWER);
	PORTB = 0x00;

	ACSR = _BV(ACD);		// Disable comparator
	DIDR0 = _BV(VBATT);		// Disable digital input on VBATT sense pin

	_delay_ms(500);

	for (;;) {

		// Sleepy time u_u
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_enable();
		sei();
		sleep_cpu();
		sleep_disable();
		cli();

		// Sensors check
		PORTB |= _BV(POWER);		// Enable sensors
		_delay_us(20);
		if (!(PINB & _BV(SENSE))) {
			if (!flag_sense) {
				flag_sense = 1;		// Set mail flag
				tx_tmr = 0;			// Reset transmit sequence
				repeat_idx = 0;
			}
		}
		PORTB &= ~_BV(POWER);		// Disable sensors

		if (!batt_test_tmr) {
			ADCSRA = _BV(ADEN) | _BV(ADPS1) | _BV(ADPS0);	// Enable ADC
			ADMUX = _BV(REFS1) | _BV(MUX1) | _BV(MUX0);		// 1.1V ref 125kHz

			DDRB |= _BV(VDIV);		// Enable voltage divider
			_delay_ms(1);
			v = get_adc();
			DDRB &= ~_BV(VDIV);		// Disable voltage divider

			ADCSRA = 0x00;			// Disable ADC

			if (v < 635) {			// 7.5V*10k/(100k+10k)*(1024/1.1V)
				flag_battery = 1;	// Set low battery flag
				tx_tmr = 0;			// Reset transmit sequence
				repeat_idx = 0;
			} else {
				flag_battery = 0;
			}

			batt_test_tmr = 14400;	// ~1 hour (60*60/0.25s)
		} else {
			batt_test_tmr--;
		}

		if (!tx_tmr) {
			if (repeat_idx < REPEATS) {
				message = 0b111100100000 | hamming[(flag_sense << 2) | (flag_battery)];
				tx_message(message);	// 12ms
				_delay_ms(40);
				tx_message(message);	// 12ms
				tx_tmr = repeat_periods[repeat_idx] * 4;
				repeat_idx++;
			} else {
				flag_sense = 0;		// Reset mail flag automatically after all repeats done
			}
		} else {
			tx_tmr--;
		}

	}
}

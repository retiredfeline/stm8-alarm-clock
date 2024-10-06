#define STM8S103

#include "clock.h"

#include "mcu.h"
#include "tick.h"
#include "mains.h"
#include "alarm.h"
#include "sleep.h"
#include "display.h"
#include "button.h"

#include "pt.h"

#define MSPERTICK	2u
#define	DEPMIN		(100u / MSPERTICK)	// debounce period
#define	RPTTHRESH	(400u / MSPERTICK)	// repeat threshold after debounce
#define	RPTPERIOD	(250u / MSPERTICK)	// repeat period
#define	BUTTON_TIMEOUT	(64000u / MSPERTICK)	// revert to NORMAL_MODE if no button press

enum set_mode mode = NORMAL_MODE;
static uint8_t swstate, swtent, swmin, swrepeat;
static uint16_t button_timeout;
static struct pt pt;

#define	MAINS_FREQ	50u
#define	MAINS_HALF	25u

static void switchaction()
{
	switch (swstate & B_BOTH) {
	case B_0:
		mode++;
		if (mode >= END_MODE)
			mode = NORMAL_MODE;
		break;
	case B_1:	// also cancels alarm in normal mode
		switch (mode) {
		case NORMAL_MODE:
			if (sleep_minutes > 0)
				sleep_zero();
			return;
		case TIME_HOURS:
			mains_time.hours++;
			if (mains_time.hours >= 24)
				mains_time.hours = 0;
			break;
		case TIME_MINS:
			mains_time.seconds = 0;
			mains_time.minutes++;
			if (mains_time.minutes >= 60)
				mains_time.minutes = 0;
			break;
		case ALARM_SET:
			alarm_time.on = !alarm_time.on;
			break;
		case ALARM_HOURS:
			alarm_time.hours++;
			if (alarm_time.hours >= 24)
				alarm_time.hours = 0;
			break;
		case ALARM_MINS:
			alarm_time.minutes++;
			if (alarm_time.minutes >= 60)
				alarm_time.minutes = 0;
			break;
		case SLEEP_MINS:
			if (sleep_minutes == 0)
				sleep_on();
			else
				sleep_sub1();
			break;
		}
		display_update();
		alarm_check();
		break;
	}
	if (mode == NORMAL_MODE)
		button_timeout = 0;
	else
		button_timeout = BUTTON_TIMEOUT;
}

static inline void reinitstate()
{
	swmin = DEPMIN;
	swrepeat = RPTTHRESH;
	swtent = swstate = B_NONE;
}

static
PT_THREAD(switchhandler(struct pt *pt))
{
	PT_BEGIN(pt);
	PT_WAIT_UNTIL(pt, swstate != swtent);
	swtent = swstate;
	PT_WAIT_UNTIL(pt, --swmin <= 0 || swstate != swtent);
	if (swstate != swtent) {		// changed, restart
		reinitstate();
		PT_RESTART(pt);
	}
	switchaction();
	PT_WAIT_UNTIL(pt, --swrepeat <= 0 || swstate != swtent);
	if (swstate != swtent) {		// changed, restart
		reinitstate();
		PT_RESTART(pt);
	}
	switchaction();
	for (;;) {
		swrepeat = RPTPERIOD;
		PT_WAIT_UNTIL(pt, --swrepeat <= 0 || swstate == B_NONE);
		if (swstate == B_NONE) {	// released, restart
			reinitstate();
			PT_RESTART(pt);
		}
		switchaction();
	}
	PT_END(pt);
}

static inline uint8_t incmin(void) 
{			
	mains_time.minutes++; 
	if (sleep_minutes > 0)
		sleep_sub1();
	if (mains_time.minutes >= 60) {
		mains_time.minutes = 0;
		return 1;
	}		
	return 0;
}			

static inline void inchour(void) 
{			
	mains_time.hours++;
	if (mains_time.hours >= 24)
		mains_time.hours = 0;
}		

static inline void incctrs(void)
{
	mains_time.hz--;
	if (mains_time.hz == 0) {
		mains_time.hz = MAINS_FREQ;
		mains_time.colon = 0;
		mains_time.seconds++;
		if (mains_time.seconds >= 60) {
			mains_time.seconds = 0;
			if (incmin())
				inchour();
			alarm_check();		// once per minute
		}
		display_update();
	}
}

int main(void)
{
	mcu_init();
	tick_init();
	mains_init();
	alarm_init();
	sleep_init();
	display_init();
	reinitstate();
	button_init();
	mcu_enable_interrupts();

	mains_time.hours = 12;
	mains_time.minutes = 34;
	mains_time.seconds = 56;
	mains_time.state = 0;
	mains_time.hz = MAINS_FREQ;
	button_timeout = 0;
	display_update();
#ifdef	DEBUG
	uint8_t counter = 0;
#endif	// DEBUG
	for (;;) {
		uint8_t mains_pin = mains_read();
		if (mains_time.state == 0 && mains_pin != 0) {
			mains_time.state = 1;
			incctrs();
			display_update();
		} else if (mains_time.state != 0 && mains_pin == 0) {
			mains_time.state = 0;
		}
		if (mains_time.colon == 0 && mains_time.hz < MAINS_HALF) {
			mains_time.colon = 0x80;
			display_update();
		}
		while (!tick_check())
			;
		if (button_timeout == 0)		// if no activity, revert to NORMAL_MODE
			mode = NORMAL_MODE;
		else
			button_timeout--;
#ifdef	DEBUG
		counter++;
		// every 0.5 second flip the LED pin -> 1 Hz
		if (counter >= 250) {
			display_flip_b5();
			counter = 0;
		}
#endif	// DEBUG
		swstate = button_state();
		PT_SCHEDULE(switchhandler(&pt));
	}
}

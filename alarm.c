#define STM8S103

#include "clock.h"
#include "mains.h"
#include "sleep.h"

#include "alarm.h"

struct alarm_time alarm_time;

void alarm_init(void)
{
	alarm_time.on = alarm_time.hours = alarm_time.minutes = 0;
}

void alarm_check(void)
{
	if (alarm_time.on && alarm_time.hours == mains_time.hours && alarm_time.minutes == mains_time.minutes)
		sleep_on();
}

#define STM8S103

#include <stm8s_gpio.h>

#include "clock.h"

#include "sleep.h"

uint8_t sleep_minutes;
const static port_pin alarm_out = { GPIOD, GPIO_PIN_4 };

void sleep_init(void)
{
	sleep_minutes = 0;
	GPIO_Init(alarm_out.port, alarm_out.pin, GPIO_MODE_OUT_PP_LOW_SLOW);
}

void sleep_on(void)
{
	if (sleep_minutes > 0)
		return;
	sleep_minutes = 59;
	GPIO_WriteHigh(alarm_out.port, alarm_out.pin);
}

void sleep_sub1(void)
{
	sleep_minutes--;
	if (sleep_minutes == 0)
		GPIO_WriteLow(alarm_out.port, alarm_out.pin);
}

void sleep_zero(void)
{
	sleep_minutes = 0;
	GPIO_WriteLow(alarm_out.port, alarm_out.pin);
}

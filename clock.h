#include "stm8s_gpio.h"

typedef struct port_pin {
	GPIO_TypeDef		*port;
	GPIO_Pin_TypeDef	pin;
} port_pin;

#ifdef	ALARM_CLOCK
enum set_mode { NORMAL_MODE = 0, TIME_HOURS = 1, TIME_MINS = 2, ALARM_SET = 3, ALARM_HOURS = 4, ALARM_MINS = 5, SLEEP_MINS = 6, END_MODE = 7 };
#else
enum set_mode { NORMAL_MODE = 0, TIME_HOURS = 1, TIME_MINS = 2, ALARM_SET = 3, ALARM_HOURS = 4, ALARM_MINS = 5, SLEEP_MINS = 6, END_MODE = 3 };
#endif	// ALARM_CLOCK
extern enum set_mode mode;

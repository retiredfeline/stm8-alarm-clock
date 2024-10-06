#include	<stm8s.h>

typedef struct alarm_time {
	uint8_t hours, minutes;
	uint8_t on;
};
extern struct alarm_time alarm_time;

extern void alarm_init(void);
extern void alarm_check(void);

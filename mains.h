#define	STM8S103

#include "stm8s_gpio.h"

typedef struct mains_time {
	uint8_t hours, minutes, seconds, colon;
	uint8_t hz, state;
};
extern struct mains_time mains_time;

extern void mains_init(void);
extern uint8_t mains_read(void);

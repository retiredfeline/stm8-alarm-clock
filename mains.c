#include "mains.h"

struct mains_time mains_time;

void mains_init(void)
{
	GPIO_Init(GPIOA, GPIO_PIN_3, GPIO_MODE_IN_PU_NO_IT);
}

uint8_t mains_read(void)
{
	return GPIO_ReadInputPin(GPIOA, GPIO_PIN_3);
}

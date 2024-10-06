#define STM8S103

#include <stm8s_gpio.h>

#ifdef	HARDWARE_SPI
#include <stm8s_spi.h>
#endif

#include "clock.h"

#include "mains.h"
#include "alarm.h"
#include "sleep.h"

#include "display.h"

static uint8_t display_buffer[4];
const static uint8_t font[] =
{ 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#define	COLON	0x80

const static port_pin rclk = { GPIOC, GPIO_PIN_4 };
const static port_pin srclk = { GPIOC, GPIO_PIN_5 };
const static port_pin ser = { GPIOC, GPIO_PIN_6 };

#define	GPIOWriteValue(port,pin,val)	((val) ? GPIO_WriteHigh(port,pin) : GPIO_WriteLow(port,pin))

static void setBrightness(uint8_t);

void display_init(void)
{
	GPIO_DeInit(GPIOC);
#ifdef	HARDWARE_SPI
	SPI_DeInit();
	// run SPI at about 250 KHz
	SPI_Init(SPI_FIRSTBIT_MSB, SPI_BAUDRATEPRESCALER_64, SPI_MODE_MASTER,
		SPI_CLOCKPOLARITY_LOW, SPI_CLOCKPHASE_1EDGE,
		SPI_DATADIRECTION_1LINE_TX, SPI_NSS_SOFT, 0x7);
	SPI_Cmd(ENABLE);
	// GPIO still used for RCLK signal
	GPIO_Init(rclk.port, rclk.pin, GPIO_MODE_OUT_PP_LOW_SLOW);
#else
	GPIO_Init(rclk.port, rclk.pin, GPIO_MODE_OUT_PP_LOW_SLOW);
	GPIO_Init(srclk.port, srclk.pin, GPIO_MODE_OUT_PP_LOW_SLOW);
	GPIO_Init(ser.port, ser.pin, GPIO_MODE_OUT_PP_LOW_SLOW);
#ifdef	TM1637
	setBrightness(0x8f);
#endif	// TM1637
#endif	// HARDWARE_SPI
#ifdef	DEBUG
	GPIO_DeInit(GPIOB);
	GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_OUT_PP_HIGH_SLOW);	// LED
#endif	// DEBUG
}

#ifdef	DEBUG
void display_flip_b5(void)
{
	GPIO_WriteReverse(GPIOB, GPIO_PIN_5);
}       
#endif	// DEBUG

// A bug is tickled if value is declared uint8_t. C requires widening to int anyway
static void digits_update(uint8_t *buffer, int value)
{
	int	tens = value / 10;
	int	units = value % 10;

	buffer[0] = font[tens];
	buffer[1] = font[units];
}

static void display_fill()
{
	uint8_t *p = display_buffer;
	for (uint8_t i = 0; i < sizeof(display_buffer); i++, p++)
		*p = 0x0;
	switch (mode) {
	case NORMAL_MODE:
		digits_update(&display_buffer[0], mains_time.hours);
		digits_update(&display_buffer[2], mains_time.minutes);
		display_buffer[2] |= mains_time.colon;
		break;
	case TIME_HOURS:
		digits_update(&display_buffer[0], mains_time.hours);
		display_buffer[2] |= mains_time.colon;
		break;
	case TIME_MINS:
		digits_update(&display_buffer[2], mains_time.minutes);
		display_buffer[2] |= mains_time.colon;
		break;
	case ALARM_SET:
		digits_update(&display_buffer[0], alarm_time.hours);
		digits_update(&display_buffer[2], alarm_time.minutes);
		if (alarm_time.on)
			display_buffer[2] |= COLON;
		break;
	case ALARM_HOURS:
		digits_update(&display_buffer[0], alarm_time.hours);
		display_buffer[2] |= COLON;
		break;
	case ALARM_MINS:
		digits_update(&display_buffer[2], alarm_time.minutes);
		display_buffer[2] |= COLON;
		break;
	case SLEEP_MINS:
		digits_update(&display_buffer[2], sleep_minutes);
		display_buffer[1] |= COLON;
		display_buffer[2] |= COLON;
		break;
	}
}

// taken from https://github.com/unfrozen/stm8_libs/blob/master/lib_delay.c
static void delay_usecs(uint8_t usecs)
{
	usecs;
__asm
	ld	a, (3, sp)
	dec	a
	clrw	x
	ld	xl,a
#ifdef STM8103
	sllw	x
#endif
	sllw	x
00001$:
	nop			; (1)
	decw	x		; (1)
	jrne	00001$		; (2)
__endasm;
}

#ifdef	HARDWARE_SPI	// 74HC595 implied

// send one translated byte
static void display_digit(uint8_t digit)
{
	while(SPI_GetFlagStatus(SPI_FLAG_BSY))
		;
	SPI_SendData(digit);
	while (!SPI_GetFlagStatus(SPI_FLAG_TXE))
		;
}

#else

// send one translated byte, MSb first
static void display_digit(uint8_t digit)
{
	for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
		GPIOWriteValue(ser.port, ser.pin, digit & mask);
		delay_usecs(5);
		GPIO_WriteHigh(srclk.port, srclk.pin);
		delay_usecs(5);
		GPIO_WriteLow(srclk.port, srclk.pin);
		delay_usecs(5);
	}
}

#endif	// HARDWARE_SPI

#ifdef	TM1637

static void startXfer()
{
	GPIO_WriteHigh(srclk.port, srclk.pin);
	GPIO_WriteHigh(ser.port, ser.pin);
	delay_usecs(5);
	GPIO_WriteLow(ser.port, ser.pin);
	GPIO_WriteLow(srclk.port, srclk.pin);
	delay_usecs(5);
}

static void stopXfer()
{
	GPIO_WriteLow(srclk.port, srclk.pin);
	GPIO_WriteLow(ser.port, ser.pin);
	delay_usecs(5);
	GPIO_WriteHigh(srclk.port, srclk.pin);
	GPIO_WriteHigh(ser.port, ser.pin);
	delay_usecs(5);
}

static uint8_t writeByte(uint8_t value)
{
	uint8_t i;

	for (i = 0; i < 8; i++) {
		GPIO_WriteLow(srclk.port, srclk.pin);
		delay_usecs(5);
		GPIOWriteValue(ser.port, ser.pin, value & 0x1);
		delay_usecs(5);
		GPIO_WriteHigh(srclk.port, srclk.pin);
		delay_usecs(5);
		value >>= 1;
	}
	GPIO_WriteLow(srclk.port, srclk.pin);
	delay_usecs(5);
	GPIO_WriteHigh(ser.port, ser.pin);
	GPIO_WriteHigh(srclk.port, srclk.pin);
	delay_usecs(5);
	return 1;
}

static void setBrightness(uint8_t val)
{
	startXfer();
	writeByte(val);
	stopXfer();
}

void display_update(void)
{
	display_fill();
	startXfer();
	writeByte(0x40);
	stopXfer();
	startXfer();
	writeByte(0xc0);
	for (uint8_t i = 0, *p = display_buffer; i < sizeof(display_buffer); i++, p++)
		writeByte(*p);
	stopXfer();
}

#else

// send load pulse to latch serial data to output register
static void display_load(void)
{
	GPIO_WriteHigh(rclk.port, rclk.pin);
	delay_usecs(5);
	GPIO_WriteLow(rclk.port, rclk.pin);
	delay_usecs(5);
}

void display_update(void)
{
	display_fill();
	for (uint8_t i = 0, *p = display_buffer; i < sizeof(display_buffer); i++, p++)
		display_digit(*p);
	display_load();
}

#endif	// TM1637

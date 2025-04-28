# Compiler
CC=sdcc
CFLAGS=-mstm8
INCLUDES=-Iinclude -Ipt-1.4
LIBS=spl.lib
ALARM_CLOCK?=y	# by default an alarm clock, otherwise plain clock
LOWON?=		# y means 0 turns on segment
HARDWARE_SPI?=	# y to use STM8 SPI port
TM1637?=	# y for TM1637 bitbanged
DEBUG?=		# y for DEBUG LED blink

ifdef ALARM_CLOCK
CFLAGS+=-DALARM_CLOCK
endif

ifdef LOWON
CFLAGS+=-DLOWON
endif

ifdef HARDWARE_SPI
CFLAGS+=-DHARDWARE_SPI
endif

ifdef TM1637
CFLAGS+=-DTM1637
endif

ifdef DEBUG
CFLAGS+=-DDEBUG
endif

clock.hex:	clock.rel mcu.rel tick.rel mains.rel display.rel button.rel alarm.rel sleep.rel
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

%.rel:		%.c %.h
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $(<:.c=.rel)

%.flash:	%.hex
	test -r $(@:.flash=.hex) && stm8flash -cstlinkv2 -pstm8s103f3 -w$(@:.flash=.hex)

flash:	clock.flash

clean:
	rm -f *.asm *.cdb *.lk *.lst *.map *.mem *.rel *.rst *.sym

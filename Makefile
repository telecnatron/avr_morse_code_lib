# -----------------------------------------------------
# CPU defs 
# CPU as used by avrdude
CPU_AVRDUDE=m8
# CPU as used by GCC
CPU_MMCU=atmega8


# CPU clock frequency
F_CPU=16000000
# -----------------------------------------------------
CC=avr-gcc
CFLAGS=-O2 -Os -mmcu=$(CPU_MMCU) -I. -Wall
# instruct linker to gc unused sections (functions)
CFLAGS += -fdata-sections -ffunction-sections 
# limit use of compiler inlining and call prologs (whatever that is)
CFLAGS += -finline-limit=3  -fno-inline-small-functions
# generate assembly output - gstabs to add c source and line nubers to asm
CFLAGS += -Wa,-adhlns=main.lst -gstabs
# linker flags
LDFLAGS=-Wl,-Map,main.map -Wl,--relax 
# instruct linker to gc unused sections
LDFLAGS += -Wl,-gc-sections  

# ---------------------------------------------
# definitions
DEFS=-D F_CPU=$(F_CPU) 


# list source file here
SOURCES=main.c morse.c 

OBJS=$(SOURCES:.c=.o)

INCLUDES=

# ---------------------------------------------
# This section is the traditional compile-the-objs-then-link type 
all: main.hex

main.o: main.c
	$(CC) -c $(CFLAGS) $(DEFS) -o $@ $<	

%.o: %.c %.h 
	$(CC) -c $(CFLAGS) $(DEFS) -o $@ $<

# ---------------------------------------------
# This section links previously compiled object modules
main.elf: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o main.elf $(OBJS)

# ---------------------------------------------
# This section compiles and links everything in one go - hence allowing whole-program optimisations
# see http://www.tty1.net/blog/2008/avr-gcc-optimisations_en.html for optimisation details
#main.elf: $(OBJS)
#	$(CC)  $(CFLAGS) $(DEFS) $(LDFLAGS)  --combine -fwhole-program  -o main.elf  $(SOURCES)

# ---------------------------------------------
main.hex: main.elf
	avr-objcopy -R .eeprom -O ihex main.elf  main.hex
	avr-size main.elf

fuses:
	echo XXX use this command line to program the fuses XXX
	echo sudo avrdude -v -c usbasp -p $(CPU_AVRDUDE)  -U lfuse:w:0xff:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m 

program: 
	sudo avrdude  -c usbasp -p m8 -U flash:w:main.hex:a

update:
	svn update

clean:
	rm *.o
	rm main.hex
	rm main.elf
	rm main.map
	rm main.lst

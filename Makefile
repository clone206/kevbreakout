# Makefile

all: breakout

breakout: breakout.o
	gcc breakout.o -o breakout.exe -lncurses

breakout.o: breakout.c
	gcc -c breakout.c

clean:
	rm -f breakout.o breakout.exe

CC = gcc
CFLAGS = $(shell sdl2-config --cflags) -Wall
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_ttf -lm

all: hotcold

hotcold: hotcold.c
	$(CC) $(CFLAGS) -o hotcold hotcold.c $(LDFLAGS)

clean:
	rm -f hotcold

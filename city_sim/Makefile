CC = g++
CFLAGS = -Wall -O2
LDFLAGS = -lSDL2 -lSDL2_ttf

# Default target
all: city_sim

# Main build target
city_sim: city_sim.cpp
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Clean build files
clean:
	rm -f city_sim

.PHONY: all clean

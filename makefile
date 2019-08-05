EXEC     = $(shell basename $$(pwd))
CC       = cc
CFLAGS   = -Wall
LOCATION = -F/Library/Frameworks
FRAMES	 = -framework SDL2 -framework SDL2_image

SRC      = $(wildcard *.c)
OBJ      = $(SRC:.c=.o)

all: $(EXEC)

${EXEC}: $(OBJ)
	$(CC) -o $@ $^ $(LOCATION) $(FRAMES)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	@rm -rf *.o

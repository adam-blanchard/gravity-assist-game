CC = gcc
FRAMEWORK = -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
CFLAGS = -Iinclude -Wall
LDFLAGS = -Llib -lraylib
SRC = src/*.c
OUT = build/gravity_assist_game

all:
	$(CC) $(FRAMEWORK) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(OUT) 

clean:
	rm -f build/*
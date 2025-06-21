CC = clang++
CFLAGS = -O3 -Wall -std=c++20
INCLUDE = -I. -I/opt/homebrew/Cellar/raylib/5.5/include
LIBS = -L/opt/homebrew/Cellar/raylib/5.5/lib -lraylib

all: main

main:
	$(CC) -o build/picwave $(CFLAGS) $(INCLUDE) $(LIBS) src/main.cpp src/ImgConvo.cpp


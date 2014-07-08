#!/bin/sh
gcc -I/usr/local/include -L/usr/local/lib -L/usr/X11R6/lib -O3 -Wall -o nw *.c -lGL -lGLU -lglut -lm

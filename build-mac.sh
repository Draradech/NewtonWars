#!/bin/sh
gcc -I/usr/local/include -L/usr/local/lib -L/usr/X11R6/lib -O3 -Wall -o nw -DTARGET_GLUT color.c config.c display.c main.c network.c simulation.c vector.c -lGL -lGLU -lglut -lm

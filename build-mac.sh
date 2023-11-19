#!/bin/sh
gcc -I/usr/local/include -L/usr/local/lib -L/opt/X11/lib -O3 -Wall -Wno-deprecated-declarations -o nw -DTARGET_GLUT config.c display.c main.c network.c simulation.c vector.c hsluv.c -framework OpenGL -framework GLUT -lm

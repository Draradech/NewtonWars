#!/bin/sh
gcc -O3 -Wall -Wextra -o nw -DTARGET_GLUT config.c display.c main.c network.c simulation.c vector.c hsluv.c -lGL -lGLU -lglut -lm

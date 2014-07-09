#!/bin/sh
gcc -O3 -Wall -o nw -DTARGET_GLUT color.c config.c display.c main.c network.c simulation.c vector.c -lGL -lGLU -lglut -lm

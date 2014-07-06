all:
	gcc -O3 -Wall -o nw newtonwars.c color.c simulation.c vector.c -lGL -lGLU -lglut -lm

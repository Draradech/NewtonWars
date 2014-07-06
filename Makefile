all:
	gcc -O3 -Wall -o nw color.c config.c interface.c main.c network.c simulation.c vector.c -lGL -lGLU -lglut -lm

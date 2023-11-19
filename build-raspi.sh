#!/bin/sh
gcc -O3 -Wall -Wextra -Wno-unused-parameter -std=gnu11 -o nw -DTARGET_RASPI config.c display.c main.c network.c simulation.c vector.c hsluv.c -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads/ -I/opt/vc/include/interface/vmcs_host/linux -L/opt/vc/lib -lm -lbrcmEGL -lbrcmGLESv2 -lbcm_host -lvchiq_arm

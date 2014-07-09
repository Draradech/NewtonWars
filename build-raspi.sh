#!/bin/sh
gcc -O3 -Wall -o nw -DTARGET_RASPI color.c  config.c  display-common.c  display-raspi.c  main.c  network.c  simulation.c  vector.c -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads/ -I/opt/vc/include/interface/vmcs_host/linux -L/opt/vc/lib -lm -lGLESv2 -lEGL -lbcm_host -lvchiq_arm

cl /Fenw.exe /I../freeglut/include /DTARGET_GLUT /D_USE_MATH_DEFINES color.c  config.c  display.c  main.c  network.c  simulation.c  vector.c /link /LIBPATH:../freeglut/lib ws2_32.lib
del *.obj

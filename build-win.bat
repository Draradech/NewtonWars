cl /Fenw.exe /I../freeglut/include /D_USE_MATH_DEFINES *.c /link /LIBPATH:../freeglut/lib ws2_32.lib
del *.obj

NewtonWars
==========

NewtonWars is a space battle game. Gravity is the main theme, influencing the path of your missiles around numerous planets on the battlefield.

It is heavily inspired by [Gravitational Combat](http://home.cs.tum.edu/~jain/software/gravcombat.php).

The game is best played in "party mode" with the display on a TV or projector and each participant connected via telnet (with a laptop or smartphone). Other possible modes (i.e. local hot-seat multiplayer) are currently not implemented, but may be added later.


Changelog
=========
2014-07-11
----------
 * prevent player spawns near existing shots

2014-07-10
----------
 * add font rendering to unified renderer
 * performance: directly render stored shots (w/o prior conversion)

2014-07-09
----------
 * add raspberry pi support
 * unified renderer working for glut and raspberry pi targets

2014-07-08
----------
 * fix windows and mac compatibility
 * remove fake Makefile
 * add build scripts
 * fix force/velocity
 * rename interface to display
 * rename force to velocity

2014-07-06
----------
 * decouple network code from rendering
 * move almost all data into simulation, renderer is a simple view now
 * simulation is now prepared for introduction of additional gamemodes (realtime, realtime with multiple shots, etc.)
 * make server echo switchable per connection for strange telnet clients

2014-07-05
----------
 * playable version - initial commit

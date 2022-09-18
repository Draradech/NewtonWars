NewtonWars
==========

NewtonWars is a space battle game. Gravity is the main theme, influencing the path of your missiles around numerous planets on the battlefield.

It is heavily inspired by [Gravitational Combat](https://web.archive.org/web/20041128233753/http://home.cs.tum.edu/~jain/software/gravcombat.php).

The game is played with the display on a TV or projector and each participant connected via telnet (with a laptop or smartphone). 

Hint: NewtonWars assumes vsync is enabled. If it is running way too fast on your system, you can use the throttle argument to slow it down (set to 16 for example).


Milestones
==========
2022-09-18
----------
Added debris particles on death, changed some setting defaults

2017-05-27
----------
Game is played in rounds (duration configurable), half of the victims energy is added to the killer

2016-09-06
----------
Fair player placement - calculate gravity potential and place players at compareable potential levels

2016-06-09
----------
Realtime mode - all players can shoot simultaneously (and have multiple shots at a time), energy is added based on time

2016-05-20
----------
Energy mode - shots now cost energy based on their velocity, energy is added per round

2015-06-06
----------
It is now possible to write bots, example included

2014-07-11
----------
Ported to Raspberry Pi, unified renderer for all platforms

2014-07-06
----------
First playable version

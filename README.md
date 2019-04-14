# UglyMetronome
It's ugly.  It's simple.

Many years ago, I wanted a simple metronome for my PC that I could quickly select from a few tempos.  I also wanted a
ramp function so the tempo would gradually increase as I played along.  I also wanted to keep track of how long I 
practice a piece at a tempo.  I couldn't find anything, so I created my own.

The original code was developed in Visual Studio 6/MFC with DirectX 9.0.  Thus, it looks ugly, but I got used to it.  
When I ported MFC to FLTK and DirectX to Jack, I retained the ugliness.

Features:

Stores presets.  The configuration is stored in a text file, metro.txt, located in same directory as your binary.
Several sounds to choose from.
Tempo ramping function!
Includes a built-in timer

Binaries are available for 
Windows (built on VS2008/XP).
Ubuntu 18.0.4

Dependancies
1. You will need FLTK
https://www.fltk.org/software.php
Follow the instructions from FLTK to build the library for your target.
Includes FLTK FLUID file metro1.fl

2. You will need Jack audio
http://jackaudio.org/downloads/

Libraries to include in your build (Linux)
fltk
Xrender
Xcursor
Xfixes
Xext
Xft
fontconfig
Xinerama
dl
X11
jack

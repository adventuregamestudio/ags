.. index::
   Engine
   Run-time engine
   Interpreter

The run-time engine
###################

The engine (also called the "interpreter") is what runs your game and is what
the end player will use.

If you are using the default interface, then you use the right mouse button to
cycle between the available 'modes', and the left button to use the current
mode on the mouse position. You can also move the mouse to the top of the
screen to bring up the icon bar where you can directly select a mode.
To exit the engine, press Ctrl-Q. You can save your game position using F5
and restore with F7.

The controls described above work with the default setup; however, you can
customize your game to use a different interface and shortcut keys.


.. index::
   Demo game

The demo game
=============

The first thing you'll probably want to do is to run the demo game. This
game will try to show you some of what AGS can do.

To run the demo, choose the "Open the Demo Game" option from the AGS folder
in your start menu. If you're looking for the files, they're located in the
All Users Application Data folder (this is for compatibility with the security
filters in Windows Vista and later versions).

*NOTE:* The demo game is currently under development. It has various unfinished
or unimplemented areas.


.. _GraphicsDriver:

.. index::
   Direct3D engine

Graphics driver selection
=========================

AGS has two different graphics drivers when run on Windows -- DirectDraw and Direct3D.

DirectDraw is the 'classic' software graphics driver, that AGS has used ever since
the initial Windows version was released. It's perfectly fine for simple games
that don't use many large sprites, tinting or alpha blending. It's also quite fast
at doing RawDrawing to the screen.

Direct3D is a new, hardware accelerated graphics driver. It uses the Direct3D 9.0 to
render the game in a fully hardware-accelerated environment. This means that the
game will run a lot faster if you use features such as alpha blending and tinting,
which are quite slow to perform in software mode. However, with Direct3D doing RawDraw
operations can be quite slow, and the driver won't work on all graphics cards.

No matter which you choose as your default graphics driver, the player can always
run the Setup program and switch to using the other driver if they are having problems
on their PC.

System Requirements
-------------------


* *DirectDraw*: any Windows-based PC with DirectX 5 or later installed.
* *Direct3D*: any Windows-based PC with DirectX 9.0 installed and a graphics card designed
  for DirectX 8.1 or later (most cards manufactured from 2003 onwards).


If you get the error message "Graphics card does not support Pixel Shader 1.4" on startup,
this indicates that your graphics card is too old to run with the Direct3D driver. You
should choose the DirectDraw driver instead.

See Also: :ref:`System.HardwareAcceleration property <System.HardwareAcceleration>`


.. index::
   Setup

Run-time engine setup
=====================

The engine Setup program allows the player to customize certain game settings.

Firstly, they can change the game resolution. If they do so, all your game
graphics will automatically be stretched or shrunk to fit, and the option is a
handy way for the player to start the game if their system won't run at the default
resolution for some reason.

They can also select to run the game in a window on the desktop rather than full-screen.
This will take a performance hit though, so it's always preferable to run full-screen.

"Enable side-borders" and "Enable top & bottom borders" options allow the game to
choose from wider range of resolutions, including the ones which display game surrounded
by horizontal and/or vertical borders (the so-called "letterboxed" and "pillarboxed" modes).

"Smooth scaled sprites" will apply anti-aliasing to scaled characters, in order to
give a smoother look to the resizing. This can slow down the game though, so it is off
by default.

The "Downgrade 32-bit graphics to 16-bit" option is only available for 32-bit games.
It allows people with slower PCs to choose to play the game at 16-bit instead, in order
to boost performance. If they use this, the graphical quality will reduce, but it should at
least allow them to play the game at a decent speed.

Finally, the "Use 85 Hz display" option sets the monitor refresh rate to 85 Hz to run the game,
which eliminates flicker. However, this does not work on all monitors, and not at all on flat
panel displays, which is why it is disabled by default.

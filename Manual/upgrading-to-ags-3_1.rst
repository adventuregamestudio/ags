.. _UpgradeTo31:

Upgrading to AGS 3.1
####################

AGS 3.1 contains some major changes over previous versions. The main change
is support for native hi-res co-ordinates.

What are native hi-res co-ordinates?
====================================

In previous versions of AGS, everything in the game was addressed in co-ordinates
ranging from 0-319 for X values, and 0-199 (or 239) for Y values.

This made sense if your game was 320x200 or 320x240, but if your game was running
at 640x480 or 800x600 this was a pain because you couldn't think in the co-ordinate
system that your game was written for.

Why was it like this?
=====================

When AGS was originally written it only supported 320x200, and extra resolutions
were then "bolted-on" on top later. But the fundamental co-ordinate system underneath
was never changed. This had the advantage that Setup could provide the option to
run a 320x200 game at 640x400 or vice versa, since it didn't make any difference
to the way the game would run internally.

Why change it?
==============

One of the main problems with the 320x200 co-ordinate system was that if you were
making a 640x480 game, you could only ever place objects and characters on even
co-ordinates. Aligning the objects with the background properly could be a pain
and involved adding an extra column of transparent pixels to the sprite to get it
to line up properly.

Originally this was judged to be a minor annoyance, but over time as more and more
people have started making hi-res games it has become a major limitation of AGS.

What has changed?
=================

If your game is 320x200 or 320x240, you will not notice any difference. Everything
should continue working the same way as before.

However, if your game is 640x400 or higher then the first thing you will notice
when importing your game into AGS 3.1 is that all the co-ordinates in the editor
will double. From now on, the editor will **always** display native hi-res
co-ordinates.

In your scripts though, you've probably got loads of commands like *player.Walk* where
you pass in specific low-res co-ordinates. By default, AGS will import your game
in backwards-compatible mode, where your scripts continue to work as before. This
means that all script commands and properties will continue to accept and return
low-res co-ordinates. It also means that the limitations of placing objects on even
pixels remains.

If you want to enable the new native hi-res co-ordinate support, there is an option
in the Scripting section of the General Settings pane called "Use low-res co-ordinates
in script". If you turn this off, AGS will expect native resolution co-ordinates to
be used in your script.

Obviously converting all your scripts to use hi-res co-ordinates would be a mammoth
task, so you may well decide to continue using low-res co-ordinates for your current
game, and then start your next game with native hi-res co-ordinates. It's up to you.

Setup has changed, I can't select the resolution any more!
==========================================================

Yes, as part of the support for native co-ordinates, you are no longer able to run
a 320x200 game at 640x400, or vice versa.

If you are developing a 320x200 game and were using the 640x400 setting to enlarge the
window, you can enable the 2x or 3x graphics filter in Setup instead, which provides a
similar result.

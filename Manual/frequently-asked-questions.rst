.. index::
   FAQs

Frequently Asked Questions
##########################

This section of the manual is very rarely updated. Please consult
the AGS Forums on the website -- in particular, the Beginners Technical
Forum has an excellent Beginners FAQ (the "BFAQ") which is much more
extensive and is updated regularly with all sorts of Q&A's.

Q. What's the deal with the license? What does it mean in plain English?
========================================================================

A. Adventure Game Studio is pretty much freeware. That means you may use
   it freely for non-commercial games, and you don't have to send me anything
   in return.
   There is one requirement - if you want to SELL a game you make, for profit,
   then you must contact me beforehand as there are some licensing issues
   which you may need to be aware of.
   When you finish your game, please do post it on the Announcements
   Forum and the Games Database so that everyone can give it a go.

Q. Why the swapware license? Why aren't you charging for it?
============================================================

A. Because I don't want to. There's no need to be suspicious, the fact that it's
   free doesn't necessarily mean that it's rubbish.

\section*{Q. On my screen, I can't move the main character. Wherever I click to move him,
   he just stands there.}

A. If the main character isn't on a walkable area, he will not be able to move.
   Load the room in the editor, and check that the location where the
   character starts is on a walkable area.

Q. When I enter a certain room, I just get a black screen.
==========================================================

A. Make sure that you haven't used a Display Message command in the "Enters
   room before fade-in" event for that room. Remember that this event happens BEFORE
   the screen fades in.

   To make sure, when you get the black screen, try pressing enter, or clicking
   the left mouse button. If nothing happens then something more serious may
   have happened. If this is the case, press Alt+X, which should exit the
   program and allow you to trace which line of script it has stopped on.

Q. The character isn't drawn behind my walk-behind areas!
=========================================================

A. You need to define the base line for the area, or he will always be drawn
   in front. See the tutorial for more information.

Q. My game EXE file seems to have disappeared.
==============================================

A. Because this file is your entire game, including the room files, when you
   save a room in the Room Editor it will delete the exe file (because the
   room contained in the exe is out of date). To get it back, simply build the
   game again by using the "Build EXE" command on the Build menu.

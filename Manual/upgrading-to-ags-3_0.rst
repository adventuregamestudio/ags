.. _UpgradeTo30:

Upgrading to AGS 3.0
####################

If you've used AGS before, you'll probably be a bit confused and perhaps even
taken aback by this new editor. But don't worry, once you've got used to it
I'm sure you'll agree that it's a massive improvement over the old one.

The best place to start is probably to flick through :ref:`the tutorial <StartingOff>`,
which has been updated for AGS 3 and by following it through you should
get a good feeling for how the editor works.

The main changes are explained below:

Interaction Editor
==================

The Interaction Editor has been removed from AGS 3.0. When you import your game, AGS
will attempt to convert all your interactions into scripts. These should mostly
just work, however there are a couple of things you should be aware of:

1. *Concurrency issues* -- that is, while a blocking script was running in 2.72
   it was still possible for interactions to run at the same time if they didn't
   include any scripting. Now, that is no longer the case.
2. *Blocking interactions* -- in 2.72, a "Run Dialog" interaction, for example,
   waits until the dialog has finished before moving onto the next action. If you
   had a Run Dialog followed by a Change Room, then the dialog would finish before
   the room change happened.

   With scripting, you need to be careful because the dialog[x].Start() command
   does not block; instead, it waits until the event script has finished running
   before the dialog runs. This would mean that the room change would not
   necessarily happen after the dialog.

   To mitigate these types of problems, you can use an alternate solution such
   as a *run-script* command at the end of the dialog to run the room change.


Global Messages
===============

Global Messages are no longer supported and should be considered obsolete --
there's really no need for them now that the interaction editor has gone.
Any global messages that you had will be retained and will still work,
however the AGS 3 editor provides no way to edit them. If you need to change
one, just replace the ``DisplayMessage`` command with a normal ``Display()``
command.

As of 3.0.2, there is now a Global Variables editor in which you can create global
string variables, which provide all the functionality of Global Messages and more.

Saving and testing your game
============================

AGS 3.0 works slightly differently to previous versions in the way that saving
and testing games works. The Save option (Ctrl+S) is the equivalent of the old
"Quick Save" option -- it will save your changes, but not compile the EXE file.

The "Test game" option has become "Run" on the Build menu. This runs your game
with the new :ref:`Script Debugger <Debuggingfeatures>`, which allows you to pause
and step through the script in order to track down problems.

The debugger always runs your game in a window, so if you want to test it out
full-screen there's the "Run without debugger" option (Ctrl+F5) to do so.

Use the "Build EXE" command (F7) when you want to create all the compiled files
to distribute your game.

*NOTE:* When you use the Run command, your game EXE file does **NOT** get built.
AGS 3's Run command is much faster than the old Test Game command because
it directly loads the files from the game folder instead. If you want to
build your game EXE, you need to use the "Build EXE" command to do so.

RawDraw scripting changes
=========================

The RawDraw family of functions have finally been object-ised, and the old versions
are now obsolete. Support has been added to allow you to RawDraw onto dynamic sprites
as well as room backgrounds. As a result, at first the new commands will seem more
complicated than the old ones, since you can no longer just do "RawDrawImage" to
draw something onto the room background.

Instead, there is a new :ref:`DrawingSurface <DrawingSurfaceFunctions>` object which
you do the drawing onto. You get one of these by calling
:ref:`DynamicSprite.GetDrawingSurface <DynamicSprite.GetDrawingSurface>` or
:ref:`Room.GetDrawingSurfaceForBackground <Room.GetDrawingSurfaceForBackground>`,
depending on what you want to draw onto; and you can then use the various drawing surface
methods to do your drawing.

You must call :ref:`Release <DrawingSurface.Release>` on the surface once you have
finished drawing onto it, which tells AGS to update the data in memory.

For examples, see the :ref:`DrawingSurface <DrawingSurfaceFunctions>` function
help pages.

Other script changes
====================

Scripting hasn't really changed in AGS 3. Two new features (extender functions
and dynamic arrays) have been added, but you shouldn't need to change any
existing code.

The only breaking changes are as follows:

1. ``new`` is now a reserved word. This means that if you had any variables
   called "new" then your script will fail to compile. Just rename the variable
   to something else.
2. Because of the removal of some system limits, some of the AGS_MAX\_ constants
   have been removed (since there is no sensible value for them now that the limits
   have gone). This will probably only affect module authors and is unlikely to
   affect your game. Specifically, the following have been removed: AGS_MAX_GUIS,
   AGS_MAX_CHARACTERS, AGS_MAX_VIEWS, AGS_MAX_LOOPS_PER_VIEW, AGS_MAX_FRAMES_PER_LOOP.

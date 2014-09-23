Tutorial
########

This section will introduce you to the AGS by leading you through how to
create a simple game.


.. _StartingOff:

Starting off
============

The tutorial has now been updated for AGS 3.0. Follow the links below
to run through it.


* :download:`Creating the game <htmlfiles/acintro1.htm>`
* :download:`Creating your first room <htmlfiles/acintro2.htm>`
* :download:`Hotspots and events <htmlfiles/acintro3.htm>`
* :download:`Objects <htmlfiles/acintro4.htm>`
* :download:`Managing inventory <htmlfiles/acintro5.htm>`
* :download:`Using your own graphics <htmlfiles/acintro6.htm>`
* :download:`Animations and cutscenes <htmlfiles/acintro7.htm>`
* :download:`Conversations <htmlfiles/acintro8.htm>`
* :download:`Cursors and fonts <htmlfiles/acintro9.htm>`


For the latest version of the tutorials, always check the AGS website.


.. _Settingupthegame:

Setting up the game
===================

Now that you know how to create a room, it's time to set up the game-wide
settings. These include inventory items, sprite graphics, palette setup
and other things which do not depend on individual rooms.


.. _PalSetup:

Palette setup
-------------

The first thing you need to do when you create a new game is to decide whether
you want to use 8-bit (palette-based) colour or 16-bit (hi-colour).
If you want to use 16-bit colour, you can still use 256-colour backgrounds and
sprites if you want to, but the engine will only run in a 16-bit colour
resolution, thus slowing it down.

If you want to use 8-bit (because it runs faster), you need to set up the
palette. This is because all sprite and background scene imports rely on the
palette setup to be the same. You **CANNOT** use hi-colour sprites or backgrounds
in a 256-colour game.

You set your chosen colour depth by opening the General Settings pane and
adjusting the Colour Depth setting near the top of the list.

Now, choose the "Colours" pane. Here you will see the 256-colour
palette displayed in a grid. Most of the slots are marked "X" - these are the
slots reserved for the background pictures, and will be different in each
room. The other colours will be as they look here for the entire game. These
fixed colours allow things like the main character graphics, which must be
displayed on more than one screen, to work.

If you want, you can assign more or less colours to the backgrounds. To toggle
the background assignment on/off, click on the slot, then check the
"This colour is room-dependant" box to swap the slot's status.

**IMPORTANT:** You must set up the palette as you want it before you start
making your game - if you change it later, you will have to re-import all the
sprites and background scenes.

You can select multiple colour slots by clicking on the first slot, then
shift-clicking on the last slot in the range you want to select. You can then
toggle the background status of all the selected slots at once.

You can right-click in the palette grid to export the entire palette to
a .PAL or PCX file which you can then use to read back into the Editor in
a different game.
If you choose to export to a pcx file, then a screen shot of the Palette Editor will
be saved as the picture. This way you can see all the game-wide colours in
the file.

The "Replace palette" option replaces the palette entries with those
entries from the PAL or PCX file you choose. It can read standard 768-byte
PAL files, SCI palette resources (renamed to extension .pal) and JASC PSP
palette files.

Inventory
---------

Most adventure games allow the player to carry a set of objects, which he can
then use to solve puzzles. Adventure Game Studio makes this inventory easy
for you to manage.

Every inventory item which the player may carry during the game at one time
or another is listed under the "Inventory items" node. Here, each item
has a number and a script name which you use in scripts to identify the object.
To create a new item, right-click on the "Inventory items" node.

Double-click on an inventory item to open it up. On the
left you'll see the graphic used for the object in the inventory window. To change
this, select the "Image" entry in the property grid on the right, and click the "..."
button.

The last thing to do with the inventory items is to define their events:
what happens when the player manipulates them in the inventory window. Click
the "Events" button (the lightning bolt button at the top of the property grid),
which brings up a list which works identically to the hotspot events.
The available events are described in the reference section.

*NOTE:* Each character in the game carries their own set of inventory items.
This means, if you want to create a game like Day of the Tentacle, where the
player can control three different characters, each character will have a
separate inventory.

You have two choices about how the inventory is displayed to the player -- a
built-in inventory window to get you started, and support for custom inventory
windows when you're ready to make your own.

The default option is the Sierra-style pop-up inventory window, which is
popped up by clicking on the Inventory icon on the icon bar. You can also have
the current inventory item displayed in its own button on the icon bar by creating
a button on the GUI and setting its text to  (INV)  which stretches the item
picture to the button size, or  (INVNS)  which draws the inventory item
picture straight onto the button with no resizing. Finally, (INVSHR) , probably
the best option, will draw it at actual size if it will fit, or shrink it if not.

The other option is a custom inventory window. To use this, you
will need to edit the GUI to add it, so I will explain this later on.
While you are starting off with AGS, it is recommended to use the supplied
standard Sierra-style inventory window.

Finally, you may have noticed a "Hotspot Marker Settings" frame at the top of the
Inventory pane. This allows you to switch on an option so that when the
selects an inventory item, the mouse cursor for it will have a dot and mini-crosshair
drawn on it, to show the player where the hotspot is.
You can enter the colour for the centre dot and also for the surrounding 4 pixels.


.. index::
   Alpha blended sprites

Importing your own sprite graphics
----------------------------------

When you were choosing the graphics for the object earlier in this tutorial,
you probably noticed that most of the graphics available didn't look up to
much. This is no problem, because you can import your own graphics using
the Sprite Manager.

Go to the **Sprites** pane in the editor. Here, you will see
the complete sprite set for the game. There are two ways to import your
graphics - either overwrite an existing slot with your graphic, or
create a new slot for it.

To overwrite an existing sprite, right-click the sprite and select "Replace sprite
from file". To import a new slot, right-click on the background to the window
and choose "Import new sprite".

The graphic you choose to import must be at the same colour depth as your game
(ie. if you are using hi-colour backgrounds, your sprites must be hi-colour,
and vice versa). AGS will attempt to convert the image if possible, but if
your game is 256-colour then the results of downgrading a hi-colour image
can be poor.

Then, the Import Sprite window will appear. Here, you need to decide which portion
of the image will be imported. You do this by right-clicking and dragging in the
image, which will produce a yellow rectangle showing the selection. Once you are
happy with it, left-click to import.
Alternatively, you can import the entire image with the "Import whole image" button.

*NOTE (256-colour only):*
You may well find that the colours on your graphic look slightly strange in
the AGS Editor. This is because the sprites are only allocated, by default,
the first 41 of the palette colours (see the :ref:`palette section <PalSetup>`), so
your graphic will be remapped to this much smaller palette. If you find that
many of your imported sprites look strange, you can increase the number of
colours assigned to sprites, at the expense of background colours (again see
the section above for information on how to do this).

If your sprite will only be used in one room then alternatively you can
use the "use background palette" option, which will remap your graphic to
the palette of the room currently loaded, giving much better results. Note,
however, that if you do this, and then try and use the sprite on another
screen, its colours will most likely be screwed up. To use the room palette,
check the "use bkgrnd pal" check-box. Make sure to un-check this box before
you import any other sprites.

*NOTE:* The transparent colour used by AGS is palette index 0 (for 256-colour
sprites) and RGB (255,0,255) for hi-colour. Any pixels you draw on imported
sprites in these colours will be transparent.

You can group imported sprites into folders. This prevents the main sprite
list from becoming too long. By default, the Sprite Manager displays the
Main folder, which contains some graphics and a sub-folder called "Defaults".
Folders work the same way as Windows folders. Right-click on a folder in the
tree to rename it or make a sub-folder.

You can delete a folder by right-clicking on it and selecting the "Delete"
option; beware though that **this will also delete all the sprites in the folder**.

*NOTE:* A few people have experienced problems when importing from clipboard,
in that the image colours get reversed (red becomes blue, blue becomes red, and so on)
when they are running Windows at 24-bit or 32-bit colour. If this happens to you, there
are two solutions: (a) turn down your desktop colour depth to 16-bit to run the AGS Editor,
or (b) import your sprites from files rather than the clipboard.

Tiled sprite import
...................

You may have noticed a checkbox called "Tiled sprite import". Some people find
this a useful way of importing many frames of a character's animation at once.

In order for this to work, you need to have all your sprites lined up on your
source bitmap at even intervals. Then, use the "Import from file" option and import it
as usual. Check the "Tiled sprite import" box, and select the upper-left frame.

When you click the left mouse button, the selection rectangle will become un-filled
and now you can drag the mouse to define how many frames to import - they'll all
be enclosed by selection rectangles. Once you have the correct number, click the left
button again and they will all be imported.

Alpha blended sprites
.....................

AGS supports alpha blended sprites if your game is 32-bit colour. In this case, you
need to import a PNG image with an alpha channel (you cannot paste alpha-blended
images from the clipboard).

When you do so, AGS will prompt you asking whether you want to use the image's alpha
channel or not. If you select Yes, then the sprite will be drawn alpha blended in
the game if it is used for a character, object, mouse cursor or GUI.

Note that if you use alpha blending, any overall transparency that you set (such
as Character.Transparency, Object.Transparency, GUI.Transparency) will be ignored.

*NOTE:* Currently, alpha blended sprites cannot be antialiased, so if you have
the Anti Alias Sprites option turned on in Setup, it will not be applied to alpha-blended
characters.


.. index::
   Cutscenes

Introduction sequences
----------------------

You can easily add intro, outro and cutscene sequences to your game. There
is no specific function to do these, but using the provided animation and
script commands you can create almost anything you might need.

Normally, the game will start in room 1. This is defined by the starting room
number of the player character. To change it, open up the player character's
Character pane, and change the StartingRoom number in the property grid.

*TIP:* The starting room facility is also useful when testing your game - you
can make the game start in any room, at the point where you are testing it,
rather than having to keep playing the game through to get there.

Cutscenes are created using the normal animation script commands, such as
Character.Walk, Object.SetView, and so forth. I would suggest you leave this
until you are more comfortable with AGS, and have some experience of how
to use these functions.


.. _Views:

.. index::
   Views

Animations
----------

In most games you will use some sort of animation during the game, whether
it be a flag waving in the breeze or the player bending over to pick something
up. The term "animation" refers to the ability to change the look of, and
move, objects.

Animations in AGS are managed using Views. A "view" is a set of one or more
"loops". A loop is a set of frames which, when put together, give the effect
of movement. Each frame in the view can be set a graphic and a speed.

Go to the editor's "Views" node, right-click it and select the "New view"
option to create us a new, empty view. Double-click the new view to open it.
Each loop is displayed horizontally with its number at the left hand side,
frames going out to the right. To add a frame, click the grey "New frame"
button. To delete a frame, right-click it.

To change a frame's graphic, double-left-click it. The sprite list screen
will be displayed (you may remember this from the Object graphic selection)
where you can choose the graphic you want to use for this frame.

Note that for walking animations, the first frame in each loop is reserved for
the standing frame, and when walking it will only cycle through from the second
frame onwards.

You select a frame by left-clicking it -- when you do so, the property grid
will update with information about the frame. One of these settings is
called "Delay", which is the frame's **relative** speed. The larger the number,
the longer the frame stays (ie. the slower it is). When the animation is run,
an overall animation speed will be set, so the actual speed of the frame
will be:  overall_speed + frame_speed  . Note that you can use negative
numbers for the frame delay to make it particularly fast, for example setting
it to -3 means that the frame will stay for hardly any time at all.

Animation speed is specified in Game Loops (ie. animation speed 4 will show the
frame for 4 game loops - at 40fps, that would be 0.1 seconds).

The "Sound" property allows you to enter a sound number that will be played
when this frame becomes visible on the screen.
This is especially useful for footstep sounds.

You run an animation by using the script animation commands, which will
be explained in detail later. Briefly, to animate an object, you first
of all need to set the object's view to the correct view number (use
the Object.SetView script command), and then use the Object.Animate
script command to actually start the animation.


.. index::
   Idle animations

Characters
----------

A character is similar to an object, except that it can change rooms,
maintain its own inventory, and take part in conversations (more on
these later). It can also have its own custom animation speed and movement
speed.

Go to the "Characters" node in the main tree. You will see under it a
list of all the characters in the game.
To create a new character, right-click the "Characters" node and
choose the "New character" option.

You will see that there are a lot of options which you can set for each
character. The most immediately obvious one is the "Make this the player character"
button, which allows you to change which character the player will control
at the start of the game.
When the game starts, the first room loaded will be this character's
starting room.

The rest of the options are hidden away in the property grid on the right. Some
of them are described below:

The "UseRoomAreaScaling" option allows you to specify whether this
character will be stretched or shrunk in scaling areas of the screen. You might
want to disable this if you have a character who always stands still in the same
place, and you want the graphics on-screen to be the same size as you drew
them, even though he is standing on a scaled area.

The "Clickable" option tells AGS whether you want the player to be able
to click on the character. If Clickable is enabled, then the character will
be interactable, like the way things worked in Sierra games. If it is not
enabled then the character works like the main character did in Lucasarts games -
if you move the cursor over him or click to look, speak, etc, then the game will
ignore the character and respond to whatever is behind him.

To set which room this character starts in, change the "StartingRoom"
property. You can set the character's location within this room by using
the "StartX" and "StartY" properties to type in the X,Y co-ordinates you want him
to start at. These co-ordinates define where the middle of his feet will be placed.

The "NormalView" is where you set what the character looks like. You must
create a view in the :ref:`View Editor <Views>`, and this
view must have either 4 or 8 loops. If you use 4 loops, then when walking
diagonally the closest straight direction is used for the graphics. Each loop
is used for the character walking in one direction, as follows:

* *Loop 0* -- walking down (towards screen)
* *Loop 1* -- walking left
* *Loop 2* -- walking right
* *Loop 3* -- walking up (away from screen)
* *Loop 4* -- walking diagonally down-right
* *Loop 5* -- walking diagonally up-right
* *Loop 6* -- walking diagonally down-left
* *Loop 7* -- walking diagonally up-left

To change the rate at which the character animates, change the Animation Speed box.
Here, a smaller number means faster animation. Note that this does NOT
effect the speed at which the character actually moves when walking.

*NOTE:* The first frame in each loop is the standing still frame. When walking, the
game will cycle through the rest of the frames in the loop.

The "MovementSpeed" option allows you to control how fast the character moves when
walking. Here, a larger number means he walks faster. If you find that a movement
speed of 1 is still too fast, you can use negative numbers (eg. -3) which will move
even more slowly. The lower you go, the slower the movement speed.

The "SpeechColor" option specifies which colour is used for the text when
this character is talking. It effects all messages that are said by this
character. You can find out the colour for each number by going
to the "Colours" pane.

The "IdleView" option allows you to set an idle animation for the character.
To do this, create a new view, with one or more loops of the character idle
(eg. smoking, reading a book, etc). Then, set the "Idle view" to this view number.
If the player stands still for 20 seconds (you can change the timeout with
the Character.SetIdleView script function), then the current loop
from the idle view will be played.

The "ScriptName" property sets the name by which the character will be
referred to in scripts and in conversation scripting.
The difference from the RealName of the character is that the script name
may only contain letters A-Z and numbers 0-9 (the first character must be
a letter, however). The convention in AGS is that character script names
start with a lower case "c".

To set what happens when the player interacts with the character, click the
"Events" button (this is the lightning bolt button at the top of the property
grid). You will be presented with the events list; select an event and
press the "..." button to allow you to enter some script to handle the event.

You can also set a talking view for the character. To set one, use the
"SpeechView" property. If you set a talking view, then that view will be
used to animate the character while they are speaking. You should generally
have about 2-3 frames in each loop (the loops are used for
the same directions as in the main view).

There is also an available "Blinking view". This is used to play intermittent extra
animations while the character is talking. You may want to use this for effects
such as blinking (hence the name).  If you set a view here, it will play intermittently
while the character talks (it is drawn on top of the normal talking view). The default
time between it playing is 3-4 seconds, but you can change this with the Character.BlinkInterval
script property.

*NOTE*: the blinking view is currently only supported with sierra-style speech.

"UseRoomAreaLighting" allows you to tell AGS whether this character will be
affected by light and tint levels set on room regions.

If you disable "TurnBeforeWalking", it will override the General Setting for
turning and tell AGS not to turn this particular character around on the
spot before they move.

"Diagonal loops" specifies that loops 4-8 of the character's view will be used for
the four diagonal directions. If this option is not enabled, the character will only
face 4 ways, and you can use loops 4-8 for other purposes.

"Adjust speed with scaling" modifies the character's walking speed in line with their
zoom level, as set on the walkable areas.

"Adjust volume with scaling" modifies the volume of any frame-linked sounds on the
character's view (eg. footstep sounds) with their zoom level, as set on the walkable areas.

"Solid" specifies that this character is solid and will block other characters from
walking through it. Note that **both** characters must be solid in order for them
to block one another.

AGS allows you to export your characters to a file, and then import the file into a
different game - so you can share the same main character between games, or
create one for distribution on the internet. Right-click on the character and
choose "Export character". The entire character setup and graphics will be exported
to the file, including the character's walking and talking animations.
To import the character into a different game, load it up, right-click the "Characters"
node and choose "Import Character". The file selector appears, where you find the CHA file which
you exported earlier. A new character slot will be created and all the settings imported.

*NOTE:* Because importing always creates a new slot, you cannot use it to
overwrite an existing character.


.. index::
   Dialogs
   run-script
   add-inv
   goto-dialog
   new-room
   option-off
   option-on
   option-off-forever
   play-sound
   return

Conversations
-------------

While the old Sierra games were mainly based on action and not talking, the
Lucasarts games took the opposite approach.

If you want to create a game with conversations where the player can choose
from a list of optional topics to talk about, you can now with the new Dialog
Editor. Go to the "Dialogs" node.

Conversations are made up of Topics. A "topic" is a list of choices from
which the player can choose. You may have up to 30 choices in a topic.
However, not all of them need to be available to the player at the start of the
game - you can enable various options for conversation once the player has
said or done other things. For example, when you talk to the man in the demo
game, the first option is just "Hi". Once he has said this, however, a new
option becomes available.

The Dialog Editor is quite self-explanatory. Double-click a dialog topic
to open up its window. You'll see the list of options for the topic on
the left, and the dialog script on the right. Each option
has a couple of checkboxes to its right:

* The "Show" column specifies whether that option is available to the player
  at the start of the game.
* The "Say" column defines whether the character
  says the option when the player clicks it. The default is on, but if you
  want options describing the player's actions rather than the actual words,
  you may want to turn this column off for that dialog.


Dialog scripts
..............

You control what happens when the player chooses an option by editing
the script on the right. This is called the **dialog script**, and is a
simplified version of scripting streamlined for conversations.

With a newly created dialog topic, all you will see in the script is a number
of lines starting with an '@' symbol. In the dialog script, these signify the starting points of
the script for each option. For example, when the player clicks on option 3,
the script will begin on the line following "@3". There is also a special
starting point, called "@S". This is run when the conversation starts, before
any choices are given to the player. This could be used to display a "Hello"
message or something similar.

To display some speech, you begin the line with the character's SCRIPT NAME
(not full name), followed by a colon, then a space, and then what you want
them to say. For example, if my main character's script name is EGO, I would
write

::

   ego: "I am very happy today because it's my birthday."

The character name is used by the system to choose the correct colour for
the text.

**IMPORTANT:** Do **NOT** include the "c" at the start of the character's
script name here.

You can also use the special character name "narrator", which
displays the text in the pop-up message box instead of as speech text; and the
alias "player", which will say it as the current player character - useful if
you don't know which character the player will be controlling when they speak
the conversation.

If you just use ``...`` as the text for a character to say, the game will
pause briefly as if they are stopping to think, and nothing will be displayed.

To signal the end of the script for this option, place a "return" command on
the last line of it. For example,

::

   @1
   ego: "Hello. How are you?"
   narrator: The man looks you in the eye.
   otherman: ...
   otherman: "I'm fine."
   return

"return" tells AGS to go back and display the choices again to the player.
If you use "stop" instead of return, then the conversation is ended. Alternatively,
you can use "goto-dialog" or "goto-previous", which abort the current dialog script
and transfer control to the new dialog.

*NOTE:* Do **NOT** indent these lines with spaces or tabs. Indented lines
signify that AGS should interpret the line as a normal scripting command
rather than a dialog scripting command.

The dialog commands available are:

* *goto-dialog X* -- Switches the current topic to Topic X, and displays the current list of
  choices for that topic.
* *goto-previous* -- Returns to the previous topic that this one was called from. If the dialog
  started on this topic, then the dialog will be stopped.
* *option-off X* -- Turns option X for the current topic off, meaning it won't be displayed in
  the list of choices next time.
* *option-off-forever X* -- Turns option X off permanently. It will never again be displayed, not even
  if an "option-on" command is used.
* *option-on X* -- Turns option X for the current topic on, including it in the list of choices
  to the player next time they are displayed.
* *return* -- Stops the script and returns to the list of choices.
* *stop* -- Stops the conversation and returns the player to the game.

For an example of a dialog script, load the demo game into the editor and
look at the script for its topic 0.

Using scripting commands in dialogs
...................................

Often the provided dialog scripting commands won't be enough for what you want
to do in the dialog. You might want to give the player an inventory item or
add some points to their score, for example.

AGS now lets you put normal scripting commands in your dialog script, by indenting
the line with spaces or tabs. For example::

   @1
   ego: "Hello. How are you?"
   narrator: The man looks you in the eye.
     player.AddInventory(iKey);
     Display("This line is displayed from a normal script command");
   otherman: "I'm fine."
   return

Here, you can see dialog script commands being used, but also then a
couple of normal scripting commands have been inserted, on indented lines.

When working with dialog scripts, the **this** keyword allows you to access
the currently running dialog.

If you want to conditionally break out of the dialog
script, the special tokens ``RUN_DIALOG_GOTO_PREVIOUS``, ``RUN_DIALOG_RETURN``
and ``RUN_DIALOG_STOP_DIALOG`` are available which you can ``return`` from inside
a script block. For example::

   @1
   ego: "Hello. How are you?"
   narrator: The man looks you in the eye.
     if (player.HasInventory(iKey)) {
       player.Say("Actually, I'd better go.");
       return RUN_DIALOG_STOP_DIALOG;
     }
   otherman: "Here's a key for you."
   return


\subsubsection*f{Parser input}

You'll notice in the dialog editor, the property grid has an option called "ShowTextParser".
If you enable this, a text box will be displayed below the predefined options in the game,
which allows the player to type in their own input.

If they type in something themselves, then the dialog_request global script function
will be run, with its parameter being the dialog topic number that the player was in.

AGS automatically calls ParseText with the text they typed in before it calls dialog_request,
so you can use Said() calls to respond. See the :ref:`text parser <TextParser>` section
for more info.


.. index::
   options
   Anti-aliasing fonts
   Crossfading music

Game options
------------

The Game Settings pane contains a list of all the various overall options
that you can set for your game.

Note that some things listed here are explained later in the documentation,
so if you don't understand one of the items in this list, come back to it
later.

Most of these options can be changed at runtime with the script command SetGameOption.

* *Debug Mode* -- whether the debug keys are active. When debug mode is on,
  you can press Ctrl-X to teleport to any room, Ctrl-S to give all inventory
  items, Ctrl-A to display walkable areas on the screen, and Ctrl-D to display
  statistics about the current room. When debug mode is off, these do nothing.
  See the :ref:`Debugging features <Debuggingfeatures>` section for more.
* *Play sound on score* -- controls whether a sound effect is played when
  the player scores points. If so, you can set the sound number, which will
  play SOUNDx.WAV (or SOUNDx.MP3), where X is the number you set.
* *Walk to hotspot in Look mode* -- controls whether the player will walk
  to "walk-to" spots when the player looks at the hotspot. Normally he only
  walks on use, speak and use-inv.
* *Dialog options on GUI* -- controls where the player's options for dialog are
  displayed. If this option is not checked, then in a conversation, the options
  will be displayed at the bottom of the screen. If you check this box, then
  instead the options will be displayed on the GUI you specify.
* *Use "anti-glide" mode* -- you may notice that, as the character walks, it
  can seem as if he is gliding, especially if you have a slow animation speed
  setting. When anti-glide mode is on, the man's position is only updated
  when the frame of animation changes. You will need to increase each
  character's walking speed if you use this option.
* *Text windows use GUI* -- allows you to customize the standard text window
  appearance in the game, using the specified interface element. See :ref:`here <TextWin>`
  for more information.
* *Pixel gap between options* -- defines the gap between the options displayed
  to the player in a conversation. Normally this is 0, which means the
  options are right below each other. Changing it to 1 or 2 can make the
  option display look less cluttered; it's a matter of personal preference.
* *Skip Speech* -- determines how and whether the player can skip speech
  in-game. This can be set to allow the mouse and/or keyboard, or neither, to
  skip speech in the game.
* *When interface disabled* -- determines what happens to buttons on your
  GUIs while the game interface is disabled (eg. during a cutscene).
* *GUI alpha rendering style* -- determines which rendering method to use in
  32-bit games when a GUI Control is drawn over GUI. The "Proper alpha blending"
  choice is meant for full alpha blending support, other options exist for
  compatibility with older versions of AGS only.
* *Sprite alpha rendering style* -- determines which rendering method to use
  in 32-bit games when an image is drawn over :ref:`drawing surface <DrawingSurfaceFunctions>`.
  The "Proper alpha blending" choice is meant for full alpha blending support,
  "Classic" style exists for  compatibility with older versions of AGS only.
* *Always display text as speech* -- if you select this option, then all normal
  text in the game will be displayed above the main character's head as speech
  text, much like the way the Lucasarts games worked. If this option is not
  checked, then normal text appears in a pop-up message box, like the way that
  the Sierra games worked.
* *Speech style* -- in the default Lucasarts-style speech, when a character talks, the
  speech text is displayed above their head in the game, and the character's
  talking view is used to animate the actual character.

  However, if you set this option to Sierra-style then the talking view is used to display an
  animating portrait separately in the top-left of the screen, with the text to the right of it.
  This is similar to the way that Space Quest 5, King's Quest 6 and other
  later Sierra games worked. You can also cycle to another option, "Sierra-
  style with background", which is the same except a text window is drawn
  behind the speech text to make it easier to read.

  "Whole Screen" uses a full-screen character portrait, like the way that QFG4 worked.
* *Speech portrait side* -- if you're using Sierra-style speech, then this
  determines whether the portrait appears on the left or the right of the screen. The "alternate"
  setting means it swaps sides whenever a different person talks, and the "Based on X position"
  setting means that the side of the screen is chosen depending on where the characters are
  standing.
* *Room transition style* -- defines what type of screen transition is used when
  moving from one room to another. Various options are available.
* *Save screenshots in save games* -- Saves a mini-screenshot of the player's current
  position into the save game file. This will create larger save game files, but it will
  mean that you can use a save game thumbnails GUI to make the save/load interface
  more professional.
* *Enforce object-based scripting* -- Puts the script compiler into strict mode,
  where it will not accept the old-style (pre-AGS 2.7) script commands. This should
  preferably be ticked, since you should no longer be using the old commands.
* *Left-to-right operator precedence* -- if this is ticked, then operators of
  equal precedence in the script will be evaluated left to right. For example,
  5 - 4 - 3 could be interpreted as  (5 - 4) - 3  or as  5 - (4 - 3), thus giving
  different results. You should always use parenthesis to clarify expressions like this,
  so that the operator precedence doesn't affect the result.
* *Pixel-perfect click detection* -- normally, when the player clicks the
  mouse, AGS just checks to see if the cursor is within the rectangular area
  of each character and object on the screen. However, if this option is
  checked, then it will further check whether the player clicked on an actual
  pixel of the object graphic, or whether it was a transparent part of the
  graphic. If this option is enabled and they click on a transparent pixel,
  then the hotspot behind the object will be activated instead.
* *Don't automatically move character in Walk mode* -- normally, when you click the mouse in
  the Walk mode, the main character will move to where you clicked. However,
  if you want to create a game all viewed from a 1st-person perspective, and
  so don't have a main character, then selecting this option allows you to
  use the Walk mode for other things. If selected, then "Character stands on
  hotspot" events are instead triggered by clicking the Walk cursor on the
  hotspot.
* *Don't use inventory graphics as cursors* -- normally, when you select an inventory
  item the mouse cursor is changed into that item. However, if you want to
  create a Lucasarts-style game (where the inventory cursor is always a
  cross-hair), check this option and it won't be changed.
* *Don't scale up fonts at 640x400* -- normally, if the player chooses 640x400, then
  the fonts will be scaled up to match. However, if you have drawn your fonts
  for the 640x400 resolution, use this option to stop them being stretched.
* *Resources split every Mb* -- see :ref:`here <SplitRes>` for information.
* *Characters turn before walking* -- specifies that when a character starts
  to walk somewhere, it will first turn round to face the correct direction
  using available animation frames, rather than just suddenly switching to
  face the right way.
* *Override built-in inventory window click handling* -- AGS has some built-in processing
  of Inventory Window GUI controls, whereby a right-click will Look at the item, and a left click
  will select it if the cursor mode is Interact. However, if you enable this
  option, then clicking on an inventory item in an Inventory Window will call your ``on_mouse_click``
  function  with eMouseLeftInv, eMouseMiddleInv or eMouseRightInv, and you then need to process it
  yourself. You can use the ``game.inv_activated`` variable to find out what they clicked on.
* *Enable mouse wheel support* -- if enabled, on_mouse_click can be called with
  the values eMouseWheelNorth and eMouseWheelSouth, which signify the user scrolling their mouse
  wheel north or south, respectively.

  *NOTE:* Not all mice have mouse wheels, and the DOS engine does not support the
  mouse wheel at all. Therefore, your game should never require the mouse wheel in order to
  be playable - it should only be used as a handy extra.
* *Number dialog options* -- adds an index number before each dialog option when
  they are displayed to the player. For example::

     1. Hello there!
     2. Goodbye

  This allows you to visually show the player which option the shortcut keys will choose,
  as well as separating the options if you don't use a bullet point.
* *Dialog options go upwards on GUI* -- Normally, if you select a non-textwindow GUI
  for the dialog options, they will be printed from the top down. However, if you select this
  option they will go from the bottom of the GUI upwards.
* *Crossfade music tracks* -- This allows you to tell AGS to crossfade between your
  background music tracks. Crossfading means fading out the old track while fading in the
  new one when the music changes. You can select a crossfade speed from the drop-down list.
  There are some disadvantages to using this option - firstly, it's fairly slow, since AGS
  has to decode two music files at once. Secondly, it only works with OGG, MP3 and WAV music.
  You cannot crossfade MIDI, XM, MOD, S3M or IT music.
* *Anti-alias TTF fonts* -- If enabled, any TTF fonts you have in your game will
  be rendered to the screen anti-aliased. This can make them look a lot better, but it has
  two drawbacks - firstly, anti-aliasing is significantly slower than normal rendering, so
  you might want an option to allow the player to turn it off. Second, anti-aliasing only
  works in hi-colour games (in 256-colour games, the output will look blurred and unreadable).

  *NOTE* that anti-aliasing is not currently done on lucasarts-style speech due to
  technical reasons.
* *Thought uses bubble GUI* -- Determines which text window GUI is used for displaying
  thoughts with :ref:`Think <Character.Think>`.
* *Characters turn to face direction* -- if set, then when a character turns round
  with the :ref:`Character.FaceLocation <Character.FaceLocation>` or
  :ref:`Character.FaceCharacter <Character.FaceCharacter>`
  script commands, they will visibly turn around using their available loops.
  If this option is not set, they will immediately appear facing their new direction.
* *Write game text backwards* -- in-game text will be written right-to-left, ie. line
  breaks are worked out from the end of the sentence going backwards, and the last words
  are displayed first. This is used by languages such as Arabic and Hebrew.
* *Display multiple inventory items multiple times* -- normally, if the player has
  two of an inventory item, the item will still only be shown once in the Inventory window.
  If you check this option, however, then all the copies of the item that the player has
  will be displayed. Useful for RPG-style inventories.
* *Save games folder name* -- if this is blank, then the player's saved games will
  be saved to the folder where the game is installed. This is not a good idea, because it
  forces different users on the same machine to share save games, and Windows Vista and later versions discourage
  games from writing to the Program Files folder. Instead, if you supply a folder name here,
  then AGS will automatically create it within the user's Saved Games (Vista and later) or My Documents
  (XP and earlier) folder, and their save games will be saved there.


See Also: :ref:`Enhanced Saved Games <EnhancedSaveGames>`,
:ref:`Windows Game Explorer <GameExplorer>`


.. index::
   Mouse cursors

Cursors
-------

The Cursors node of the editor shows you the current mouse cursor modes available in
the game. Each cursor mode performs a different action within the game. Double-click
one to open it up.

The "StandardMode" option in the property grid tells AGS that this is a 'normal' cursor mode - ie.
using this cursor will fire an event on whatever is clicked on as usual. This
mode applies to the standard Walk, Look, Interact and Talk modes, but you can create
others too. Do not tick it for the Use Inventory mode, since this is a special mode.

The "Animate" option allows you to specify that the mouse cursor will animate while it
is on the screen. Choose a view number, and the cursor will animate using the first
loop of that view. You can make it animate only when over something (hotspot, object
or character) by enabling the "AnimateOnlyOnHotspots" option.

The "AnimateOnlyWhenMoving" box allows you to do a QFG4-style cursor, where it only
animates while the player is moving it around.

Three of the cursor modes are hard-coded special meanings into AGS:

* *Mode 4 (Use Inventory)* -- This is special because the game decides whether to
  allow its use or not, depending on whether the player has an active inventory item
  selected.
* *Mode 6 (Pointer)* -- This cursor is used whenever a modal dialog is displayed
  (ie. a GUI that pauses the game). Normally this is a standard arrow pointer.
* *Mode 7 (Wait)* -- This cursor is used whenever the player cannot control the
  action, for example during a scripted cutscene. For a lucasarts-style game where the cursor
  disappears completely in this state, simply import a blank graphic over the wait cursor.


For the standard modes:

* Mode 0 will cause the player to walk to the mouse pointer location when clicked.
* Modes 1, 2, 3, 5, 8 and 9 will run the event with the same name as the cursor mode.


Fonts
-----

AGS comes with a couple of default fonts, but you can replace the and add your own.
You can use both TrueType (TTF) and SCI fonts (Sierra's font format).

SCI fonts can be created in two ways:

* Extract the font from a Sierra game, using the SCI Decoder program
  available on the internet.
* Create your own font and save it in SCI Font format, using the
  `SCI Graphic Studio program <http://scigraphicstudio.cjb.net/>`_.

There are also some fonts available on the `AGS website <http://www.adventuregamestudio.co.uk/fonts/>`_.

Note that SCI fonts are faster to render than TTF fonts, and so may give your game
a speed advantage. It's preferable to use a SCI font if you can.

Go to the "Fonts" node in the main tree. Here you can see all the current fonts
listed underneath. You can create a new font by right-clicking the "Fonts" node
and choosing "New font". To overwrite an existing font, open it up and
press the "Import over this font" button.

Fonts can have outlines. For lucasarts-style speech, outlines are really a
necessity since they stop the text blending into the background and becoming
un-readable. To outline a font, either set the OutlineStyle to "Automatic" to
have AGS do it for you, or you can use a specific font slot as the outline font
(it will be drawn in black behind the main font when the main font is used).

*NOTE:* If you go to your Windows Fonts folder, you will not be able to select
any fonts to import, since double-clicking them will open them up in the Windows Font
Viewer. Unfortunately there is nothing I can do about this, you must either type the
filename in manually, or copy the font to another folder and import it from there.

*NOTE:* Font 0 is used as the normal text font, and font 1 is used as the
speech font. To use any additional fonts, you can set the Game.NormalFont
and Game.SpeechFont properties in your script.

Advanced room features
======================

This section describes slightly more advanced things you can do with the
rooms.


.. index::
   Scaling

Character scaling
-----------------

AGS supports scaling of characters, where the character can appear to get
smaller as he walks away from the screen. Character scaling is supported as
part of the walkable areas in a room.

The reason why you have multiple colours available for the walkable areas is
because you can set a zoom level for each colour, which defines how large
the character will be while he is in that area. The default for all walkable
areas is ``100%``, ie. full size. However, you can adjust it using the "Walkable
Areas" mode to anywhere from ``10%`` (one-tenth size) to ``200%`` (double size).

The scaling settings can effect all characters and objects in the game. For characters,
it is on by default but you can disable the scaling for an individual character by
setting the "UseRoomAreaScaling" option in that character's properties.

For objects, it is off by default but you can make a specific object obey scaling
levels by setting its "UseRoomAreaScaling" option.

If you set the "UseContinuousScaling" option, then rather than just specifying a
zoom level for the whole walkable area, you specify a min and max zoom level. These
specify the scaling at the top and bottom of the walkable area. When the game is run,
AGS will interpolate these values to make the character smoothly scale down from
one value to another as he walks towards the back or front of the screen.

Scrolling
---------

It's easy to create scrolling rooms like the ones used in Lucasarts games
like Monkey Island (tm) and Day of the Tentacle.

To do this, just import a background scene that is larger than your game resolution.
For example, in a 320x200 game, 500x200 is a good size for Lucasarts-type rooms.

That's all you have to do. Draw on the walkable areas, hotspots and so on, as
normal, and then save the room. The screen will scroll to follow the main character
around.

The script command :ref:`SetViewport <SetViewport>` allows you to manually scroll
the room around if you don't want it to follow the character.


.. index::
   Mask; importing

Importing a file as the walkable area mask
------------------------------------------

AGS has the ability to import an external BMP or PNG file to use as the
walkable-area, hotspot or walk-behind area mask. If you don't like the way
you have to draw these in the editor itself, you can draw them in another
program and then import them. This is also useful if you are converting a
game you were making with another game-creation system into AGS.

To use the feature, click the "Import Mask" button (in the toolbar) in the
relevant mode of the Areas editor. There are some restrictions to how this
file must be drawn: it must be the exact same size as the background scene,
and it must be in 16-colour (4-bit) or 256-colour (8-bit). Then, colour 0
on the bitmap signifies transparency and colours 1-15 are used as the
respective hotspot/walk-behind/walkable area numbers.

**IMPORTANT:** Do NOT use any colour numbers above 15 on the mask bitmap. Use only
palette indexes 0 to 15.


Animating background scenes
---------------------------

If you want to have a lot of animation on the screen, you will come across
two problems if you try to do it using objects:

* There is a limit on the number of objects per screen, so you may not be able to
  animate everything that you want to that way.
* Objects slow down the game - the more objects on the screen, the slower
  the game runs.

The solution to these problems is to use an animating background scene.

How it works is this: Each room can have from 1 to 5 backgrounds. Normally,
each room just has one background. However, you can import up to four extra
backgrounds in each room, and if you do so then the game will cycle through
them, giving the effect of animation.

This gives two main advantages - you can animate the entire screen, and due
to the way the engine works, it doesn't slow down the game at all.

To import a second background for a room, load the room into the editor, pull
down the "Main background" list box, and choose the "Import new background" option.
Choose the file that's storing the background and you're done.

To delete a background, select it then click the "Delete" button.

You define the speed at which the backgrounds will animate by setting the
"BackgroundAnimationDelay" option in the property grid for the room. The default
is 5, which cycles background every 5 frames.

*NOTE:* All the background scenes must be the same size.

*NOTE:* (256-colour only) The backgrounds frames each have their own palette (unless
you select "Share palette with main background" before importing). This means
that when the current frame switches in-game, the palette will get reset - therefore
you can't use special palette effects such as CyclePalette or SetPalRGB on screens
with animating backgrounds.


Lighting effects
----------------

You can control the brightness of your characters, courtesy of the "LightLevel" setting
for room Regions.

By default this is ``100%``, but you can change it from ``0% to 200%``.
This number is the light level in the current walkable area. Smaller
numbers are darker, so that ``0%`` is pitch black and ``200%`` is very bright.

This feature could be useful if, for example, you have a street lamp on your scene so
when the character walks under it they get brighter, or if a wall is shading
the character from the light they can get darker.

You can alternatively use a colour tint for the region. If you select this, then you
can enter Red, Green and Blue values as numbers from 0-255, which reflect the colour
you want the area to be tinted to. The Amount setting determines to what extent
characters will be tinted, and is from 0-100.

*NOTE:* Light levels only work when the character's graphic is at the same
colour depth as the background (ie. a 256-colour character in a hi-colour
game won't get lightened).

*NOTE:* In a 256-colour game, only darkening areas (light level < ``100%``) will work.
Also, depending on the room palette the quality of the darkening will vary
in 256-colour games.

*NOTE:* Light levels affect characters and objects, depending on
the "UseRoomAreaLighting" setting for each one.
They do not affect overlays or the background scene.

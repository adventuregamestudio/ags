Adventure Game Studio runtime PSP port - version 3.21 R1
--------------------------------------------------------

Licensed under the Artistic License 2.0, see License.txt in the Engine folder.


Requirements:
A PSP 2000/3000/GO with a Custom Firmware or Homebrew Enabler that can access the
extended memory. Examples are 5.00 M33, 5.50 GEN, 6.xx PRO, 6.20 TN-HEN.
To run most games you absolutely need the extended memory of the newer PSP models,
so while this will seem to work on the PSP 1000 you will encounter crashing with
larger games.
For the same reason, signing the Eboot is not useful as only the standard memory
is available on official firmwares. It also requires kernel mode if run with the menu.


AGS game compatibility:
This runtime engine port is not compatible with all AGS games. There are the
following restrictions:
- The ONLY supported AGS versions right now are 3.2.0 and 3.2.1.
  You can check the version of AGS a game was made with by examining the properties 
  of the game executable.
  If you try to run a game made with a newer or older version of AGS, you will
  receive an error when trying to launch the game.
- Resolution must be 320x200 or 320x240. No higher resolution is supported and
  you will receive an error stating that when trying to run such a game.
- Games using a lot of memory (large rooms especially) will crash. You will then
  see a "blue screen of death" with debug information. Press X to save them
  to a file. The game will then quit and reopen the menu.
These glitches can appear, affecting the game experience:
- Games which make use of advanced scripting might run slow, possibly making
  them unplayable.
- Sound will stutter in certain instances, e.g. when loading/saving.
- When skipping cutscenes, the PSP may appear to hang for several seconds.
- The framerate drops when several sounds are played at once.


How to run games:
1. Copy the original game folder into the Eboot directory.
   E.g. if you want to run the game "Demo Quest" you put the games files into
   the folder "x:\PSP\GAME\ags\Demo Quest\" (drive letter and PSP game folder
   is dependent on your configuration).
   NOTE: If the game is created with an incompatible engine version, this will
   be indicated in the games list.
2. If you only want to run a single game, put the data files directly into the
   folder with the Eboot and rename the main game executable to "ac2game.dat".
   It will then autostart this game.



The configuration file:

You can customize the behaviour of the runtime by placing a file "psp.cfg" in the
Eboot directory or the directory of the game you want to run. A config file in
the game directory will overwrite the settings from the global configuration file.

Available settings:

[analog_stick]
sensitivity: Defines how the analog stick movement translates to mouse movement. Higher
  values mean that the cursor moves faster.
deadzone: Analog stick input values below this threshold are discarded. This prevents the
  mouse from moving on its own with not properly centering analog sticks.

[button_mapping]
left_trigger, right_trigger, triangle, cross, square, circle, up, down, left, right, start, select:
Specify here what PSP button corresponds to what key/mouse input.

[onscreen_keyboard]
left_trigger, right_trigger, triangle, cross, square, circle, up, down, left, right, start, select:
Specify here what PSP button corresponds to what key/mouse input. Same as above, but only if
the onscreen keyboard is active.

[sound]
enabled: You can disable sound for a game speed up.
threaded: If 1, plays and decodes sound in a separate thread. This reduces sound stuttering.
cache_size: Determines how many sounds the runtime caches to reduce disk access. Set this to 1
  to disable caching.
samplerate: The sound playback sample rate, typical values are 44100, 22050, 11025. Lower values
  mean a slight speedup with the trade-off of reduced sound quality.

[graphics]
scaling: The game's graphics can either be shown unscaled in their original resolution or stretched
  to the full height of the PSP screen.
smoothing: If scaling is enabled, this determines whether a linear interpolation filter is used.
  This reduces scaling artefacts and gives a more uniform appearance but at the same times makes
  the crisp pixel art somewhat blurry.

[misc]
show_fps: If 1, shows the framerate.
disable_power_saving: The PSPs power saving features like dimming the backlight and switching to
  standby are deactivated if this is set.
return_to_menu: If 1, you will reenter the menu instead of going back to the XMB after
  quitting a game. This only works if the AGS game has an option to quit.
ignore_acsetup_cfg_file: If 1, the engine will not read settings from the AGS configuration file.
enable_extra_memory: If 1, there are additional 4 MiB of memory available for the game. But this
  disables suspending the PSP. Also trying to use the pause feature of a Go causes a freeze.
  While enabling certain games to run (especiall on a PSP 1000), this feature is experimental
  and might itself cause instability.


Available button values for mouse input:
mouse_up, mouse_down, mouse_left, mouse_right: Moves the mouse cursor in the indicated direction.
  Note: The analog stick is always used for mouse movement too.
mouse_click_left, mouse_click_right, mouse_click middle: Simulates the respective mouse button.

Available button values for keyboard input:
A ... Z, 0 ... 9, 0_PAD ... 9_PAD, F1 ... F12
ESC, TILDE, MINUS, EQUALS, BACKSPACE, TAB, OPENBRACE, CLOSEBRACE, ENTER, COLON, QUOTE, BACKSLASH,
BACKSLASH2, COMMA, STOP, SLASH, SPACE, INSERT, DEL, HOME, END, PGUP, PGDN, LEFT, RIGHT, UP, DOWN
SLASH_PAD, ASTERISK, MINUS_PAD, PLUS_PAD, DEL_PAD, ENTER_PAD, PRTSCR, PAUSE, ABNT_C1, YEN, KANA, 
CONVERT, NOCONVERT, AT, CIRCUMFLEX, COLON2, KANJI, LSHIFT, RSHIFT, LCONTROL, RCONTROL, ALT, ALTGR,
LWIN, RWIN, MENU, SCRLOCK, NUMLOCK, CAPSLOCK EQUALS_PAD, BACKQUOTE, SEMICOLON, COMMAND

Available button values for the onscreen keyboard:
keyboard_toggle: This button will switch between the standard button set and the one for the osk.
keyboard_enter_char: Presses the currently selected key.
keyboard_next_char: Switches focus to the next key in the current set.
keyboard_previous_char: Switches focus to the previous key in the current set.
keyboard_next_keyset: Switches to the next key set.
keyboard_previous_keyset: Switches to the previous key set.



Compiling the source:

1. Building the AGS port:
   - You need MinPSPW 0.11 for Windows or Linux.
   - You also need the M33 SDK from 4.01 M33 or from PRO CFW.
   - The sources for the AGS port are available from here:
     "http://gitorious.org/~jjs/ags-for-psp"
   - Typing "make" in the "PSP" folder will build all files necessary and put them
     into the "PSP/bin" folder.

2. Building the libraries:
   - Check the file "readme.txt" in the "patches" folder.


Main source code changes from the PC version (also check the commit log of the git repository):
- Instances of unaligned variable access that are not allowed on the MIPS processor
  are replaced with equivalent memcpy calls.
- The PSP framebuffer has a BGR colour order. A new function is introduced to convert
  32 bit graphics to the correct format.
- Alfont seems to use wrong ascender and descender values for TTF fonts, there is a
  workaround written for it.
- Sound decoding and playing is handled in a separate thread to reduce 
  audio stuttering.
- Function imports for some plugins are implemented as stubs. Therefore games using
  them wil load and play but will lack certain visual effects. Stubbed functions
  are in place for: ags_shell.dll, ags_snowrain.dll, agsjoy.dll, agsblend.dll.
- OGG decoding is handled by libvorbisidec (libtremor).


Future plans:
- Running 3.1.x and 2.7x AGS games, preferably with a unified launcher.
- Ingame menu to change settings/controls.
- Graphical menus.
- Completely fixing the sound stuttering.



Credits:
Adventure Game Studio by Chris Jones (http://www.bigbluecup.com/)
Linux port by berolinux (http://gitorious.org/ags)
Additional code by Bernhard Rosenkraenzer and Christian Morales Vega
Eboot artwork by Paul Wilkinson (subspark)
PSP port by JJS

Thanks to thebudds for testing.
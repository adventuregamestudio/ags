#Adventure Game Studio runtime PSP port

Licensed under the Artistic License 2.0, see License.txt in the Engine folder.


##Requirements:

A PSP 2000/3000/GO with a Custom Firmware or Homebrew Enabler that can access the
extended memory. Examples are 5.00 M33, 5.50 GEN, 6.xx PRO, 6.20 TN-HEN.
To run most games you absolutely need the extended memory of the newer PSP models,
so while this will seem to work on the PSP 1000 you will encounter crashing with
larger games.
For the same reason, signing the Eboot is not useful as only the standard memory
is available on official firmwares. It also requires kernel mode if run with the menu.


##AGS game compatibility:

###This runtime engine port is not compatible with all AGS games. There are the following restrictions:

-   Resolution must be 320x200 or 320x240. No higher resolution is supported and
    you will receive an error stating that when trying to run such a game.
-   Games using a lot of memory (large rooms especially) will crash. You will then
    see a "blue screen of death" with debug information. Press X to save them
    to a file. The game will then quit and reopen the menu.

###These glitches can appear, affecting the game experience:

-   Games which make use of advanced scripting might run slow, possibly making
    them unplayable.
-   Sound will stutter in certain instances, e.g. when loading/saving.
-   When skipping cutscenes, the PSP may appear to hang for several seconds.
-   The frame rate drops when several sounds are played at once.


###Compatibility note on specific games:

####Playable with restrictions:

-   Quest for Glory II Remake: Very close to the memory limit. You have to set
    `clear_cache_on_room_change=1` and `enable_extra_memory=1` in the configuration
    file. The game might still crash later on, this is not fully tested.
-   Fountain of Youth Demo: The save/load dialog has a very low framerate, but
    otherwise the game runs fine.
-   The McCarthy Chronicles: In the games option menu, disable reflections to increase
    the frame rate. It is also recommended to move the *"film grain"* slider fully
    to the left.
-   The Journey Down: The outro sequence will run with a very low frame rate.
-   Aeronuts: The plane arcade sections are slow but playable.

####Not playable on the PSP:

-   Eternally Us: Runs out of memory before displaying anything.
-   Dead Hand: Very low frame rate (1 fps).
-   Dacey in the Dark - Prelude: Very low frame rate.
-   Of the Essence: Runs out of memory for the pathfinder after the intro sequence.
-   Death Wore Endless Feathers: The agstrans plugin is not available on the PSP.
-   Two of a Kind: The interface is not usable.
-   Prodigal: Crashes the MP3 player during the intro sequence.


##How to run games:

1.  Copy the original game folder into the Eboot directory.
    E.g. if you want to run the game *Demo Quest* you put the games files into
    the folder **x:\PSP\GAME\ags\Demo Quest\** (drive letter and PSP game folder
    is dependent on your configuration).
    NOTE: If the game is created with an incompatible engine version, this will
    be indicated in the games list.
2.  If you only want to run a single game, put the data files directly into the
    folder with the Eboot and rename the main game executable to **ac2game.dat**.
    It will then autostart this game.
3.  For midi music playback, you have to download GUS patches and place them
    in the Eboot directory. I recommend going to this address:

    http://alleg.sourceforge.net/digmid.html

    and downloading "Richard Sanders's GUS patches". A direct link is here:

    http://www.eglebbk.dds.nl/program/download/digmid.dat

    This 'digmid.dat' is, in fact, a **bzip2** archive, containing actual data file,
    which should be about 25 MB large. Extract that file, rename it to **patches.dat**
    and place it in the same directory as the Eboot.

    Advanced users can also download other patchsets. The configuration file
    must be named **default.cfg** and placed in the Eboot directory.


##The configuration file:

You can customize the behaviour of the runtime by placing a file **psp.cfg** in the
Eboot directory or the directory of the game you want to run. A config file in
the game directory will overwrite the settings from the global configuration file.

###Available settings:

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
      to effectively disable caching.
    samplerate: The sound playback sample rate, typical values are 44100, 22050, 11025. Lower values
      mean a slight speedup with the trade-off of reduced sound quality.

    [midi]
    enabled: If 1, midi music will be played. This requires a set of GUS patches, see above.
    preload_patches: Determines whether all GUS patches are loaded the first time a midi files is
      played. This will take several seconds during which the game is unresponsive, but it will
      provide faster loading of subsequent midi files.

    [graphics]
    scaling: The game's graphics can either be shown unscaled in their original resolution or stretched
      to the full height of the PSP screen.
    smoothing: If scaling is enabled, this determines whether a linear interpolation filter is used.
      This reduces scaling artefacts and gives a more uniform appearance but at the same times makes
      the crisp pixel art somewhat blurry.

    [compatibility]
    ignore_acsetup_cfg_file: If 1, the engine will not read settings from the AGS configuration file.
    enable_extra_memory: If 1, there are additional 4 MiB of memory available for the game. But this
      disables suspending the PSP. Also trying to use the pause feature of a Go causes a freeze.
      While enabling certain games to run (especially on a PSP 1000), this feature is experimental
      and might itself cause instability.
      NOTE: You must not have RemoteJoy (Lite) enabled while using the extra memory!
    clear_cache_on_room_change: If 1, all sprite data will be unloaded if the player changes the
      rooms. This helps saving memory but increases room load times.

    [misc]
    show_fps: If 1, shows the frame rate.
    disable_power_saving: The PSPs power saving features like dimming the backlight and switching to
      standby are deactivated if this is set.
    return_to_menu: If 1, you will reenter the menu instead of going back to the XMB after
      quitting a game. This only works if the AGS game has an option to quit.


###Available button values for mouse input:

    mouse_up, mouse_down, mouse_left, mouse_right: Moves the mouse cursor in the indicated direction.
      Note: The analog stick is always used for mouse movement too.
    mouse_click_left, mouse_click_right, mouse_click middle: Simulates the respective mouse button.
    mouse_slow_down: While this button is pressed, the speed of the mouse cursor will be decreased.

###Available button values for keyboard input:

    A ... Z, 0 ... 9, 0_PAD ... 9_PAD, F1 ... F12
    ESC, TILDE, MINUS, EQUALS, BACKSPACE, TAB, OPENBRACE, CLOSEBRACE, ENTER, COLON, QUOTE, BACKSLASH,
    BACKSLASH2, COMMA, STOP, SLASH, SPACE, INSERT, DEL, HOME, END, PGUP, PGDN, LEFT, RIGHT, UP, DOWN
    SLASH_PAD, ASTERISK, MINUS_PAD, PLUS_PAD, DEL_PAD, ENTER_PAD, PRTSCR, PAUSE, ABNT_C1, YEN, KANA,
    CONVERT, NOCONVERT, AT, CIRCUMFLEX, COLON2, KANJI, LSHIFT, RSHIFT, LCONTROL, RCONTROL, ALT, ALTGR,
    LWIN, RWIN, MENU, SCRLOCK, NUMLOCK, CAPSLOCK EQUALS_PAD, BACKQUOTE, SEMICOLON, COMMAND

###Available button values for the onscreen keyboard:

    keyboard_toggle: This button will switch between the standard button set and the one for the osk.
    keyboard_enter_char: Presses the currently selected key.
    keyboard_next_char: Switches focus to the next key in the current set.
    keyboard_previous_char: Switches focus to the previous key in the current set.
    keyboard_next_keyset: Switches to the next key set.
    keyboard_previous_keyset: Switches to the previous key set.


##Compiling the source:

1.  Building the AGS port:
    -   You need MinPSPW 0.11 for Windows or Linux.
    -   You also need the M33 SDK from 4.01 M33 or from PRO CFW.
    -   The sources for the AGS port are available from here:
        http://github.com/adventuregamestudio/ags
    -   Typing `make` in the **PSP** folder will build all files necessary and put them
        into the **PSP/bin** folder.

2.  Building the libraries:
    -   Check the file **readme.txt** in the **patches** folder.


##Main source code changes from the PC version (also check the commit log of the git repository):

-   Instances of unaligned variable access that are not allowed on the MIPS processor
    are replaced with equivalent memcpy calls.
-   The PSP framebuffer has a BGR colour order. A new function is introduced to convert
    32 bit graphics to the correct format.
-   Alfont seems to use wrong ascender and descender values for TTF fonts, there is a
    workaround written for it.
-   Sound decoding and playing is handled in a separate thread to reduce
    audio stuttering.
-   Function imports for some plugins are implemented as stubs. Therefore games using
    them will load and play but will lack certain visual effects. Stubbed functions
    are in place for: ags_shell.dll, ags_snowrain.dll, agsjoy.dll, agsblend.dll.
-   OGG decoding is handled by libvorbisidec (libtremor).

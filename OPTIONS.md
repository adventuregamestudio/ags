# Engine runtime options

## Configuration file locations

For historical reasons configuration file should be called "acsetup.cfg".

The engine supports three configuration files that are read in the following order, every next overriding values from the previous one:
1. Default config file, found in the game's installation directory, applied for the game loaded from that directory;
2. Current user's global config file, applied for any AGS game.
3. Current user's game config file, applied only for the game of particular title. This config file is also the one supposed to be written to when the engine or setup application modifies game configuration.

Locations of two latter files differ between running platforms:
  * **Linux**:
    * user's global config: $XDG_DATA_HOME/ags/acsetup.cfg
    * user's game config: $XDG_DATA_HOME/ags/GAMENAME/acsetup.cfg
    * NOTE: if $XDG_DATA_HOME is not defined, then "$HOME/.local/share" is used instead.
  * **Windows**:
    * user's global config: %USERPROFILE%/Saved Games/Adventure Game Studio/acsetup.cfg
    * user's game config: %USERPROFILE%/Saved Games/GAMENAME/acsetup.cfg

## Configuration file options

* **\[graphics\]** - display mode and various graphics options
  * driver = \[string\] - id of the graphics renderer to use. Supported names are:
    * D3D9 - Direct3D9 (MS Windows only);
    * OGL - OpenGL;
    * Software - software renderer.
  * software_driver = \[string\] - *optional* id of the SDL2 driver to use for the final output in software mode, leave empty for default. IDs are provided by SDL2, not all of these will work on any system:
    * direct3d, opengl, opengles, opengles2, metal, software.
  * fullscreen = \[string\] - a fullscreen mode definition, which may be one of the following:
    * WxH - explicit window size (e.g. `1280x720`);
    * xS - integer game scaling factor (e.g. `x4`);
    * desktop - use current system's desktop resolution;
    * native - use game's native resolution;
    * full_window - a borderless window covering whole desktop, as opposed to the exclusive fullscreen mode;
    * default - use engine defaults, which is `full_window`.
  * window = \[string\] - a windowed mode definition; the options are same as for the `fullscreen`, except for `full_window`, and following differences:
    * desktop - will try to create largest possible resizing window, while keeping game scaling style (see `game_scale_win`);
    * default - use engine defaults, which is `desktop`.
  * windowed = \[0; 1\] - whether to start the game in windowed mode.
  * game_scale_fs = \[string\] - game scaling rule for fullscreen mode, and...
  * game_scale_win = \[string\] - game scaling rule for windowed mode, where
    * max_round - deduce maximal integer multiplier that fits in current desktop/device size;
    * stretch - stretch to current desktop/device size;
    * proportional - similar to stretch, but keep game's aspect ratio.
  * filter = \[string\] - id of the scaling filter to use when required. Supported filter names are:
    * stdscale - nearest-neighbour scaling;
    * linear - anti-aliased scaling; not usable with software renderer.
  * refresh = \[integer\] - refresh rate for the display mode.
  * render_at_screenres = \[0; 1\] - whether the sprites are transformed and rendered in native game's or current display resolution;
  * supersampling = \[integer\] - supersampling multiplier, default is 1, used with render_at_screenres = 0 (currently supported only by OpenGL renderer);
  * vsync = \[0; 1\] - enable or disable vertical sync.
  * rotation = \[string | integer\] - screen rotation. Possible values are:
    * unlocked (0) - device can be freely rotated if possible.
    * portrait (1) - locks the screen in portrait orientation.
    * landscape (2) - locks the screen in landscape orientation.
  * sprite_cache_size = \[integer\] - size of the sprite cache, stored in RAM, in kilobytes. Default is 131072 (128 MB).
  * texture_cache_size = \[integer\] - size of the texture cache, stored in VRAM, in kilobytes. Default is 131072 (128 MB).
* **\[sound\]** - sound options
  * enabled = \[0; 1\] - enable or disable game audio.
  * driver = \[string\] - audio driver id, leave empty for default. Driver IDs are provided by SDL2 and are platform-dependent.
    * For Linux:
      * pulseaudio, alsa, arts, esd, jack, pipewire, disk, dsp, dummy
    * For Windows:
      * wasapi, directsound, winmm, disk, dummy
  * cache_size = \[integer\] - size of the sound cache, in kilobytes. Default is 32768 (32 MB).
  * stream_threshold = \[integer\] - max size of the sound clip that engine is allowed to load in memory at once, as opposed to continuously streaming one. In the current implementation this also defines the max size of a clip that may be put into the sound cache. Default is 1024 (1 MB).
  * usespeech = \[0; 1\] - enable or disable in-game speech (voice-overs).
* **\[mouse\]** - mouse options
  * auto_lock = \[0; 1\] - enables mouse autolock in window: mouse cursor locks inside the window whenever it receives input focus.
  * control_when = \[string\] - determines when the mouse cursor speed control is allowed, acceptable values are:
    * never - self-explanatory;
    * fullscreen - only when the game is run in fullscreen (this is default);
    * always - both in fullscreen and windowed mode.
  * control_enabled = \[0; 1\] - enables or disables mouse control. Note that this setting may be overriden by control_when.
  * speed_def = \[string\] - determines how the cursor speed value is interpreted, possible modes are:
    * absolute - use precisely the speed value provided by config;
    * current_display - keep cursor's speed by screen size relation by increasing actual cursor speed when running game in low resolution and decreasing when running in higher than the current user's dekstop resolution (this is default).
  * speed = \[real\] - mouse cursor speed (default is 1.0).
* **\[touch\]** - touch input options
  * emul_mouse_mode = \[string | integer\] - touch-to-mouse emulation (whether touch input should emulate the mouse input and how):
    * off (0) - disabled;
    * one_finger (1) - one finger maps directly to left click;
    * two_fingers (2) - tap 1 finger for left click, hold 1st finger and tap 2nd for right click, double tap 1 finger for drag mode.
  * emul_mouse_relative = \[0; 1\] - enable or disable relative emulated mouse movement.
* **\[language\]** - language options
  * translation = \[string\] - name of the translation to use. A \<name\>.tra file should be present in the game directory.
* **\[misc\]** - various options
  * datafile = \[string\] - path to the game file.
  * datadir = \[string\] - path to the game directory.
  * localuserconf = \[0; 1\] - read and write user config in the game's directory rather than using standard system path. Game directory must be writeable for this option to work, otherwise engine will fall back to standard path.
  * user_data_dir = \[string\] - custom path to savedgames location.
  * shared_data_dir = \[string\] - custom path to shared appdata location.
  * antialias = \[0; 1\] - anti-alias scaled sprites.
  * clear_cache_on_room_change = \[0; 1\] - whether to clear sprite cache on every room change.
  * load_latest_save = \[0; 1\] - whether to load latest save on game launch.
  * background = \[0; 1\] - whether the game should continue to run in background, when the window does not have an input focus (does not work in exclusive fullscreen mode).
  * show_fps = \[0; 1\] - whether to display fps counter on screen.
* **\[log\]** - log options, allow to setup logging to the chosen OUTPUT with given log groups and verbosity levels.
  * \[outputname\] = GROUP[:LEVEL][,GROUP[:LEVEL]][,...];
  * \[outputname\] = +GROUPLIST[:LEVEL];
    Groups may be defined either by name or by a LIST of one-letter IDs, preceded by '+', e.g. +ABCD:LEVEL. Verbosity may be defined either by name or a numeric ID.
    - OUTPUTs are:
      * stdout, file, debugger (external debugging program);
    - GROUPs are:
      * all, main (m), game (g), manobj (o), sdl (l), script(s), sprcache (c);
    - LEVELs are:
      * all, alert (1), fatal (2), error (3), warn (4), info (5), debug (6);
    - Examples:
      * file=all:warn
      * stdout=+mg:debug
  * file-path = \[string\] - custom path to the log file.
  * sdl = LEVEL - setup SDL's own logging level, defined either by name or numeric ID:
    * verbose (1), debug (2), info (3), warn (4), error (5), critical (6).
* **\[override\]** - special options, overriding game behavior.
  * multitasking = \[0; 1\] - lock the game in the "single-tasking" or "multitasking" mode. In the nutshell, "multitasking" here means that the game will continue running when player switched away from game window; otherwise it will freeze until player switches back.
  * os = \[string\] - trick the game to think that it runs on a particular operating system. This may come handy if the game is scripted to play differently depending on OS. Possible choices are:
    * dos - MS DOS;
    * win - Windows;
    * linux - Linux;
    * mac - MacOS.
  * legacysave_assume_dataver = \[integer\] - when restoring a save of a legacy format, lets engine assume certain game data version ID. This may be necessary because of a mistake in pre-3.5.0 save formats, where contents depended on a game data version rather than a save version.
  * legacysave_let_gui_diff = \[0; 1\] - when restoring a save of a legacy format, lets engine read less gui elements than are registered by the current game. This was a (possibly unintended) effect in pre-3.5.0 save format.
  * restore_game_key = \[integer\] - key for calling built-in restore game dialog. Key value corresponds to the [AGS script keycode](https://github.com/adventuregamestudio/ags-manual/wiki/Keycodes).
  * save_game_key = \[integer\] - key for calling built-in save game dialog.
  * upscale = \[0; 1\] - run game in the "upscale mode". The earlier versions of AGS provided support for "upscaling" low-res games to hi-res. The script API has means for detecting if the game is running upscaled, and game developer could use this opportunity to setup game accordingly (e.g. assign hi-res fonts, etc). This options works **only** for games created before AGS 3.1.0 with low-res native resolution, such as 320x200 or 320x240, and it may somewhat improve
  game looks.
* **\[disabled\]** - special instructions for the setup program hinting to disable particular options or lock some in the certain state. Ignored by the engine.
  * filters = \[0; 1\] - tells to lock "Graphics filter" selection in a default state;
  * \<filter id\> = \[0; 1\] - tells to remove particular graphics filter from the selection list;
  * antialias = \[0; 1\] - tells to lock "Smooth scaled sprites" in a default state;
  * render_at_screenres = \[0; 1\] - tells to lock "Render sprites in screen resolution" in a default state;
  * speechvox = \[0; 1\] - tells to lock "Use digital speech pack" in a default state;
  * translation = \[0; 1\] - tells to lock "Game language" in a default state;
  

## Command line

General usage: ags \[OPTIONS\] \[GAMEFILE PATH or GAME DIRECTORY\]
(Where "ags" is executable name)

Following OPTIONS are supported when running from command line:

* -? / --help - prints most useful command line arguments and quits.
* -v / --version - prints engine version and quits.
* --background - keep game running in background (does not work in exclusive fullscreen).
* --clear-cache-on-room-change - clears sprite cache on every room change.
* --conf \<FILEPATH\> - specify explicit config file to read on startup.
* --console-attach - write output to the parent process's console (Windows only).
* --fps - display fps counter.
* --fullscreen - run in fullscreen mode.
* --gfxdriver \<name\> - use specified graphics driver:
  * d3d9 - Direct3D9 (MS Windows only);
  * ogl - OpenGL;
  * software - software renderer.
* --gfxfilter \<name\> [ \<game_scaling\> ] - use specified graphics filter and scaling factor.
  * filter names:
    * stdscale - nearest-neighbour scaling;
    * linear - anti-aliased scaling; not usable with software renderer.
  * game scaling:
    * proportional, round, stretch,
    * or an explicit integer multiplier.
* --loadsavedgame \<filepath\> - load savegame on startup.
* --localuserconf - read and write user config in the game's directory rather than using standard system path. Game directory must be writeable.
* --log-OUTPUT=GROUP[:LEVEL][,GROUP[:LEVEL]][,...];
* --log-OUTPUT=+GROUPLIST[:LEVEL] - setup logging to the chosen OUTPUT with given log groups and verbosity levels (see explanation for the related config option).
  * Examples:
    * --log-file=all:warn
    * --log-stdout=+mg:debug
* --log-file-path=PATH - define custom path for the log file.
* --no-message-box - disable alerts as modal message boxes (Windows only).
* --no-translation - use default game language on start.
* --noiface - don't draw game GUI (for test purposes).
* --noscript - don't run room scripts (for test purposes); *WARNING:* unreliable.
* --nospr - don't draw room objects and characters (for test purposes).
* --noupdate - don't run game update (for test purposes).
* --novideo - don't play game videos (for test purposes).
* --rotation \<MODE\> - screen rotation preferences. MODEs are:  unlocked (0), portrait (1), landscape (2).
* --sdl-log=LEVEL - setup SDL's own logging level (see explanation for the related config option).
* --setup - run integrated setup dialog. Currently only supported by Windows version.
* --shared-data-dir \<DIR\> - set the shared game data directory. Corresponds to "shared_data_dir" config option.
* --startr \<room_number\> - start game by loading certain room (for test purposes).
* --tell - print various information concerning engine and the game, and quits. Output is done in INI format.
  * --tell-config - print contents of merged game config.
  * --tell-configpath - print paths to available config files.
  * --tell-data - print information on game data and its location.
  * --tell-gameproperties - print information on game general settings.
  * --tell-engine - print engine name and version.
  * --tell-filepath - print all filepaths engine uses for the game.
  * --tell-graphicdriver - print list of supported graphic drivers.
* --test - run game in the test mode, unlocking test key combinations.
* --translation - select the given translation on start.
* --user-data-dir \<DIR\> - set the save game directory. Corresponds to "user_data_dir" config option.
* --windowed - run in windowed mode.


Command line arguments override options from configuration file where applicable.

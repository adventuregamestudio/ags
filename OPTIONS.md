# Engine runtime options

## Configuration file locations

For historical reasons configuration file should be called "acsetup.cfg".

The engine supports three configuration files that are read in the following order, every next overriding values from the previous one:
1. Default config file, found in the game's installation directory, applied for the game loaded from that directory;
2. Current user's global config file, applied for any AGS game.
3. Current user's game config file, applied only for the game of particular title. This config file is also the one being written to when the engine or setup application (Windows only) modifies game configuration.

Locations of two latter files differ between running platforms:
  * **Linux**:
    * user's global config: $XDG_DATA_HOME/ags/acsetup.cfg
    * user's game config: $XDG_DATA_HOME/ags/GAMENAME/acsetup.cfg
    * NOTE: if $XDG_DATA_HOME is not defined, then "$HOME/.local/share" is used instead.
  * **Windows**:
    * user's global config: not used
    * user's game config: %USERPROFILE%/Saved Games/GAMENAME/acsetup.cfg

## Configuration file options

* **\[graphics\]** - display mode and various graphics options
  * driver = \[string\] - id of the graphics renderer to use. Supported names are:
    * Software - software renderer.
    * D3D9 - Direct3D9 (MS Windows version only).
    * OGL - OpenGL (currently supported on Android, iOS and Windows).
  * windowed = \[0; 1\] - when enabled, runs game in windowed mode.
  * screen_def = \[string\] - determines how display mode is deduced:
    * explicit - use screen_width and screen_height parameters;
    * scaling - sets equal to scaled game size;
    * max - sets equal to device/desktop size;
  * screen_width = \[integer\] - if screen_def is 'explicit', defines display mode width; otherwise ignored.
  * screen_height = \[integer\] - if screen_def is 'explicit', defines display mode height; otherwise ignored.
  * match_device_ratio = \[0; 1\] - when looking for appropriate fullscreen mode, prioritise ones which have same aspect ration as current device/desktop mode.
  * game_scale_fs = \[string | integer\] - game scaling rule for fullscreen mode, and...
  * game_scale_win = \[string | integer\] - game scaling rule for windowed mode, where
    * any integer number - positive number means upscale multiplier, negative number means downscale divisor;
    * max_round - deduce maximal integer multiplier that fits in current desktop/device size;
    * stretch - stretch to current desktop/device size;
    * proportional - similar to stretch, but keep game's aspect ratio;
  * filter = \[string\] - id of the scaling filter to use. Supported filter names are:
    * none - run in native game size;
    * stdscale - nearest-neighbour scaling;
    * hqx - high quality scaling filter; only usable in 32-bit games with software renderer;
    * linear - anti-aliased scaling; only usable with hardware-accelerated renderer;
  * refresh = \[integer\] - refresh rate for the display mode.
  * render_at_screenres = \[0; 1\] - whether the sprites are transformed and rendered in native game's or current display resolution;
  * supersampling = \[integer\] - supersampling multiplier, default is 1, used with render_at_screenres = 0 (currently supported only by OpenGL renderer);
  * vsync = \[0; 1\] - enable or disable vertical sync.
* **\[sound\]** - sound options
  * digiid = \[integer\] - digital driver id, not used on Windows.
  * midiid = \[integer\] - MIDI driver id, not used on Windows.
  * digiwinindx = \[integer\] - digital driver id, used only on Windows.
  * midiwinindx = \[integer\] - MIDI driver id, used only on Windows.
  * usespeech = \[0; 1\] - enable or disable in-game speech (voice-overs).
  * threaded = \[0; 1\] - when enabled, engine runs audio on a separate thread; WARNING: incomplete and unsafe feature that does not work well on every platform.
* **\[mouse\]** - mouse options
  * auto_lock = \[0; 1\] - enables mouse autolock in window: mouse cursor locks inside the window whenever it receives input focus.
  * control = \[string\] - determines when the mouse cursor speed control is enabled, acceptable values are:
    * never - self-explanatory;
    * fullscreen - only when the game is run in fullscreen (this is default);
    * always - both in fullscreen and windowed mode.
  * speed_def = \[string\] - determines how the cursor speed value is interpreted, possible modes are:
    * absolute - use precisely the speed value provided by config;
    * current_display - keep cursor's speed by screen size relation by increasing actual cursor speed when running game in low resolution and decreasing when running in higher than the current user's dekstop resolution (this is default).
  * speed = \[real\] - mouse cursor speed (default is 1.0).
* **\[language\]** - language options
  * translation = \[string\] - name of the translation to use. A \<name\>.tra file should be present in the game directory.
* **\[misc\]** - various options
  * log = \[0; 1\] - enable or disable writing debug messages to the log file.
  * datafile = \[string\] - path to the game file.
  * datadir = \[string\] - path to the game directory.
  * user_data_dir = \[string\] - custom path to savedgames location.
  * shared_data_dir = \[string\] - custom path to shared appdata location.
  * antialias = \[0; 1\] - anti-alias scaled sprites.
  * notruecolor = \[0; 1\] - run 32-bit games in 16-bit mode. This option may only be useful on old low-end machines.
  * cachemax = \[integer\] - size of the engine's sprite cache, in kilobytes. Default is 131072 (128 MB).
* **\[override\]** - special options, overriding game behavior.
  * multitasking = \[0; 1\] - lock the game in the "single-tasking" or "multitasking" mode. In the nutshell, "multitasking" here means that the game will continue running when player switched away from game window; otherwise it will freeze until player switches back.
  * os = \[string\] - trick the game to think that it runs on a particular operating system. This may come handy if the game is scripted to play differently depending on OS. Possible choices are:
    * dos - MS DOS;
    * win - Windows;
    * linux - Linux;
    * mac - MacOS;
  * upscale = \[0; 1\] - run game in the "upscale mode". The earlier versions of AGS provided support for "upscaling" low-res games to hi-res. The script API has means for detecting if the game is running upscaled, and game developer could use this opportunity to setup game accordingly (e.g. assign hi-res fonts, etc). This options works **only** for games created before AGS 3.1.0 with low-res native resolution, such as 320x200 or 320x240, and it may somewhat improve
  game looks.
* **\[disabled\]** - special instructions for the setup program hinting to disable particular options or lock some in the certain state. Ignored by the engine.
  * render_at_screenres = \[0; 1\] - tells to lock "Render sprites in screen resolution" in a default state;
  * speechvox = \[0; 1\] - tells to lock "Use digital speech pack" in a default state;
  * filters = \[0; 1\] - tells to lock "Graphics filter" selection in a default state;
  * \<filter id\> - tells to remove particular graphics filter from the selection list;
  

## Command line

General usage: ags \[OPTIONS\] \[GAMEFILE PATH or GAME DIRECTORY\]
(Where "ags" is executable name)

Following OPTIONS are supported when running from command line:

* -? / --help - prints most useful command line arguments and quits.
* -v / --version - prints engine version and quits.
* --setup - run setup dialog. Currently only supported by Windows version.
* --log - write debug messages to log file.
* --no-log - prevent from writing to log file.
* --fullscreen - run in fullscreen mode.
* --windowed - run in windowed mode.
* --gfxfilter \<name\> [ \<game_scaling\> ] - use specified graphics filter and scaling factor (see explanation above).
* --hicolor - force hicolor (16-bit) mode when running 32-bit games. This option may only be useful on old low-end machines.
* --fps - display fps counter.

Command line arguments override options from configuration file where applicable.

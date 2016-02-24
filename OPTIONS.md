# Engine runtime options

## Configuration file

* **\[sound\]** - sound options
  * digiid = \[integer\] - digital driver id.
  * midiid = \[integer\] - MIDI driver id.
  * usespeech = \[0; 1\] - enable or disable in-game speech (voice-overs).
  * threaded = \[0; 1\] - when enabled, engine runs audio on a separate thread; WARNING: incomplete feature that does not work well on Linux-based platforms.
* **\[language\]** - language options
  * translation = \[string\] - name of the translation to use. A <name>.tra file should be present in the game directory.
* **\[misc\]** - various options
  * log = \[0; 1\] - enable or disable writing debug messages to the log file.
  * datafile = \[string\] - path to the game file.
  * datadir = \[string\] - path to the game directory.
  * windowed = \[0; 1\] - when enabled, runs game in windowed mode.
  * gfxdriver = \[string\] - id of the graphics renderer to use. Supported names are:
    * DX5 - software renderer.
	* D3D9 - Direct3D9 (MS Windows version only).
	* OGL - OpenGL (iOS and Android versions only).
  * gfxfilter = \[string\] - id of the scaling filter to use. Supported filter names are:
    * max - use highest supported nearest-neighbour scaling;
    * none - run in native game size;
    * stdscaleN, where N is between 2 and 8 - nearest-neighbour scaling with N multiplier;
    * hq2x, hq3x - high quality scaling filter (x2 and x3 scaling correspondingly); only usable with software renderer;
    * aaxN, where N is between 2 and 8 - anti-aliased scaling with N multiplier; only usable with hardware-accelerated renderer;
  * refresh = \[integer\] - refresh rate for the display mode.
  * prefer_letterbox = \[0; 1\] - engine will prefer letterboxed resolutions when looking for best display mode.
  * prefer_sideborders = \[0; 1\] - engine will prefer resolutions with side borders when looking for best display mode.
  * antialias = \[0; 1\] - anti-alias scaled sprites.
  * notruecolor = \[0; 1\] - run 32-bit games in 16-bit mode. This option may only be useful on old low-end machines.
  * cachemax = \[integer\] - size of the engine's sprite cache, in kilobytes. Default is 20480 (20 MB).
* **\[override\]** - special options, overriding game behavior.
  * multitasking = \[0; 1\] - lock the game in the "single-tasking" or "multitasking" mode. In the nutshell, "multitasking" here means that the game will continue running when player switched away from game window; otherwise it will freeze until player switches back.
  * os = \[string\] - trick the game to think that it runs on a particular operating system. This may come handy if the game is scripted to play differently depending on OS. Possible choices are:
    * dos - MS DOS;
	* win - Windows;
	* linux - Linux;
	* mac - MacOS;
  * upscale = \[0; 1\] - run game in the "upscale mode". The earlier versions of AGS provided support for "upscaling" low-res games to hi-res. The script API has means for detecting if the game is running upscaled, and game developer could use this opportunity to setup game accordingly (e.g. assign hi-res fonts, etc). This options works **only** for games created before AGS 3.1.0 with low-res native resolution, such as 320x200 or 320x240, and it may somewhat improve
  game looks.
  

## Command line

General usage: ags \[OPTIONS\] \[GAMEFILE PATH or GAME DIRECTORY\]
(Where "ags" is executable name)

Following OPTIONS are supported when running from command line:

* --help - displays most useful command line arguments;
* --setup - run setup dialog. Currently only supported by Windows version.
* --log - write debug messages to log file.
* --no-log - prevent from writing to log file.
* --fullscreen - run in fullscreen mode;
* --windowed - run in windowed mode;
* --gfxfilter <name> - use specified graphics filter.
* --letterbox - tell engine to prefer letterboxed resolutions when looking for best display mode;
* --hicolor - force hicolor (16-bit) mode when running 32-bit games. This option may only be useful on old low-end machines.
* --fps - display fps counter;

Command line arguments override options from configuration file where applicable.

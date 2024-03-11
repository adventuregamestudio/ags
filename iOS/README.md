# AGS for iOS

iOS thread on the AGS forum: https://www.adventuregamestudio.co.uk/forums/index.php?topic=46040.0

## Building

Currently building both requires Xcode and macOS. Development was originally done in Xcode 14.

The first thing you need to do is to get all dependencies necessary in `libsrc/` in the project root.
To do this, navigate to it and run the `./download.sh` script there.

After the script is ran, open Xcode and load the project in `<SOURCE>/iOS/xcode/ags/ags.xcodeproj`.

The iOS AGS app is all contained in a single Xcode project. It currently embeds SDL as a subproject
downloaded by `./download.sh` and located in the in the `/libsrc` folder.

## Preparing game

1. Copy the game files to `iOS/Resources`: `game.ags`, `speech.vox`, `audio.vox`, and any other files related to the game (e.g. Lip Sync)
2. Add additional files to `AGS/Resources`` in the Xcode project
3. Select the target "AGS Game" in the AGS project
4. Edit the Identity section (Category, Display Name, bundle identifier, and version)
5. Under Build Phases -> Copy Bundle Resources, add additional game resources that need to be embedded with the game, the ones that were added to `AGS/Resources` (e.g. Lip Sync files)
4. Build and run the AGS Game target on the simulator or device! Remember you need to set a Team, like your Personal Team, and a Bundle identifier (e.g. com.mystudio.mygame).
5. Modify the `AGS/Resources/ios.cfg` file to make game setup changes (see below)

You may also need to manually assign a launch screen and game icons the way you normally would for an iOS app.

## Game setup config file

The configuration file is similar, but unique to mobile and iOS devices and may differ from the configuration for Windows/Linux builds. Here's an explanation of all the settings:

```ini
[misc]
config_enabled = 1    ; Enable or disable config. If disabled, only translation config is read, default 0
rotation = 1          ; Set game rotation (0 = unlocked, 1 = force portrait, 2 = force landscape), default 0
translation = default ; (string) Translation to use, default "default"
[controls]
mouse_emulation = 1   ; 0 = off, 1 = one finger (hold, drag), 2 = two fingers (tap), default 1
mouse_method = 0      ; Mouse emulation method (0 = absolute, 1 = relative), default 0
mouse_speed = 1.0     ; Mouse speed in relative mode, default 10
[compatibility]
clear_cache_on_room_change = 0  ; For low-end devices, clear the cache when changing rooms (1 = enabled), default 0
[sound]
enabled = 1           ; Enable sound, default 0,
cache_size = 64       ; Sound cache size in bytes, default 32 * 1024
[video]
framedrop = 0         ; Video framedrop, default 0
[graphics]
renderer = 1          ; Renderer type (0 = Software, 1 = Render to screen, 2 = Render to texture), default 0
smoothing = 0         ; Scale smoothing (0 = nearest-neighbor, linear) enabled, default 0,
scaling = 1           ; Scaling style (0 = none, 1 = stretch and preserve aspect ratio, 2 stretch to whole screen), default 0
smooth_sprites = 0    ; Sprite anti-aliasing, default 0
[debug]
show_fps = 0          ; Show FPS overlay while running the game, default 0
logging = 0           ; Write logging to logcat, default 0
```

## Embedded plugins

The AGS iOS build will provide ports for existing AGS plugins built right into the library:

* Snow/Rain
* Parallax
* Flashlight
* Touch

The Touch plugin is specific to the iOS build and allows the option to pull
the virtual keyboard with the following calls in a game script:

```agsscript
// Shows the virtual keyboard
import void TouchShowKeyboard();

// Hides the virtual keyboard
import void TouchHideKeyboard();

// Returns true or false if the virtual keyboard is visible
import bool TouchIsKeyboardVisible();
```

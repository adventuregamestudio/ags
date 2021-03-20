# Hell's Puppy Android Studio Project

This is the Android Studio Project for the Game Hell's Puppy built on the Global Game Jam 2019.

This game repository is available here: [github.com/VacaRoxa/dogfromhell](https://github.com/VacaRoxa/dogfromhell)

This is only the repository for it's Android Studio Project, shared with the intention to 
enable other AGS Projects to use this as a base in recent versions of Android Studio.

This source code was originally built by monkey0506 for OceanSpirit Dennis: Scourge of the
Underworld. I then refactored and slightly changed some bit to make it work for Dungeon
Hands, with newer versions of Android Studio. 

This [Android Studio](https://developer.android.com/studio/index.html) project is designed for
building standalone APKs of [Adventure Game Studio](http://www.adventuregamestudio.co.uk/) games,
for release on the Google Play Store.

## Prerequisites

You will need the following to build this project:

* [Android Studio](https://developer.android.com/studio/)
* AGS engine native libraries.
* A **compiled** AGS game (**NOTE:** Game must be **< 2 GB total**).
* [jobbifier](https://github.com/monkey0506/jobbifier), replacement for `jobb` tool

### Downloading AGS Engine Native Libraries 

***Building AGS is out of the scope of this README, so do this***

It is much easier to download the pre-built native libraries than it is to rebuild them yourself, so
unless you have made any changes to the AGS engine or the built-in plugin sources there's no need to
build them. You can grab the latest libraries from the forums, under Editor Releases, Crimson
Wizard usually releases under For Android spoiler note. 
[Here is a link for 3.4.1 native engine builds](http://www.adventuregamestudio.co.uk/releases/finals/AGS-3.4.1-P4/ags-3.4.1.15-android-libs.zip)

Extract the contents of the android-libs.zip (armeabi, armeabi-v7a, mips, and x86 folders) to `PROJECT_DIR/agsEngineLibrary/jniLibs/`
(where `PROJECT_DIR` is the directory where you cloned this project). **You will have to create `PROJECT_DIR/agsEngineLibrary/jniLibs/`**. 

## Setting up the project for your game

With the prerequisites installed, you will need to change the following items to set up the project
for building your game.

* Before opening Android Studio, do as I told before and get AGS Engine Native Libraries and place it in `PROJECT_DIR/agsEngineLibrary/jniLibs/`.

* Open Android Studio and wait it to sync.

* Update package name:
  * [![](https://user-images.githubusercontent.com/2244442/52019947-9ac95d80-24d6-11e9-9b41-a99b0d8cfe89.gif)](https://user-images.githubusercontent.com/2244442/52019940-969d4000-24d6-11e9-838c-3f8d83c2fccb.gif)
  * Open the project in Android Studio, then in the project tree navigate to
    app/java/com.mythsuntold.hellspuppy
  * Right-click on this folder and select "Refactor -> Move...". When prompted, select "Move
    package 'com.vacaroxa' to another package" (the default). You may receive a
    warning that multiple directories will be moved, select Yes. Type the *parent* name of your
    package, not the *final* package name (e.g., `com.bigbluecup` *not* `com.bigbluecup.game`),
    select "Refactor" and then click "Do Refactor".
  * Right-click on the new project folder in the project tree (e.g., `com.bigbluecup.hellspuppy`)
    and select "Refactor -> Rename". Type the package name for your game (e.g., `game`), then
    select "Refactor" and click "Do Refactor".
  * Finally, delete the `com.mythsuntold.hellspuppy` folder.

* Update `project.properties`. This file contains gradle settings related to your project. The
  application ID, version code, and version name need to be set to match your project settings
  (application ID is your package name).

* Update `app/src/main/res/project.xml`. This file contains resources for your project. 
  The values there are described in that file.

* Update `local.static.properties`. This file contains local data that should **NOT** be added
  to version control (.gitignore will ignore your changes to this file). You need to add your
  keystore path, alias, and passwords, and optionally the path to your copy of the AGS source
  (if you are rebuilding the engine native libraries). See the
  [Java docs on keytool](http://docs.oracle.com/javase/6/docs/technotes/tools/solaris/keytool.html)
  or use the
  [Android Studio signing wizard](https://developer.android.com/studio/publish/app-signing.html)
  to generate a keystore.

* Update `app/src/main/res/private.xml`. This file contains definitions for your RSA public key and an
  integer-array with bytes for your salt which is used by the ExpansionDownloaderService. These
  values are necessary if distributing your app via the Google Play Store. The RSA public key
  is provided in the Google Play Developer Console, and the salt bytes may be any number of
  values in the range [-128, 127]. You may need to upload an APK without the RSA public key
  first before the key is provided. That APK should not be public unless the OBB file is
  embedded.

* Update graphics resources (`app/src/main/res`). You can use the
  [Android Asset Studio](https://romannurik.github.io/AndroidAssetStudio/) to easily generate
  graphics for your app, or use your preferred method. Everything in the `drawable` and various
  `mipmap-*` folders should be replaced with your resources.

* Update your `app/src/main/AndroidManifest.xml` , check if `android:name` of both your application and
  the your MainActivity to your `STUDIONAME.GAMENAME` instead of `vacaroxa.hellspuppy`. If you have
  my game installed and don't do this, it will launch Hell's Puppy instead of your game!


* Build your `.obb` expansion file, if it's smaller then 100MB, place on `app/src/main/assets/`. If
it isn't, see detailed instructions below.

## `.obb` APK Expansion File

Your game's data files need to be packaged into an
[OBB file](https://developer.android.com/google/play/expansion-files.html). The `jobb` tool included
with the Android SDK has been identified with bugs that prevent using game folders of *< 4 MB* or
*> 511 MB*. The `jobbifier` tool has fixes for these bugs and includes a GUI application. If you
prefer the command line, the jobb source included with the jobbifier tool may still be run from the
command line.

Whether using the jobb tool or jobbifier, the input directory is your AGS game's Compiled folder. If
using AGS 3.4.0, you will need to copy the data files from "Compiled" into a separate directory
first (newer versions of AGS will place these files into a separate "Compiled/data" folder, and
eventually include an Android builder). Specifically you need the ".ags" or ".exe" file and any
resource and VOX files. Note that (in my tests) not all versions of Android properly support
mounting embedded OBB files, so it is currently not recommended to add a password when creating the
OBB file.

Copy your game files and pasted it under the `obb/` folder in this repo, with the android.cfg file.
Edit android.cfg according to your game (eg: rotation = 1 for vertical or 2 for horizontal game, etc).

![](https://user-images.githubusercontent.com/2244442/52019491-08748a00-24d5-11e9-9c2d-76acb04da1c1.png)

On the input folder, navigate to this repo `obb/` folder and on output, navigate to `app/src/main/assets/` 
on this repo folder. Don't use password, leave as main, don't forget to write your identifier and please 
give the same version written in your game project in android studio.

Your expansion OBB file *may* be embedded into the APK by placing it in the "app/src/main/assets"
folder, but it should be understood that this requires *copying* the OBB file onto external storage
before it can be used (duplicating the size of your game files!). Once you can get it running like this, 
a reminder that you can ship the obb and apk separately in the Google Play Console, because you 
can't have bigger than 100MB apks.


So for bigger games you will be using the Download Library, though this only applies to apps 
published via Google Play. You may explore other methods of bundling the game files into the APK, 
but these two methods are currently supported by the code. You can also distribute the OBB file 
yourself by other means, but you must make sure that the file is installed to 
the `OBB_FILE_EXTERNAL_PATH` (see `app/src/main/java/MainActivity.java`) on the user's device.

Another note on the External obb to download through Google Play, under `app/src/main/res/drawable`
are the images shown while downloading the obb files, so update those to match your game images.

## Running this project


With your device connected in Android Studio, selected from the list, click play to run the debugger 
and have your game process selected in debugable process. This will give you better messages in the logcat.

![](https://user-images.githubusercontent.com/2244442/52019497-0f9b9800-24d5-11e9-99f0-6b602ee533ab.png)


## Building the APK

To build your APK for release, simply select "Build -> Generated Signed APK..." in Android Studio
and follow the wizard, or run `gradlew.bat assembleRelease` from the project directory. This APK
will be signed and aligned for release on Google Play.

## Androig Configuration

`android.cfg`

Below midi is disabled for everyone happiness.
Rotation 0 allows the player to control rotation - you probably don't want that.

```
[misc]
config_enabled = 1
rotation = 0
translation = default
[controls]
mouse_method = 0
mouse_longclick = 1
[compatibility]
clear_cache_on_room_change = 0
[sound]
samplerate = 44100
enabled = 1
threaded = 1
cache_size = 10
[midi]
enabled = 0
preload_patches = 0
[video]
framedrop = 0
[graphics]
renderer = 0
smoothing = 0
scaling = 1
super_sampling = 0
smooth_sprites = 0
[debug]
show_fps = 0
logging = 0
```


## License and Author

The libraries [**`downloader_library`**](https://github.com/monkey0506/google_downloader_library) and [**`play_licensing_library`**](https://github.com/monkey0506/google_play_licensing_library) are release through The Android Open Source Project, using APACHE-2 License, and has modifications provided by MonkeyMoto Productions on Public Domain. I did only small modifications on the gradle to build.

This code was initially provided by MonkeyMoto Productions, under Public Domain, on the [monkey0506/osd-scourgeoftheunderworld-as](https://github.com/monkey0506/osd-scourgeoftheunderworld-as) repository. My modifications on this repository are provided under [MIT License](LICENSE).

# Single Game Release Android Studio Project

This [Android Studio](https://developer.android.com/studio/index.html) Project is designed for
building standalone Apps of [Adventure Game Studio](http://www.adventuregamestudio.co.uk/) games,
for release on the Google Play Store. This project uses the SDL2 base Android Project, 
and has some code bits also from the monkey0506 project OceanSpirit Dennis: Scourge of the Underworld. 

## Prerequisites

You will need the following to build this project:

* [Android Studio](https://developer.android.com/studio/)
* AGS engine native libraries.
* A **compiled** AGS game (**NOTE:** Game must be **< 1 GB total**).

***NOTE:** Without AGS engine native libraries*

If you cloned the whole AGS Repository, you will need CMake and Ninja. If you are using Android Studio Artic Fox or newer, you 
can go into Tools -> SDK Manager, and in the SDK Tools tab mark NDK Side by Side and CMake. After that you will be prompt to accept licenses
and you will be able to build this project entirely, including AGS engine native from source.

### Downloading AGS Engine Native Libraries 

It is much easier to download the pre-built native libraries than it is to rebuild them yourself, so
unless you have made any changes to the AGS engine or the built-in plugin sources there's no need to
build them. You can grab the latest libraries from the forums, under Editor Releases, Crimson
Wizard usually releases under For Android spoiler note in the forums.

Extract the contents of the android-libs.zip (armeabi, armeabi-v7a, mips, and x86 folders) to `mygame/../library/runtime/libs`, 
there's a file called `libs_go_here.txt` in this directory which explains how it will look like after you place the files.


## Setting up the project for your game

With the prerequisites installed, you will need to change the following items to set up the project
for building your game.

- Before opening Android Studio, do as I told before and get AGS Engine Native Libraries and place it in `mygame/../library/runtime/lib`.

- Open Android Studio and wait it to sync.

- Update package name:
  - [![](https://user-images.githubusercontent.com/2244442/52019947-9ac95d80-24d6-11e9-9b41-a99b0d8cfe89.gif)](https://user-images.githubusercontent.com/2244442/52019940-969d4000-24d6-11e9-838c-3f8d83c2fccb.gif)
  - Open the project in Android Studio, then in the project tree navigate to
    app/java/com.mystudioname.mygamename
  - Right-click on this folder and select "Refactor -> Move...". When prompted, select "Move
    package 'com.vacaroxa' to another package" (the default). You may receive a
    warning that multiple directories will be moved, select Yes. Type the *parent* name of your
    package, not the *final* package name (e.g., `com.bigbluecup` *not* `com.bigbluecup.game`),
    select "Refactor" and then click "Do Refactor".
  - Right-click on the new project folder in the project tree (e.g., `com.bigbluecup.mygamename`)
    and select "Refactor -> Rename". Type the package name for your game (e.g., `game`), then
    select "Refactor" and click "Do Refactor".
  - Finally, delete the `com.mystudioname.mygamename` folder.

- Update `project.properties`. This file contains gradle settings related to your project. The
  application ID, version code, and version name need to be set to match your project settings
  (application ID is your package name).

- Update `app/src/main/res/project.xml`. This file contains resources for your project. 
  Update the `app_name` there from `MY_FULL_GAME_NAME` to match your game name!

- Update `local.static.properties`. This file contains local data that should **NOT** be added
  to version control (.gitignore will ignore your changes to this file). You need to add your
  keystore path, alias, and passwords. See the [Java docs on keytool](http://docs.oracle.com/javase/6/docs/technotes/tools/solaris/keytool.html)
  or use the [Android Studio signing wizard](https://developer.android.com/studio/publish/app-signing.html)
  to generate a keystore.

- Update graphics resources (`app/src/main/res`). You can use the
  [Android Asset Studio](https://romannurik.github.io/AndroidAssetStudio/) to easily generate
  graphics for your app, or use your preferred method. Everything in the `drawable` and various
  `mipmap-*` folders should be replaced with your resources.

- Update your `app/src/main/AndroidManifest.xml` , check if `android:name` of both your application and
  the your MainActivity to your `STUDIONAME.GAMENAME` instead of `mystudioname.mygamename`. 


## Packaging the AGS game for Google Play

There are two ways to go when creating your App. As of 2021, Google Play no longer accepts APKs and OBBs for newer apps.

Additionally and also Google Play has size limits, you can upload an App bundle of up to 150 MB (this is the final size), and you can also have an install time asset of up to 1GB. 

- If your AGS game is smaller than 125 MB, place it's `.ags`, `acsetup.cfg` and additional files in `mygame/app/src/main/assets` directory.

- If your game is bigger than 125 MB, then place these files under `mygame/game/src/main/assets`, and it will create a package that will use Play Asset Delivery for installing when your player installs your app.

Once you placed your game in the appropriate directory, in Android Studio click in `Build -> Build Bundles (s) / APK (s) -> Build Bundle(s)`, this will produce a `.aab` file under `mygame/app/build/outputs/bundle/debug`.

To properly build your app for delivery (`mygame/app/build/outputs/bundle/release`), you will need to sign your app. Go to `Build -> Generate Signed Bundle or APK`, select `Android App Bundle` and follow the screens. Make sure to note down your passwords!

## Including binary plugins

In the `../plugins` dir, you can place binary prebuilts of plugins, in their respective arch directories, which gradle will then package in place in the same directory as the ags and sdl libraries in the app.

Read the [`../plugins/plugins_go_here.txt`](../plugins/plugins_go_here.txt) file for description of expected directory structure.

## Packaging the AGS game as APK for other stores

Other stores that have not yet implemented Android App Bundles don't have a size limit for APKs. So instead of the above, place your `.ags`, `acsetup.cfg` and additional files directly in `mygame/app/src/main/assets` directory.

To build your APK for release, simply select "Build -> Generated Signed APK..." in Android Studio and follow the wizard, or run `gradlew.bat assembleRelease` from the project directory. 
This APK will be signed and aligned for release.


## Running this project

With your device connected in Android Studio, selected from the list, click play to run the debugger 
and have your game process selected in debugable process. This will give you better messages in the logcat.

![](https://user-images.githubusercontent.com/2244442/52019497-0f9b9800-24d5-11e9-99f0-6b602ee533ab.png)

To be easier for debugging, keep your game files on `mygame/app/src/main/assets` instead of `mygame/game/src/main/assets` even if you plan to use Play Asset Delivery later.


## Testing the app bundle

If you want to make sure that the built `.aab` bundle actually has your game package working, you can use the bundletool, grab it from it's [release page](https://github.com/google/bundletool/releases).

Now, you will need to set `JAVA_HOME`, `ANDROID_HOME` and make sure java is on your Path to be able to use this tool. Below is an example for configuring via `cmd.exe` on Windows, but adjust it accordingly, attention to java directory that will change on each version.

    setx ANDROID_HOME "%USERPROFILE%\AppData\Local\Android\Sdk"
    setx JAVA_HOME "%ProgramFiles%\Android\Android Studio\jre"
    setx Path "%Path%;%JAVA_HOME%\bin"

With that set, generate APKs with the `--local-testing` flag

    java -jar bundletool-all.jar build-apks --bundle=path/to/your/bundle.aab  --output=output.apks --local-testing

And after, you can push the generated apks file with package, just connect a device (or turn on the emulator) and run bundletool to sideload the APKs:

    java -jar bundletool.jar install-apks --apks=output.apks

_note:_ If you don't have Android Studio installed you will need to install java, [grab it here](https://www.java.com/en/download/), and you need to tell your software where Java is, so use `setx JAVA_HOME "C:\Program Files\Java\jre1.8.0_301"`.


## Androig Configuration

Android's port configuration is looked up in a file called `android.cfg`. See the main [Preferences section](../README.md) in the main Android README for the details.

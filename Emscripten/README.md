# Adventure Game Studio - Emscripten

This is the web port of AGS.


## Installing dependencies

### Windows
On Windows you need Windows 10 or 11, because it depends on Python >= 3.9.2. Python should be on your PATH. You can simply type `python` on the `cmd.exe` and it will guide you through installing python on Windows from MS Store, which should work fine. Using the new [Windows Terminal](https://github.com/microsoft/terminal) is not needed but it's a nice quality of life improvement.

You will also need to install and add to your path
- [git](https://git-scm.com/) 
- [cmake](https://cmake.org/), any CMake above 3.16.3 should be fine, but the newer, the better on Windows
- [ninja](https://ninja-build.org/), this is a portable binary so just put it in an empty directory for your user and add that directory to your path

### Linux or macOS
On Linux or macOS you will need Python3, anything above 3.6.5 should be fine, [see this for more info](https://github.com/emscripten-core/emscripten/issues/6275)

You will also need

- git
- cmake, any CMake above 3.16.3 should be fine.
- make

You can install these using your system package manager on Linux or brew on macOS.

### Other OSes

For help with EMSDK requirements in other OS and configurations, refer to [EMSDK installation docs](https://emscripten.org/docs/getting_started/downloads.html).


## Build Requirements

Your environment will need **git**, **python3**, **pip** and **cmake**, any reasonably updated version of these are fine. On Linux you will need **make** for building, on Windows **ninja**.

With those, it's time to install Emscripten's EMSDK, either on bash (if on Linux) or cmd (if on Windows). Currently, you can clone and install it in this directory as following:

    cd Emscripten
    mkdir emscripten
    cd emscripten
    git clone https://github.com/emscripten-core/emsdk.git
    cd emsdk
	
On bash:
	
    ./emsdk install latest
    ./emsdk activate latest

On cmd:	

    .\emsdk install latest
    .\emsdk activate latest

If you are on cmd, your environment variables are now pointing to Emscripten utilities (you can also reapply by running `emsdk_env.bat`). 

If you are on bash however, you need to change the environment variables:

    source ./emsdk_env.sh

Now in this terminal instance, you can use the Emscripten utilities to actually build AGS!

Navigate to `ags/Emscripten` directory and build it using cmake as following:

    cd ags/Emscripten
    mkdir build-web-release
    cd build-web-release
    emcmake cmake ../.. -DCMAKE_BUILD_TYPE=Release
    
After CMake is done generating (some files are downloaded at this time and may take time the first time) if on Linux, use make

    make
	
If you are on Windows, just use ninja

    ninja
	
This may take a while, the first time you run this it will also download and build all Emscripten STD library and dependencies. After it's done you should see an `ags.wasm`, `ags.js` and `ags.html` file in `build-web-release` directory. 

## Using IDE with CMake support

You may pass the Emscripten toolchain file in a new profile where you set the CMake variables, using 
`-DCMAKE_TOOLCHAIN_FILE=./Emscripten/emscripten/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake`.

## Running

For running the build result, if you built the default web port (with AGS_DISABLE_THREADS), you just need a simple web server to serve it, and a web browser to run it. Either **Chrome** or **Firefox** should be fine.

For the web server, since you have python3 installed, you can use it's own web server. Navigate to `Emscripten/build-web-release` or the directory your build result is, and invoke the python3 web server there:

    python -m http.server
	
Now, navigating to http://localhost:8000/ags.html should open the AGS web port you just built!

Click to browse files and select all the files for the AGS game you want to test!

***warning:*** as with every web development be aware your browser will try to cache things and will give you non-updated results, force refresh with ctrl+shift+R or use incognito/private tabs to make sure you are being served the latest build result!


## Debugging

![screenshot_debug](https://user-images.githubusercontent.com/2244442/147873441-181a432b-6bea-499e-a8c1-e0ba3d166f4c.png)

You already understand how to build and how to run, but you are changing the AGS codebase and it just doesn't work! Something is wrong... It's time for debugging!

Chrome has the best tooling for debugging Webassembly and I recommend you use it for this task. But there are a couple of things we can do to make life a bit easier. We are going to rebuild with some debug additions! On the `Emscripten/build-web-release` directory, rerun CMake as below.

    emcmake cmake ../.. -DCMAKE_C_FLAGS="-g4" -DCMAKE_CXX_FLAGS="-g4" -DCMAKE_EXE_LINKER_FLAGS="-g4 -fsanitize=null -fsanitize-minimal-runtime --emit-symbol-map --source-map-base http://localhost:8000/"
	
And then run either **ninja** or **make** to rebuild with the new debug options. 

After the build result is regenerated, serve it again with `python3 -m http.server` and [open](http://localhost:8000/ags.html) in the Chrome browser. Click on the browser three dots, more tools and select Development tools on Chrome - if you are not running the game yet, you can use f12 or Ctrl+Shift+I to bring the Developer Tools.

Finally, now you can navigate to Sources and see the cpp files from AGS there, you can set breakpoints and explore the source code! 

You may need to manually add the source files by clicking "+Add folder to workspace" and selecting the `ags/` code directory. The directory folder icons for the Engine and Common directory must be with full color and not muted, if this is not the case, load the console and make sure the WASM source map was download sucessfully - the console will have a message in case of otherwise.

If you want to fully integrate the IDE to the Chrome tools, the only one currently able to do it is VS Code, but this is a better topic to ask in the forums.


## Changes from other ports

- Single thread: Emscripten support pthreads and JS has multithreading features, but having multiple threads today with current in place wasm solutions in Browsers require that the website serves two additional headers. Unfortunately, the usual hosts for this kinda of build doesn't serve these (itch.io, gamejolt, GitHub, ...). Currently this means supporting single thread audio in AGS (at compile time).
  - the pretty code for this would require [#1349](https://github.com/adventuregamestudio/ags/issues/1349).
  - If you enable threads, editing the `CMakeLists.txt` to have `set(AGS_DISABLE_THREADS FALSE)`, you are going to fall in Emscripten Pthreads
    - Your browser need to be served both COOP, COEP and also the webpage has to be HTTPS with a valid certificate! This requires a webserver you are fully in control currently.
    - see more info here: https://emscripten.org/docs/porting/pthreads.html

It's important to think of the browser tab with the game and the JS/WASM machine to be running in a single thread, so this means at some point the browser must be in focus so it can process inputs and draw in the screen!

Since AGS has many loops internally, the way it's currently done is using [Asyncify](https://emscripten.org/docs/porting/asyncify.html), there's also [a good talk here](https://www.youtube.com/watch?v=qQOP6jqZqf8), so essentially at some points the stack unrolls and it passes to the browser do it's stuff, and then the stacks rollback to where it was. Currently the SDL_Delay is used to make this work!

There are other important differences when writing your game:

- You can't move the player mouse, so setting mouse position won't work.
- Esc key will always exit fullscreen mode, so avoid Esc in your control scheme for your Web ports. People use it for menus a lot, use a different key.
- Avoid big games, they will take a long time to download, take a big chunk of RAM, and may crash the JS VM in the browser. Try to keep things below 150 MBs.
- There's no easy way to play midi files, they will require a soundfont and those are usually very big in size, which is not appropriate for the web
- Keep games in a reasonable size for a website, leverage AGS Sprite Compression.


## Single Game Launcher

The `ags.wasm` and `ags.js` are the main parts of the AGS engine Web Ports. If you add an additional `my_game_files.js` file, it will be detected by the `ags.html` file and it will behave as a single game launcher.

- follow the above instructions to build the `ags.wasm`, `ags.js` and `ags.html` files.
- create an empty directory and place these files in it
- copy all files from the `Compiled/Data` directory from the game you are porting and place on the directory you copied `ags.wasm` and other files
- copy `my_game_files.js` to the same directory, and edit it so it contains the name of all files from `Compiled/Data`. List only files directly, if your games uses subdata directory, adapt it to a flat directory.
- rename the html file to `index.html`
- serve this directory (using `python -m http.server` or similar), and verify it's working in your web browser

You can now zip this directory and upload it in game stores that support HTML5 games (itch.io, gamejolt, ...). You now have a web port of your game!

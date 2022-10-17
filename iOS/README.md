# AGS for iOS

This port was initally done by JJS, when the backend was Allegro4. Currently, it uses SDL2.

iOS thread on the AGS forum: https://www.adventuregamestudio.co.uk/forums/index.php?topic=46040.0

## Building

The iOS app consists of two parts:

-   iOS app and engine
-   SDL2 xcFramework

Currently building both requires Xcode and macOS. Development was originally done in Xcode 14.


## Building SDL2 Framework for iOS

The SDL2 Framework for iOS is not yet provided as release artifacts, so you have to build it yourself. If you are in an arm based mac, you may need to use the xcFramework-iOS instead, but it's not available in a release yet.

1. Download **SDL** and extract it to a directory of your choice
2. Start **Xcode** and open the SDL project in `Xcode/SDL/SDL.xcodeproj`
3. In the Xcode title menu, go in **Product**, **Scheme**, **Choose Scheme** and  select **Framework-iOS**
4. Still in **Product**, go in **Destination** and select **Any iOS Device (arm64)** or select one of the **iOS Simulators** target (e.g. iPhone 14) if you are still going to test in a simulator.
5. Now in **Product**, **Archive** should be available, hit it to build the xcFramework
6. Wait for the build to finish, when it's done, in the window that appears, make sure the selected archive is the one you just build and click **Distribute Content**
7. Now click in **Built Products** and save it somewhere you remember.

After the `SDL2.Framework` is built, navigate to it in Finder and copy it on the directory you cloned the ags source code, in the `<SOURCE>/iOS/xcode/ags/` directory. You can close the SDL xcode project now.

## Building AGS for iOS

After building the SDL2 Framework and placing it in the indicated directory, open Xcode and load the project in `<SOURCE>/iOS/xcode/ags/ags.xcodeproj`.


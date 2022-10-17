# AGS for iOS

This port was initally done by JJS, when the backend was Allegro4. Currently, it uses SDL2.

iOS thread on the AGS forum: https://www.adventuregamestudio.co.uk/forums/index.php?topic=46040.0

## Building

Currently building both requires Xcode and macOS. Development was originally done in Xcode 14.

The first thing you need to do is to get all dependencies necessary in `libsrc/` in the project root.
To do this, navigate to it and run the `./download.sh` script there.

After the script is ran, open Xcode and load the project in `<SOURCE>/iOS/xcode/ags/ags.xcodeproj`.

The iOS AGS app is all contained in a single Xcode project.



Building the libraries
----------------------

First of all, you need MinPSPW 0.11 for Linux. The libraries cannot be compiled on Windows.


Allegro
-----------------------
1. Check out the MinPSPW sources from the SVN
   "svn co https://minpspw.svn.sourceforge.net/svnroot/minpspw minpspw"

2. In the checked out sources, browse to
   "minpspw/trunk/devpaks/041_allegro/"
   and run
   "./devpak.sh"
   Watch the Allegro library being built.

3. Go to the directory
   "minpspw/trunk/devpaks/041_allegro/build/allegro-4.4.1.1/"
   and apply the patch
   "patch -p0 < allegro-4.4.1.1_minpspw.patch"
   from the patches directory of the AGS source.

4. Type "make" in the same directory and the patched Allegro library will build.
   It will fail to compile the samples because of the patching, but you can
   ignore that.

5. The library files are located in
   "minpspw/trunk/devpaks/041_allegro/build/allegro-4.4.1.1/lib/"



libTheora
-----------------------
This instruction follows the same pattern as the building of e.g. libVorbis in MinPSPW.

1. Download the library sources from
   "http://downloads.xiph.org/releases/theora/libtheora-1.1.1.tar.bz2"
   and unpack it.

2. Change to the directory
   "libtheora-1.1.1/"
   and apply the patch with
   "patch -p0 < libtheora-1.1.1.patch"

3. Run "./configure; make"

4. The library will be written to
   "libtheora-1.1.1/lib/.libs"



libDumb
-----------------------
1. Download the library source from
   "http://sourceforge.net/projects/dumb/files/dumb/0.9.3/dumb-0.9.3.tar.gz/download"
   and upack it.

2. Change into the dumb-0.9.3 directory and apply the patch with
   "patch -p0 < libdumb-0.9.3.patch"

3. Type "make" to build the library.

4. The output will be in
   "lib/psp/"


Exception Handler for Kernel 3.71
---------------------------------
All credits goes to crazyc (for the prx code) and SamuraiX (sample usage code)

What is it?
-----------
exception.prx is a prx that handle exception and can be used 
to show usefull information abuot a crash.


How to build:
-------------
To build exception.prx

$cd prx
$make

To build the test program:
$cd test
$make


How to use it:
--------------
Include utility/exception.h in your main.c and init the handler with:

initExceptionHandler();

Copy exception.prx in the same directory your EBOOT is.

Please check the test program. ;)

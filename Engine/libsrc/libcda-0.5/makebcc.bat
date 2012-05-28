@echo off
rem 
rem  I don't know how to write Borland-style makefiles.
rem  Nor do I remember the command-line options (it's been a while).
rem 

bcc -c bcc.c example.c
bcc -eexample.exe bcc.obj example.obj


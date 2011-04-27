/*
 *  macport.cpp
 *  AGSRunTime
 *
 *  Created by Steve McCrea on 6/16/05.
 *  Part of the Adventure Game Studio source code (c)1999-2005 Chris Jones.
 *
 *  Interface, display and misc code for the macosx port
 *
 *  cvs -d :pserver:macport@shiva.warpcore.org:/home/ags/cvs login
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macport.h"

//------------------------------------------------------------------
// get the length of an opened file

long flength(FILE *fp)
{
  long cur = ftell(fp);
  fseek(fp, 0, SEEK_END);
  long len = ftell(fp);
  fseek(fp, cur, SEEK_SET);
  return len;
}

//------------------------------------------------------------------


//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// macport.cpp
// Created by Steve McCrea on 6/16/05.
//
// Interface, display and misc code for the macosx port
//
//=============================================================================

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


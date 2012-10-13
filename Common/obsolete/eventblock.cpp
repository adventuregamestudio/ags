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

#include "ac/eventblock.h"

#ifdef UNUSED_CODE
// [IKM] 2012-06-22: not really used anywhere
void add_to_eventblock(EventBlock *evpt, int evnt, int whatac, int val1, int data, short scorr) {
  evpt->list[evpt->numcmd] = evnt;
  evpt->respond[evpt->numcmd] = whatac;
  evpt->respondval[evpt->numcmd] = val1;
  evpt->data[evpt->numcmd] = data;
  evpt->score[evpt->numcmd] = scorr;
  evpt->numcmd++;
}
#endif // UNUSED_CODE

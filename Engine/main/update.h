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
// Contains various update functions (moving, animating, etc).
// TODO: cleanup this h/cpp pair, the header is mostly useless, merge with
// something else.
//
//=============================================================================
#ifndef __AGS_EE_MAIN__UPDATE_H
#define __AGS_EE_MAIN__UPDATE_H

// Update MoveList of certain index, save current position;
// *resets* mslot to zero if path is complete.
// returns "need_to_fix_sprite" value, which may be 0,1,2;
// TODO: find out what this return value means, and refactor.
// TODO: do not reset mslot in this function, reset externally instead.
int do_movelist_move(short &mslot, int &pos_x, int &pos_y);
// Update various things on the game frame (historical code mess...)
void update_stuff();

#endif // __AGS_EE_MAIN__UPDATE_H

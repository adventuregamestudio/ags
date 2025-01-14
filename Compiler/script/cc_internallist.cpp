//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <stdlib.h>
#include "cc_internallist.h"

extern int currentline;  // in script_common

void ccInternalList::startread() {
    pos=0;
}

bool ccInternalList::isPosValid(int pos) {
	return pos >= 0 && pos < length;
}

int32_t ccInternalList::peeknext() {
    int tpos = pos;
	// this should work even if 3 bytes aren't remaining
	while (isPosValid(tpos) && (script[tpos] == SCODE_META)) {
        tpos+=3;
	}

	if (isPosValid(tpos)) {
		return script[tpos];
	} else {
        return SCODE_INVALID;
	}
}
int32_t ccInternalList::getnext() {
    // process line numbers internally
    while (isPosValid(pos) && script[pos] == SCODE_META) {
		int32_t bytesRemaining = length - pos;
		if (bytesRemaining >= 3) {
			if (script[pos+1] == SMETA_LINENUM) {
				currentline = script[pos+2];
			} else if (script[pos+1] == SMETA_END) {
				lineAtEnd = currentline;
				if (cancelCurrentLine) {
					currentline = -10;
				}
                // TODO DEFECT?: If we break, we return SCODE_META *and* increase pos, so next getnext will return SMETA_END.
				break;
			}
		}
        pos+=3;
    }
    if (pos >= length) {
		if (cancelCurrentLine) {
            currentline = -10;
		}
        return SCODE_INVALID;
    }

	if (isPosValid(pos)) {
		return script[pos++];
	} else {
        return SCODE_INVALID;
	}
}
void ccInternalList::write(int value) {
    if ((length+1) * sizeof(int32_t) >= (uint32_t)allocated) {

		if (allocated < 1000) {
            allocated += 1000;
		} else {
            allocated *= 2;
		}

        script = (int32_t*)realloc(script, allocated);
		// TODO: this doesn't check realloc result
    }
    script[length] = value;
    length ++;
}
// write a meta symbol (ie. non-code thingy)
void ccInternalList::write_meta(int type,int param) {
    write(SCODE_META);
    write(type);
    write(param);
}
void ccInternalList::shutdown() {
	if (script!=NULL) { free(script); }
    script = NULL;
    length = 0;
    allocated = 0;
}
void ccInternalList::init() {
    allocated = 0;
    length = 0;
    script = NULL;
	pos = -1;
	lineAtEnd = -1;
    cancelCurrentLine = 1;
}
ccInternalList::~ccInternalList() {
    shutdown();
}
ccInternalList::ccInternalList() {
    init();
}

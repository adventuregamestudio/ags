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
// Software drawing component. Optimizes drawing for software renderer using
// dirty rectangles technique.
//
//=============================================================================

#include <string.h>
#include <vector>
#include "ac/draw_software.h"
#include "gfx/bitmap.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// TODO: choose these values depending on game resolution?
#define MAXDIRTYREGIONS 25
#define WHOLESCREENDIRTY (MAXDIRTYREGIONS + 5)
#define MAX_SPANS_PER_ROW 4

struct IRSpan
{
    int x1, x2;
    int mergeSpan(int tx1, int tx2);

    IRSpan();
};

struct IRRow
{
    IRSpan span[MAX_SPANS_PER_ROW];
    int numSpans;

    IRRow();
};

struct DirtyRects
{
    // Size of the surface managed by this dirty rects object
    Size surfaceSize;

    std::vector<IRRow> dirtyRows;
    Rect dirtyRegions[MAXDIRTYREGIONS];
    size_t numDirtyRegions;

    DirtyRects();
    // Initialize dirty rects for the given surface size
    void Init(const Size &surf_size);
    // Delete dirty rects
    void Destroy();
    // Mark all surface as tidy
    void Reset();
};


IRSpan::IRSpan()
    : x1(0), x2(0)
{
}

IRRow::IRRow()
    : numSpans(0)
{
}

int IRSpan::mergeSpan(int tx1, int tx2)
{
    if ((tx1 > x2) || (tx2 < x1))
        return 0;
    // overlapping, increase the span
    if (tx1 < x1)
        x1 = tx1;
    if (tx2 > x2)
        x2 = tx2;
    return 1;
}

DirtyRects::DirtyRects()
    : numDirtyRegions(0)
{
}

void DirtyRects::Init(const Size &surf_size)
{
    int height = surf_size.Height;
    if (surfaceSize != surf_size)
    {
        Destroy();
        surfaceSize = surf_size;
        dirtyRows.resize(height);
    }

    numDirtyRegions = WHOLESCREENDIRTY;

    for (int i = 0; i < height; ++i)
        dirtyRows[i].numSpans = 0;
}

void DirtyRects::Destroy()
{
    dirtyRows.clear();
    numDirtyRegions = 0;
}

void DirtyRects::Reset()
{
    numDirtyRegions = 0;

    for (int i = 0; i < dirtyRows.size(); ++i)
        dirtyRows[i].numSpans = 0;
}


// Dirty rects object for the single room camera
DirtyRects RoomCamRects;

void destroy_invalid_regions()
{
    RoomCamRects.Destroy();
}

void init_invalid_regions(const Size &surf_size)
{
    RoomCamRects.Init(surf_size);
}

void invalidate_all_rects()
{
    // mark the whole screen dirty
    RoomCamRects.numDirtyRegions = WHOLESCREENDIRTY;
}

void invalidate_rect(int x1, int y1, int x2, int y2, const Rect &viewport)
{
    if (RoomCamRects.numDirtyRegions >= MAXDIRTYREGIONS) {
        // too many invalid rectangles, just mark the whole thing dirty
        RoomCamRects.numDirtyRegions = WHOLESCREENDIRTY;
        return;
    }

    int a;

    if (x1 >= viewport.GetWidth()) x1 = viewport.GetWidth() - 1;
    if (y1 >= viewport.GetHeight()) y1 = viewport.GetHeight() - 1;
    if (x2 >= viewport.GetWidth()) x2 = viewport.GetWidth() - 1;
    if (y2 >= viewport.GetHeight()) y2 = viewport.GetHeight() - 1;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 < 0) x2 = 0;
    if (y2 < 0) y2 = 0;
    RoomCamRects.numDirtyRegions++;

    // ** Span code
    std::vector<IRRow> &dirtyRow = RoomCamRects.dirtyRows;
    int s, foundOne;
    // add this rect to the list for this row
    for (a = y1; a <= y2; a++) {
        foundOne = 0;
        for (s = 0; s < dirtyRow[a].numSpans; s++) {
            if (dirtyRow[a].span[s].mergeSpan(x1, x2)) {
                foundOne = 1;
                break;
            }
        }
        if (foundOne) {
            // we were merged into a span, so we're ok
            int t;
            // check whether now two of the spans overlap each other
            // in which case merge them
            for (s = 0; s < dirtyRow[a].numSpans; s++) {
                for (t = s + 1; t < dirtyRow[a].numSpans; t++) {
                    if (dirtyRow[a].span[s].mergeSpan(dirtyRow[a].span[t].x1, dirtyRow[a].span[t].x2)) {
                        dirtyRow[a].numSpans--;
                        for (int u = t; u < dirtyRow[a].numSpans; u++)
                            dirtyRow[a].span[u] = dirtyRow[a].span[u + 1];
                        break;
                    }
                }
            }
        }
        else if (dirtyRow[a].numSpans < MAX_SPANS_PER_ROW) {
            dirtyRow[a].span[dirtyRow[a].numSpans].x1 = x1;
            dirtyRow[a].span[dirtyRow[a].numSpans].x2 = x2;
            dirtyRow[a].numSpans++;
        }
        else {
            // didn't fit in an existing span, and there are none spare
            int nearestDist = 99999, nearestWas = -1, extendLeft;
            int tleft, tright;
            // find the nearest span, and enlarge that to include this rect
            for (s = 0; s < dirtyRow[a].numSpans; s++) {
                tleft = dirtyRow[a].span[s].x1 - x2;
                if ((tleft > 0) && (tleft < nearestDist)) {
                    nearestDist = tleft;
                    nearestWas = s;
                    extendLeft = 1;
                }
                tright = x1 - dirtyRow[a].span[s].x2;
                if ((tright > 0) && (tright < nearestDist)) {
                    nearestDist = tright;
                    nearestWas = s;
                    extendLeft = 0;
                }
            }
            if (extendLeft)
                dirtyRow[a].span[nearestWas].x1 = x1;
            else
                dirtyRow[a].span[nearestWas].x2 = x2;
        }
    }
    // ** End span code
    //}
}

void update_invalid_region(Bitmap *ds, int x, int y, Bitmap *src, const Rect &viewport)
{
    int i;
    // convert the offsets for the destination into
    // offsets into the source
    x = -x;
    y = -y;

    if (RoomCamRects.numDirtyRegions == WHOLESCREENDIRTY) {
        ds->Blit(src, x, y, 0, 0, ds->GetWidth(), ds->GetHeight());
    }
    else {
        std::vector<IRRow> &dirtyRow = RoomCamRects.dirtyRows;
        int k, tx1, tx2, srcdepth = src->GetColorDepth();
        if ((srcdepth == ds->GetColorDepth()) && (ds->IsMemoryBitmap())) {
            int bypp = src->GetBPP();
            // do the fast copy
            for (i = 0; i < viewport.GetHeight(); i++) {
                const uint8_t *src_scanline = src->GetScanLine(i + y);
                uint8_t *dst_scanline = ds->GetScanLineForWriting(i);
                const IRRow &dirty_row = dirtyRow[i];
                for (k = 0; k < dirty_row.numSpans; k++) {
                    tx1 = dirty_row.span[k].x1;
                    tx2 = dirty_row.span[k].x2;
                    memcpy(&dst_scanline[tx1 * bypp], &src_scanline[(tx1 + x) * bypp], ((tx2 - tx1) + 1) * bypp);
                }
            }
        }
        else {
            // do the fast copy
            int rowsInOne;
            for (i = 0; i < viewport.GetHeight(); i++) {
                rowsInOne = 1;

                // if there are rows with identical masks, do them all in one go
                while ((i + rowsInOne < viewport.GetHeight()) && (memcmp(&dirtyRow[i], &dirtyRow[i + rowsInOne], sizeof(IRRow)) == 0))
                    rowsInOne++;

                const IRRow &dirty_row = dirtyRow[i];
                for (k = 0; k < dirty_row.numSpans; k++) {
                    tx1 = dirty_row.span[k].x1;
                    tx2 = dirty_row.span[k].x2;
                    ds->Blit(src, tx1 + x, i + y, tx1, i, (tx2 - tx1) + 1, rowsInOne);
                }

                i += (rowsInOne - 1);
            }
        }
    }
}

void update_invalid_region_and_reset(Bitmap *ds, int x, int y, Bitmap *src, const Rect &viewport)
{
    update_invalid_region(ds, x, y, src, viewport);

    // screen has been updated, no longer dirty
    RoomCamRects.Reset();
}

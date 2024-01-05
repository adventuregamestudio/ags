//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/walkbehind.h"
#include <algorithm>
#include "ac/draw.h"
#include "ac/gamestate.h"
#include "ac/roomstatus.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"


using namespace AGS::Common;
using namespace AGS::Engine;

extern RoomStruct thisroom;
extern GameState play;
extern IGraphicsDriver *gfxDriver;
extern RoomStatus *croom;

// An info on vertical column of walk-behind mask, which may contain WB area
struct WalkBehindColumn
{
    bool Exists = false; // whether any WB area is in this column
    int Y1 = 0, Y2 = 0; // WB top and bottom Y coords
};

std::vector<WalkBehindColumn> walkBehindCols; // precalculated WB positions
Rect walkBehindAABB[MAX_WALK_BEHINDS]; // WB bounding box
int walkBehindsCachedForBgNum = 0; // WB textures are for this background
bool noWalkBehindsAtAll = false; // quick report that no WBs in this room
bool walk_behind_baselines_changed = false;


// Generates walk-behinds as separate sprites
void walkbehinds_generate_sprites()
{
    const Bitmap *mask = thisroom.WalkBehindMask.get();
    const Bitmap *bg = thisroom.BgFrames[play.bg_frame].Graphic.get();
    
    const int coldepth = bg->GetColorDepth();
    Bitmap wbbmp; // temp buffer
    // Iterate through walk-behinds and generate a texture for each of them
    for (int wb = 1 /* 0 is "no area" */; wb < MAX_WALK_BEHINDS; ++wb)
    {
        const Rect pos = walkBehindAABB[wb];
        if (pos.Right > 0)
        {
            wbbmp.CreateTransparent(pos.GetWidth(), pos.GetHeight(), coldepth);
            // Copy over all solid pixels belonging to this WB area
            const int sx = pos.Left, ex = pos.Right, sy = pos.Top, ey = pos.Bottom;
            for (int y = sy; y <= ey; ++y)
            {
                const uint8_t *check_line = mask->GetScanLine(y);
                const uint8_t *src_line = bg->GetScanLine(y);
                uint8_t *dst_line = wbbmp.GetScanLineForWriting(y - sy);
                for (int x = sx; x <= ex; ++x)
                {
                    if (check_line[x] != wb) continue;
                    switch (coldepth)
                    {
                    case 8:
                        dst_line[(x - sx)] = src_line[x];
                        break;
                    case 16:
                        reinterpret_cast<uint16_t*>(dst_line)[(x - sx)] =
                            reinterpret_cast<const uint16_t*>(src_line)[x];
                        break;
                    case 32:
                        reinterpret_cast<uint32_t*>(dst_line)[(x - sx)] =
                            reinterpret_cast<const uint32_t*>(src_line)[x];
                        break;
                    default: assert(0); break;
                    }
                }
            }
            // Add to walk-behinds image list
            add_walkbehind_image(wb, &wbbmp, pos.Left, pos.Top);
        }
    }

    walkBehindsCachedForBgNum = play.bg_frame;
}

// Edits the given game object's sprite, cutting out pixels covered by walk-behinds;
// returns whether any pixels were updated;
bool walkbehinds_cropout(Bitmap *sprit, int sprx, int spry, int basel)
{
    if (noWalkBehindsAtAll)
        return false;

    const int maskcol = sprit->GetMaskColor();
    const int spcoldep = sprit->GetColorDepth();

    bool pixels_changed = false;
    // pass along the sprite's pixels, but skip those that lie outside the mask
    for (int x = std::max(0, 0 - sprx);
        (x < sprit->GetWidth()) && (x + sprx < thisroom.WalkBehindMask->GetWidth()); ++x)
    {
        // select the WB column at this x
        const auto &wbcol = walkBehindCols[x + sprx];
        // skip if no area, or sprite lies outside of all areas in this column
        if ((!wbcol.Exists) ||
            (wbcol.Y2 <= spry) ||
            (wbcol.Y1 > spry + sprit->GetHeight()))
            continue;

        // ensure we only check within the valid areas (between Y1 and Y2)
        // we assume that Y1 and Y2 are always within the mask
        for (int y = std::max(0, wbcol.Y1 - spry);
            (y < sprit->GetHeight()) && (y + spry < wbcol.Y2); ++y)
        {
            const int wb = thisroom.WalkBehindMask->GetScanLine(y + spry)[x + sprx];
            if (wb < 1) continue; // "no area"
            if (croom->walkbehind_base[wb] <= basel) continue;

            pixels_changed = true;
            uint8_t *dst_line = sprit->GetScanLineForWriting(y);
            switch (spcoldep)
            {
            case 8:
                dst_line[x] = maskcol;
                break;
            case 16:
                reinterpret_cast<uint16_t*>(dst_line)[x] = maskcol;
                break;
            case 32:
                reinterpret_cast<uint32_t*>(dst_line)[x] = maskcol;
                break;
            default:
                assert(0);
                break;
            }
        }
    }
    return pixels_changed;
}

void walkbehinds_recalc()
{
    // Reset all data
    walkBehindCols.clear();
    for (int wb = 0; wb < MAX_WALK_BEHINDS; ++wb)
    {
        walkBehindAABB[wb] = Rect(INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN);
    }
    noWalkBehindsAtAll = true;

    // Recalculate everything; note that mask is always 8-bit
    const Bitmap *mask = thisroom.WalkBehindMask.get();
    walkBehindCols.resize(mask->GetWidth());
    for (int col = 0; col < mask->GetWidth(); ++col)
    {
        auto &wbcol = walkBehindCols[col];
        for (int y = 0; y < mask->GetHeight(); ++y)
        {
            int wb = mask->GetScanLine(y)[col];
            // Valid areas start with index 1, 0 = no area
            if ((wb >= 1) && (wb < MAX_WALK_BEHINDS))
            {
                if (!wbcol.Exists)
                {
                    wbcol.Y1 = y;
                    wbcol.Exists = true;
                    noWalkBehindsAtAll = false;
                }
                wbcol.Y2 = y + 1; // +1 to allow bottom line of screen to work (CHECKME??)
                // resize the bounding rect
                walkBehindAABB[wb].Left = std::min(col, walkBehindAABB[wb].Left);
                walkBehindAABB[wb].Top = std::min(y, walkBehindAABB[wb].Top);
                walkBehindAABB[wb].Right = std::max(col, walkBehindAABB[wb].Right);
                walkBehindAABB[wb].Bottom = std::max(y, walkBehindAABB[wb].Bottom);
            }
        }
    }
}

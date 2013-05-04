
#include "ac/common.h"
#include "ac/runtime_defines.h"
#include "ac/viewframe.h"
#include "game/game_objects.h"
#include "main/update.h"
#include "util/alignedstream.h"

extern int spritewidth[MAX_SPRITES];
extern int spriteheight[MAX_SPRITES];
extern ViewStruct*views;

namespace AGS
{
namespace Engine
{

using Common::AlignedStream;
using Common::Stream;

int RoomObject::GetWidth()
{
    if (LastWidth == 0)
        return spritewidth[SpriteIndex];
    return LastWidth;
}

int RoomObject::GetHeight() {
    if (LastHeight == 0)
        return spriteheight[SpriteIndex];
    return LastHeight;
}

int RoomObject::GetBaseline()
{
    if (Baseline < 1)
        return Y;
    return Baseline;
}

void RoomObject::UpdateCyclingView()
{
	if (IsOn != 1) return;
    if (Moving>0) {
      do_movelist_move(ObjMoveLists, &Moving,&X,&Y);
      }
    if (Cycling==0) return;
    if (View<0) return;
    if (Wait>0) { Wait--; return; }

    if (Cycling >= ANIM_BACKWARDS) {

      UpdateCycleViewBackwards();

    }
    else {  // Animate forwards
      
	  UpdateCycleViewForwards();

    }  // end if forwards

    ViewFrame*vfptr=&views[View].loops[Loop].frames[Frame];
    SpriteIndex = vfptr->pic;

    if (Cycling == 0)
      return;

    Wait=vfptr->speed+OverallSpeed;
    CheckViewFrame (View, Loop, Frame);
}

void RoomObject::UpdateCycleViewForwards()
{
	Frame++;
      if (Frame >= views[View].loops[Loop].numFrames) {
        // go to next Loop thing
        if (views[View].loops[Loop].RunNextLoop()) {
          if (Loop+1 >= views[View].numLoops)
            quit("!Last Loop in a View requested to move to next Loop");
          Loop++;
          Frame=0;
        }
        else if (Cycling % ANIM_BACKWARDS == ANIM_ONCE) {
          // leave it IsOn the last Frame
          Cycling=0;
          Frame--;
          }
        else {
          if (play.NoMultiLoopRepeat == 0) {
            // multi-Loop anims, go back to start of it
            while ((Loop > 0) && 
              (views[View].loops[Loop - 1].RunNextLoop()))
              Loop --;
          }
          if (Cycling % ANIM_BACKWARDS == ANIM_ONCERESET)
            Cycling=0;
          Frame=0;
        }
      }
}

void RoomObject::UpdateCycleViewBackwards()
{
	// animate backwards
      Frame--;
      if (Frame < 0) {
        if ((Loop > 0) && 
           (views[View].loops[Loop - 1].RunNextLoop())) 
        {
          // If it's a Go-to-next-Loop IsOn the previous one, then go back
          Loop --;
          Frame = views[View].loops[Loop].numFrames - 1;
        }
        else if (Cycling % ANIM_BACKWARDS == ANIM_ONCE) {
          // leave it IsOn the first Frame
          Cycling = 0;
          Frame = 0;
        }
        else { // repeating animation
          Frame = views[View].loops[Loop].numFrames - 1;
        }
      }
}

void RoomObject::ReadFromFile(Stream *in)
{
    X           = in->ReadInt32();
    Y           = in->ReadInt32();
    Transparency = in->ReadInt32();
    TintR       = in->ReadInt16();
    TintG       = in->ReadInt16();
    TintB       = in->ReadInt16();
    TintLevel   = in->ReadInt16();
    TintLight   = in->ReadInt16();
    LastZoom    = in->ReadInt16();
    LastWidth   = in->ReadInt16();
    LastHeight  = in->ReadInt16();
    SpriteIndex = in->ReadInt16();
    Baseline    = in->ReadInt16();
    View        = in->ReadInt16();
    Loop        = in->ReadInt16();
    Frame       = in->ReadInt16();
    Wait        = in->ReadInt16();
    Moving      = in->ReadInt16();
    Cycling     = in->ReadInt8();
    OverallSpeed = in->ReadInt8();
    IsOn        = in->ReadInt8();
    Flags       = in->ReadInt8();
    BlockingWidth = in->ReadInt16();
    BlockingHeight = in->ReadInt16();
}

void RoomObject::WriteToFile(Stream *out)
{
    out->WriteInt32(X);
    out->WriteInt32(Y);
    out->WriteInt32(Transparency);
    out->WriteInt16(TintR);
    out->WriteInt16(TintG);
    out->WriteInt16(TintB);
    out->WriteInt16(TintLevel);
    out->WriteInt16(TintLight);
    out->WriteInt16(LastZoom);
    out->WriteInt16(LastWidth);
    out->WriteInt16(LastHeight);
    out->WriteInt16(SpriteIndex);
    out->WriteInt16(Baseline);
    out->WriteInt16(View);
    out->WriteInt16(Loop);
    out->WriteInt16(Frame);
    out->WriteInt16(Wait);
    out->WriteInt16(Moving);
    out->WriteInt8(Cycling);
    out->WriteInt8(OverallSpeed);
    out->WriteInt8(IsOn);
    out->WriteInt8(Flags);
    out->WriteInt16(BlockingWidth);
    out->WriteInt16(BlockingHeight);
}

} // namespace Engine
} // namespace AGS

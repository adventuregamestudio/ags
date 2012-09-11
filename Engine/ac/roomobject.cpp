
#include <stdio.h>
#include "util/wgt2allg.h"
#include "ac/roomobject.h"
#include "ac/common.h"
#include "ac/common_defines.h"
#include "ac/gamestate.h"
#include "ac/runtime_defines.h"
#include "ac/viewframe.h"
#include "main/update.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;


extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern ViewStruct*views;
extern GameState play;

int RoomObject::get_width() {
    if (last_width == 0)
        return spritewidth[num];
    return last_width;
}
int RoomObject::get_height() {
    if (last_height == 0)
        return spriteheight[num];
    return last_height;
}
int RoomObject::get_baseline() {
    if (baseline < 1)
        return y;
    return baseline;
}

void RoomObject::UpdateCyclingView()
{
	if (on != 1) return;
    if (moving>0) {
      do_movelist_move(&moving,&x,&y);
      }
    if (cycling==0) return;
    if (view<0) return;
    if (wait>0) { wait--; return; }

    if (cycling >= ANIM_BACKWARDS) {

      update_cycle_view_backwards();

    }
    else {  // Animate forwards
      
	  update_cycle_view_forwards();

    }  // end if forwards

    ViewFrame*vfptr=&views[view].loops[loop].frames[frame];
    num = vfptr->pic;

    if (cycling == 0)
      return;

    wait=vfptr->speed+overall_speed;
    CheckViewFrame (view, loop, frame);
}


void RoomObject::update_cycle_view_forwards()
{
	frame++;
      if (frame >= views[view].loops[loop].numFrames) {
        // go to next loop thing
        if (views[view].loops[loop].RunNextLoop()) {
          if (loop+1 >= views[view].numLoops)
            quit("!Last loop in a view requested to move to next loop");
          loop++;
          frame=0;
        }
        else if (cycling % ANIM_BACKWARDS == ANIM_ONCE) {
          // leave it on the last frame
          cycling=0;
          frame--;
          }
        else {
          if (play.no_multiloop_repeat == 0) {
            // multi-loop anims, go back to start of it
            while ((loop > 0) && 
              (views[view].loops[loop - 1].RunNextLoop()))
              loop --;
          }
          if (cycling % ANIM_BACKWARDS == ANIM_ONCERESET)
            cycling=0;
          frame=0;
        }
      }
}

void RoomObject::update_cycle_view_backwards()
{
	// animate backwards
      frame--;
      if (frame < 0) {
        if ((loop > 0) && 
           (views[view].loops[loop - 1].RunNextLoop())) 
        {
          // If it's a Go-to-next-loop on the previous one, then go back
          loop --;
          frame = views[view].loops[loop].numFrames - 1;
        }
        else if (cycling % ANIM_BACKWARDS == ANIM_ONCE) {
          // leave it on the first frame
          cycling = 0;
          frame = 0;
        }
        else { // repeating animation
          frame = views[view].loops[loop].numFrames - 1;
        }
      }
}

void RoomObject::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    in->ReadArrayOfInt32(&x, 3);
    in->ReadArrayOfInt16(&tint_r, 15);
    in->ReadArrayOfInt8((int8_t*)&cycling, 4);
    in->ReadArrayOfInt16(&blocking_width, 2);
    in->Seek(Common::kSeekCurrent, 2);
//#else
//    throw "RoomObject::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}
void RoomObject::WriteToFile(CDataStream *out)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    out->WriteArrayOfInt32(&x, 3);
    out->WriteArrayOfInt16(&tint_r, 15);
    out->WriteArrayOfInt8((int8_t*)&cycling, 4);
    out->WriteArrayOfInt16(&blocking_width, 2);
    out->WriteInt8(0);
    out->WriteInt8(0);
//#else
//    throw "RoomObject::WriteToFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}

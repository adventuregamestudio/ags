
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gui/guiinv.h"
#include "gui/guimain.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

DynamicArray<GUIInv> guiinv;
int numguiinv = 0;

void GUIInv::WriteToFile(CDataStream *out)
{
	GUIObject::WriteToFile(out);
	out->WriteInt32(charId);
	out->WriteInt32(itemWidth);
	out->WriteInt32(itemHeight);
	out->WriteInt32(topIndex);
}

void GUIInv::ReadFromFile(CDataStream *in, int version)
{
	GUIObject::ReadFromFile(in, version);
	if (version >= 109) {
	  charId = in->ReadInt32();
	  itemWidth = in->ReadInt32();
	  itemHeight = in->ReadInt32();
	  topIndex = in->ReadInt32();
	}
	else {
	  charId = -1;
	  itemWidth = 40;
	  itemHeight = 22;
	  topIndex = 0;
	}

	if (loaded_game_file_version >= 31) // 2.70
	{
	  // ensure that some items are visible
	  if (itemWidth > wid)
		itemWidth = wid;
	  if (itemHeight > hit)
		itemHeight = hit;
	}

	CalculateNumCells();
}

void GUIInv::CalculateNumCells() {
  if (loaded_game_file_version >= 31) // 2.70
  {
    itemsPerLine = wid / multiply_up_coordinate(itemWidth);
    numLines = hit / multiply_up_coordinate(itemHeight);
  }
  else
  {
    itemsPerLine = floor((float)wid / (float)multiply_up_coordinate(itemWidth) + 0.5f);
    numLines = floor((float)hit / (float)multiply_up_coordinate(itemHeight) + 0.5f);
  }
}

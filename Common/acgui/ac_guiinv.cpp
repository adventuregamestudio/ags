
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "acgui/ac_guiinv.h"
#include "acgui/ac_guimain.h"


DynamicArray<GUIInv> guiinv;
int numguiinv = 0;

void GUIInv::WriteToFile(FILE * ooo)
{
	GUIObject::WriteToFile(ooo);
	putw(charId, ooo);
	putw(itemWidth, ooo);
	putw(itemHeight, ooo);
	putw(topIndex, ooo);
}

void GUIInv::ReadFromFile(FILE * ooo, int version)
{
	GUIObject::ReadFromFile(ooo, version);
	if (version >= 109) {
	  charId = getw(ooo);
	  itemWidth = getw(ooo);
	  itemHeight = getw(ooo);
	  topIndex = getw(ooo);
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

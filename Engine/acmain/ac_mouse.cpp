
#include "acmain/ac_maindefines.h"


void RefreshMouse() {
  domouse(DOMOUSE_NOCURSOR);
  scmouse.x = divide_down_coordinate(mousex);
  scmouse.y = divide_down_coordinate(mousey);
}

void SetMousePosition (int newx, int newy) {
  if (newx < 0)
    newx = 0;
  if (newy < 0)
    newy = 0;
  if (newx >= BASEWIDTH)
    newx = BASEWIDTH - 1;
  if (newy >= GetMaxScreenHeight())
    newy = GetMaxScreenHeight() - 1;

  multiply_up_coordinates(&newx, &newy);
  filter->SetMousePosition(newx, newy);
  RefreshMouse();
}

int GetCursorMode() {
  return cur_mode;
}

int GetMouseCursor() {
  return cur_cursor;
}

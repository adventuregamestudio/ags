
#include "acmain/ac_maindefines.h"
#include "acmain/ac_system.h"

int System_GetColorDepth() {
  return final_col_dep;
}

int System_GetOS() {
  return scsystem.os;
}

int System_GetScreenWidth() {
  return final_scrn_wid;
}

int System_GetScreenHeight() {
  return final_scrn_hit;
}

int System_GetViewportHeight() {
  return divide_down_coordinate(scrnhit);
}

int System_GetViewportWidth() {
  return divide_down_coordinate(scrnwid);
}

const char *System_GetVersion() {
  return CreateNewScriptString(ACI_VERSION_TEXT);
}

int System_GetHardwareAcceleration() 
{
  return gfxDriver->HasAcceleratedStretchAndFlip() ? 1 : 0;
}

int System_GetNumLock()
{
  return (key_shifts & KB_NUMLOCK_FLAG) ? 1 : 0;
}

int System_GetCapsLock()
{
  return (key_shifts & KB_CAPSLOCK_FLAG) ? 1 : 0;
}

int System_GetScrollLock()
{
  return (key_shifts & KB_SCROLOCK_FLAG) ? 1 : 0;
}

void System_SetNumLock(int newValue)
{
  // doesn't work ... maybe allegro doesn't implement this on windows
  int ledState = key_shifts & (KB_SCROLOCK_FLAG | KB_CAPSLOCK_FLAG);
  if (newValue)
  {
    ledState |= KB_NUMLOCK_FLAG;
  }
  set_leds(ledState);
}

int System_GetVsync() {
  return scsystem.vsync;
}

void System_SetVsync(int newValue) {
  scsystem.vsync = newValue;
}

int System_GetWindowed() {
  if (usetup.windowed)
    return 1;
  return 0;
}


int System_GetSupportsGammaControl() {
  return gfxDriver->SupportsGammaControl();
}

int System_GetGamma() {
  return play.gamma_adjustment;
}

void System_SetGamma(int newValue) {
  if ((newValue < 0) || (newValue > 200))
    quitprintf("!System.Gamma: value must be between 0-200 (not %d)", newValue);

  if (play.gamma_adjustment != newValue) {
    DEBUG_CONSOLE("Gamma control set to %d", newValue);
    play.gamma_adjustment = newValue;

    if (gfxDriver->SupportsGammaControl())
      gfxDriver->SetGamma(newValue);
  }
}

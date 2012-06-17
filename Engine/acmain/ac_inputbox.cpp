
#include "wgt2allg.h"
#include "ali3d.h"
#include "acmain/ac_maindefines.h"
#include "ac/ac_common.h"
#include "acmain/ac_game.h"
#include "acdialog/ac_cscidialog.h"
#include "acmain/ac_translation.h"
#include "acmain/ac_string.h"


void sc_inputbox(const char*msg,char*bufr) {
  VALIDATE_STRING(bufr);
  setup_for_dialog();
  enterstringwindow(get_translation(msg),bufr);
  restore_after_dialog();
  }

const char* Game_InputBox(const char *msg) {
  char buffer[STD_BUFFER_SIZE];
  sc_inputbox(msg, buffer);
  return CreateNewScriptString(buffer);
}

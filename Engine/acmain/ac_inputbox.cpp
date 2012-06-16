
#include "acmain/ac_maindefines.h"


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
